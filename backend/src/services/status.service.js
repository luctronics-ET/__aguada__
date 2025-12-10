/**
 * Status Service - Gerenciamento de status online/offline
 *
 * Rastreia estado de conexão de sensores e gateway com timeout configurável.
 * Usa cache em memória para performance + persistência em banco de dados.
 */

import logger from "../config/logger.js";
import pool from "../config/database.js";
import { EventEmitter } from "events";

// Configurações padrão de timeout (em segundos)
const DEFAULT_CONFIG = {
  // Timeout para considerar sensor offline (segundos)
  sensorTimeoutSec: 120, // 2 minutos (heartbeat é 30s)

  // Timeout para considerar gateway offline (segundos)
  gatewayTimeoutSec: 60, // 1 minuto

  // Intervalo para verificação de status (segundos)
  checkIntervalSec: 30,

  // Tempo para status "warning" antes de offline (segundos)
  warningThresholdSec: 60, // 1 minuto

  // Manter histórico de heartbeats (quantos)
  heartbeatHistorySize: 10,
};

class StatusService extends EventEmitter {
  constructor() {
    super();

    // Estado em memória
    this.sensorStatus = new Map(); // sensor_id -> { lastSeen, status, rssi, battery, ... }
    this.gatewayStatus = new Map(); // gateway_id -> { lastSeen, status, ... }

    // Configurações (podem ser carregadas do banco)
    this.config = { ...DEFAULT_CONFIG };

    // Timer para verificação periódica
    this.checkTimer = null;

    // Histórico de heartbeats por sensor
    this.heartbeatHistory = new Map();

    this.initialized = false;
  }

  /**
   * Inicializa o serviço e carrega configurações
   */
  async initialize() {
    if (this.initialized) return;

    try {
      // Carregar configurações do banco (se existirem)
      await this.loadConfig();

      // Iniciar verificação periódica
      this.startStatusCheck();

      this.initialized = true;
      logger.info("StatusService inicializado", { config: this.config });
    } catch (error) {
      logger.error("Erro ao inicializar StatusService:", error);
    }
  }

  /**
   * Carrega configurações do banco de dados
   */
  async loadConfig() {
    try {
      const result = await pool.query(`
        SELECT chave, valor FROM aguada.configuracoes 
        WHERE categoria = 'status_timeout'
      `);

      for (const row of result.rows) {
        if (row.chave in this.config) {
          this.config[row.chave] = parseInt(row.valor, 10);
        }
      }
    } catch (error) {
      // Tabela pode não existir, usar padrões
      logger.debug("Usando configurações padrão de timeout");
    }
  }

  /**
   * Atualiza configurações de timeout
   */
  async updateConfig(newConfig) {
    const oldConfig = { ...this.config };

    // Validar e aplicar novas configurações
    if (newConfig.sensorTimeoutSec && newConfig.sensorTimeoutSec > 0) {
      this.config.sensorTimeoutSec = newConfig.sensorTimeoutSec;
    }
    if (newConfig.gatewayTimeoutSec && newConfig.gatewayTimeoutSec > 0) {
      this.config.gatewayTimeoutSec = newConfig.gatewayTimeoutSec;
    }
    if (newConfig.warningThresholdSec && newConfig.warningThresholdSec > 0) {
      this.config.warningThresholdSec = newConfig.warningThresholdSec;
    }
    if (newConfig.checkIntervalSec && newConfig.checkIntervalSec > 0) {
      this.config.checkIntervalSec = newConfig.checkIntervalSec;
      // Reiniciar timer com novo intervalo
      this.startStatusCheck();
    }

    // Persistir no banco
    try {
      for (const [key, value] of Object.entries(this.config)) {
        await pool.query(
          `
          INSERT INTO aguada.configuracoes (categoria, chave, valor, atualizado_em)
          VALUES ('status_timeout', $1, $2, NOW())
          ON CONFLICT (categoria, chave) 
          DO UPDATE SET valor = $2, atualizado_em = NOW()
        `,
          [key, value.toString()]
        );
      }
    } catch (error) {
      // Tabela pode não existir
      logger.debug("Não foi possível persistir configurações");
    }

    logger.info("Configurações de timeout atualizadas", {
      old: oldConfig,
      new: this.config,
    });

    return this.config;
  }

  /**
   * Inicia verificação periódica de status
   */
  startStatusCheck() {
    if (this.checkTimer) {
      clearInterval(this.checkTimer);
    }

    this.checkTimer = setInterval(() => {
      this.checkAllStatus();
    }, this.config.checkIntervalSec * 1000);

    logger.debug(
      `Status check iniciado (intervalo: ${this.config.checkIntervalSec}s)`
    );
  }

  /**
   * Para a verificação periódica
   */
  stopStatusCheck() {
    if (this.checkTimer) {
      clearInterval(this.checkTimer);
      this.checkTimer = null;
    }
  }

  /**
   * Registra heartbeat de sensor (chamado quando recebe telemetria)
   */
  recordSensorHeartbeat(sensorId, data = {}) {
    const now = new Date();
    const mac = data.mac || data.node_mac;

    const existing = this.sensorStatus.get(sensorId) || {
      sensorId,
      mac,
      status: "unknown",
      consecutiveFailures: 0,
    };

    const previousStatus = existing.status;

    // Atualizar estado
    const newStatus = {
      sensorId,
      mac: mac || existing.mac,
      status: "online",
      lastSeen: now,
      lastSeenIso: now.toISOString(),
      rssi: data.rssi,
      battery: data.battery || data.vcc_bat_mv,
      uptime: data.uptime,
      consecutiveFailures: 0,
      lastValue: data.value || data.distance_mm,
    };

    this.sensorStatus.set(sensorId, newStatus);

    // Registrar no histórico
    this.addHeartbeatHistory(sensorId, now, data);

    // Emitir evento se mudou de status
    if (previousStatus !== "online" && previousStatus !== "unknown") {
      this.emit("sensor:online", { sensorId, mac, previousStatus });
      logger.info(`Sensor ${sensorId} voltou online`, { mac, previousStatus });
    }

    return newStatus;
  }

  /**
   * Registra heartbeat de gateway
   */
  recordGatewayHeartbeat(gatewayId, data = {}) {
    const now = new Date();

    const existing = this.gatewayStatus.get(gatewayId) || {
      gatewayId,
      status: "unknown",
    };

    const previousStatus = existing.status;

    const newStatus = {
      gatewayId,
      mac: data.mac || existing.mac,
      status: "online",
      lastSeen: now,
      lastSeenIso: now.toISOString(),
      ipAddress: data.ipAddress,
      sensorsRelayed: data.sensorsRelayed || 0,
      uptime: data.uptime,
      freeHeap: data.freeHeap,
      wifiRssi: data.wifiRssi,
    };

    this.gatewayStatus.set(gatewayId, newStatus);

    // Emitir evento se mudou de status
    if (previousStatus !== "online" && previousStatus !== "unknown") {
      this.emit("gateway:online", { gatewayId, previousStatus });
      logger.info(`Gateway ${gatewayId} voltou online`, { previousStatus });
    }

    return newStatus;
  }

  /**
   * Adiciona entrada ao histórico de heartbeats
   */
  addHeartbeatHistory(sensorId, timestamp, data) {
    let history = this.heartbeatHistory.get(sensorId) || [];

    history.push({
      timestamp,
      rssi: data.rssi,
      battery: data.battery || data.vcc_bat_mv,
    });

    // Manter apenas os últimos N registros
    if (history.length > this.config.heartbeatHistorySize) {
      history = history.slice(-this.config.heartbeatHistorySize);
    }

    this.heartbeatHistory.set(sensorId, history);
  }

  /**
   * Verifica status de todos os sensores e gateways
   */
  checkAllStatus() {
    const now = Date.now();
    const sensorTimeoutMs = this.config.sensorTimeoutSec * 1000;
    const gatewayTimeoutMs = this.config.gatewayTimeoutSec * 1000;
    const warningMs = this.config.warningThresholdSec * 1000;

    // Verificar sensores
    for (const [sensorId, status] of this.sensorStatus.entries()) {
      if (!status.lastSeen) continue;

      const elapsed = now - status.lastSeen.getTime();
      const previousStatus = status.status;

      if (elapsed > sensorTimeoutMs) {
        status.status = "offline";
        status.consecutiveFailures++;
      } else if (elapsed > warningMs) {
        status.status = "warning";
      } else {
        status.status = "online";
      }

      // Emitir eventos de mudança
      if (previousStatus !== status.status) {
        this.emit(`sensor:${status.status}`, {
          sensorId,
          previousStatus,
          elapsed: Math.round(elapsed / 1000),
          lastSeen: status.lastSeenIso,
        });

        if (status.status === "offline") {
          logger.warn(`Sensor ${sensorId} offline`, {
            elapsed: Math.round(elapsed / 1000) + "s",
            consecutiveFailures: status.consecutiveFailures,
          });
        } else if (status.status === "warning") {
          logger.info(`Sensor ${sensorId} em warning`, {
            elapsed: Math.round(elapsed / 1000) + "s",
          });
        }
      }

      this.sensorStatus.set(sensorId, status);
    }

    // Verificar gateways
    for (const [gatewayId, status] of this.gatewayStatus.entries()) {
      if (!status.lastSeen) continue;

      const elapsed = now - status.lastSeen.getTime();
      const previousStatus = status.status;

      if (elapsed > gatewayTimeoutMs) {
        status.status = "offline";
      } else if (elapsed > warningMs) {
        status.status = "warning";
      } else {
        status.status = "online";
      }

      // Emitir eventos de mudança
      if (previousStatus !== status.status) {
        this.emit(`gateway:${status.status}`, {
          gatewayId,
          previousStatus,
          elapsed,
        });

        if (status.status === "offline") {
          logger.warn(`Gateway ${gatewayId} offline`, {
            elapsed: Math.round(elapsed / 1000) + "s",
          });
        }
      }

      this.gatewayStatus.set(gatewayId, status);
    }
  }

  /**
   * Obtém status de um sensor específico
   */
  getSensorStatus(sensorId) {
    const status = this.sensorStatus.get(sensorId);

    if (!status) {
      return {
        sensorId,
        status: "unknown",
        message: "Sensor nunca enviou dados desde o início do servidor",
      };
    }

    // Calcular tempo desde última leitura
    const elapsed = status.lastSeen
      ? Math.round((Date.now() - status.lastSeen.getTime()) / 1000)
      : null;

    return {
      ...status,
      elapsedSec: elapsed,
      elapsedFormatted: elapsed ? this.formatElapsed(elapsed) : null,
      history: this.heartbeatHistory.get(sensorId) || [],
    };
  }

  /**
   * Obtém status de todos os sensores
   */
  getAllSensorStatus() {
    const result = [];

    for (const sensorId of this.sensorStatus.keys()) {
      result.push(this.getSensorStatus(sensorId));
    }

    return result;
  }

  /**
   * Obtém status de um gateway específico
   */
  getGatewayStatus(gatewayId) {
    const status = this.gatewayStatus.get(gatewayId);

    if (!status) {
      return {
        gatewayId,
        status: "unknown",
        message: "Gateway nunca enviou dados desde o início do servidor",
      };
    }

    const elapsed = status.lastSeen
      ? Math.round((Date.now() - status.lastSeen.getTime()) / 1000)
      : null;

    return {
      ...status,
      elapsedSec: elapsed,
      elapsedFormatted: elapsed ? this.formatElapsed(elapsed) : null,
    };
  }

  /**
   * Obtém status de todos os gateways
   */
  getAllGatewayStatus() {
    const result = [];

    for (const gatewayId of this.gatewayStatus.keys()) {
      result.push(this.getGatewayStatus(gatewayId));
    }

    return result;
  }

  /**
   * Obtém resumo geral do sistema
   */
  getSystemStatusSummary() {
    const sensors = this.getAllSensorStatus();
    const gateways = this.getAllGatewayStatus();

    const sensorSummary = {
      total: sensors.length,
      online: sensors.filter((s) => s.status === "online").length,
      warning: sensors.filter((s) => s.status === "warning").length,
      offline: sensors.filter((s) => s.status === "offline").length,
      unknown: sensors.filter((s) => s.status === "unknown").length,
    };

    const gatewaySummary = {
      total: gateways.length,
      online: gateways.filter((g) => g.status === "online").length,
      warning: gateways.filter((g) => g.status === "warning").length,
      offline: gateways.filter((g) => g.status === "offline").length,
      unknown: gateways.filter((g) => g.status === "unknown").length,
    };

    // Status geral do sistema
    let systemStatus = "healthy";
    if (sensorSummary.offline > 0 || gatewaySummary.offline > 0) {
      systemStatus = "degraded";
    }
    if (sensorSummary.online === 0 && sensorSummary.total > 0) {
      systemStatus = "critical";
    }
    if (gatewaySummary.online === 0 && gatewaySummary.total > 0) {
      systemStatus = "critical";
    }

    return {
      systemStatus,
      timestamp: new Date().toISOString(),
      config: this.config,
      sensors: sensorSummary,
      gateways: gatewaySummary,
    };
  }

  /**
   * Formata tempo decorrido
   */
  formatElapsed(seconds) {
    if (seconds < 60) {
      return `${seconds}s`;
    } else if (seconds < 3600) {
      const min = Math.floor(seconds / 60);
      const sec = seconds % 60;
      return `${min}m ${sec}s`;
    } else {
      const hours = Math.floor(seconds / 3600);
      const min = Math.floor((seconds % 3600) / 60);
      return `${hours}h ${min}m`;
    }
  }

  /**
   * Obtém configurações atuais
   */
  getConfig() {
    return { ...this.config };
  }

  /**
   * Limpa status de um sensor (para reset/manutenção)
   */
  clearSensorStatus(sensorId) {
    this.sensorStatus.delete(sensorId);
    this.heartbeatHistory.delete(sensorId);
    logger.info(`Status do sensor ${sensorId} limpo`);
  }

  /**
   * Limpa todos os status (para reset)
   */
  clearAllStatus() {
    this.sensorStatus.clear();
    this.gatewayStatus.clear();
    this.heartbeatHistory.clear();
    logger.info("Todos os status limpos");
  }

  /**
   * Registra sensores conhecidos (a partir de configuração)
   * Usado para inicializar o status com sensores mesmo sem dados recebidos
   */
  registerKnownSensors(sensors) {
    for (const sensor of sensors) {
      const sensorId = sensor.sensor_id || sensor.sensorId;
      if (!this.sensorStatus.has(sensorId)) {
        this.sensorStatus.set(sensorId, {
          sensorId,
          mac: sensor.mac_address || sensor.mac,
          elementoId: sensor.elemento_id || sensor.ativo_id,
          status: "unknown",
          lastSeen: null,
          lastSeenIso: null,
          message: "Aguardando dados do sensor",
        });
      }
    }
    logger.info(`Registrados ${sensors.length} sensores conhecidos`);
  }

  /**
   * Registra gateways conhecidos
   */
  registerKnownGateways(gateways) {
    for (const gateway of gateways) {
      const gatewayId = gateway.gateway_id || gateway.mac;
      if (!this.gatewayStatus.has(gatewayId)) {
        this.gatewayStatus.set(gatewayId, {
          gatewayId,
          mac: gateway.mac,
          status: "unknown",
          lastSeen: null,
          lastSeenIso: null,
          message: "Aguardando dados do gateway",
        });
      }
    }
    logger.info(`Registrados ${gateways.length} gateways conhecidos`);
  }

  /**
   * Simula um sensor online (para testes)
   */
  simulateSensorOnline(sensorId, data = {}) {
    this.recordSensorHeartbeat(sensorId, {
      mac: data.mac || "simulated",
      rssi: data.rssi || -50,
      battery: data.battery || 5000,
      uptime: data.uptime || 0,
      value: data.value || 0,
    });
    return this.getSensorStatus(sensorId);
  }
}

// Singleton
const statusService = new StatusService();

export default statusService;
