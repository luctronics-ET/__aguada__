import { SerialPort } from 'serialport';
import { ReadlineParser } from '@serialport/parser-readline';
import logger from '../config/logger.js';
import fetch from 'node-fetch';

/**
 * Serial Bridge - Captura dados do Gateway ESP32 via USB
 * Gateway envia JSON via serial após receber ESP-NOW dos sensores
 */

class SerialBridge {
  constructor(config = {}) {
    this.port = null;
    this.parser = null;
    this.isConnected = false;
    
    // Configurações
    this.portPath = config.portPath || '/dev/ttyACM0';
    this.baudRate = config.baudRate || 115200;
    this.backendUrl = config.backendUrl || 'http://localhost:3000/api/telemetry';
    this.autoReconnect = config.autoReconnect !== false;
    this.reconnectDelay = config.reconnectDelay || 5000;
    
    // Estatísticas
    this.stats = {
      packetsReceived: 0,
      packetsSent: 0,
      errors: 0,
      lastPacketTime: null,
      startTime: Date.now(),
    };
  }

  /**
   * Conecta à porta serial do gateway
   */
  connect() {
    try {
      logger.info(`[Serial Bridge] Conectando a ${this.portPath} @ ${this.baudRate} baud...`);

      this.port = new SerialPort({
        path: this.portPath,
        baudRate: this.baudRate,
        autoOpen: false,
      });

      // Parser para ler linhas
      this.parser = this.port.pipe(new ReadlineParser({ delimiter: '\n' }));

      // Event: porta aberta
      this.port.on('open', () => {
        this.isConnected = true;
        logger.info(`[Serial Bridge] ✅ Conectado a ${this.portPath}`);
        logger.info(`[Serial Bridge] Aguardando dados do gateway...`);
      });

      // Event: dados recebidos
      this.parser.on('data', (line) => {
        this.handleIncomingData(line.trim());
      });

      // Event: erro
      this.port.on('error', (err) => {
        this.stats.errors++;
        logger.error(`[Serial Bridge] Erro na porta serial:`, err);
        
        if (this.autoReconnect) {
          this.scheduleReconnect();
        }
      });

      // Event: porta fechada
      this.port.on('close', () => {
        this.isConnected = false;
        logger.warn(`[Serial Bridge] Porta serial fechada`);
        
        if (this.autoReconnect) {
          this.scheduleReconnect();
        }
      });

      // Abrir porta
      this.port.open((err) => {
        if (err) {
          logger.error(`[Serial Bridge] Erro ao abrir porta:`, err);
          
          if (this.autoReconnect) {
            this.scheduleReconnect();
          }
        }
      });

    } catch (error) {
      logger.error(`[Serial Bridge] Erro ao criar porta serial:`, error);
      
      if (this.autoReconnect) {
        this.scheduleReconnect();
      }
    }
  }

  /**
   * Agenda reconexão
   */
  scheduleReconnect() {
    logger.info(`[Serial Bridge] Tentando reconectar em ${this.reconnectDelay / 1000}s...`);
    
    setTimeout(() => {
      this.connect();
    }, this.reconnectDelay);
  }

  /**
   * Processa dados recebidos do gateway
   */
  async handleIncomingData(line) {
    // Ignorar linhas vazias ou de log do ESP-IDF
    if (!line || 
        line.startsWith('I (') || 
        line.startsWith('W (') || 
        line.startsWith('E (') ||
        line.includes('rst:') ||
        line.includes('mode:') ||
        line.includes('ets ') ||
        line.length < 10) {
      return;
    }

    // Tentar extrair JSON do payload
    try {
      // Gateway pode enviar formato: "║ Dados: {json}"
      let jsonString = line;
      
      // Extrair JSON de linhas formatadas
      const jsonMatch = line.match(/\{.*\}/);
      if (jsonMatch) {
        jsonString = jsonMatch[0];
      }

      // Parse JSON
      const data = JSON.parse(jsonString);

      // Validar estrutura esperada
      // Formato antigo: {mac, type, value}
      // Formato AGUADA-1: {mac, distance_mm, vcc_bat_mv, rssi}
      const isAguada1Format = data.mac && data.distance_mm !== undefined;
      const isOldFormat = data.mac && data.type && data.value !== undefined;
      
      if (!isAguada1Format && !isOldFormat) {
        logger.warn(`[Serial Bridge] JSON recebido mas estrutura inválida:`, data);
        return;
      }

      this.stats.packetsReceived++;
      this.stats.lastPacketTime = new Date();

      if (isAguada1Format) {
        logger.info(`[Serial Bridge] 📡 Telemetria AGUADA-1 recebida:`, {
          mac: data.mac,
          distance_mm: data.distance_mm,
          vcc_bat_mv: data.vcc_bat_mv,
          rssi: data.rssi,
        });
      } else {
        logger.info(`[Serial Bridge] 📡 Telemetria recebida:`, {
          mac: data.mac,
          type: data.type,
          value: data.value,
        });
      }

      // Enviar ao backend
      await this.sendToBackend(data);

    } catch (error) {
      // Não é JSON ou erro no parse - ignorar silenciosamente
      // (evita spam de logs do ESP-IDF)
      return;
    }
  }

  /**
   * Envia telemetria ao backend via HTTP POST
   */
  async sendToBackend(data) {
    try {
      const response = await fetch(this.backendUrl, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify(data),
        timeout: 5000,
      });

      if (response.ok) {
        this.stats.packetsSent++;
        const result = await response.json();
        logger.info(`[Serial Bridge] ✅ Enviado ao backend (${result.sensor_id || 'unknown'})`);
      } else {
        this.stats.errors++;
        logger.error(`[Serial Bridge] ❌ Backend retornou status ${response.status}`);
      }

    } catch (error) {
      this.stats.errors++;
      logger.error(`[Serial Bridge] ❌ Erro ao enviar ao backend:`, error.message);
    }
  }

  /**
   * Desconecta da porta serial
   */
  disconnect() {
    if (this.port && this.port.isOpen) {
      logger.info(`[Serial Bridge] Desconectando...`);
      this.port.close();
      this.isConnected = false;
    }
  }

  /**
   * Retorna estatísticas
   */
  getStats() {
    return {
      ...this.stats,
      uptime: Math.floor((Date.now() - this.stats.startTime) / 1000),
      isConnected: this.isConnected,
      portPath: this.portPath,
    };
  }

  /**
   * Log de status periódico
   */
  startStatusLogger(interval = 60000) {
    setInterval(() => {
      if (this.isConnected) {
        logger.info(`[Serial Bridge] Status:`, this.getStats());
      }
    }, interval);
  }
}

export default SerialBridge;
