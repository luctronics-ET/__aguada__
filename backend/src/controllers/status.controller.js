/**
 * Status Controller
 *
 * Endpoints para consultar e configurar status online/offline
 * de sensores e gateway.
 */

import statusService from "../services/status.service.js";
import logger from "../config/logger.js";

/**
 * GET /api/status
 * Obtém resumo geral do sistema
 */
export async function getSystemStatus(req, res) {
  try {
    await statusService.initialize();

    const summary = statusService.getSystemStatusSummary();

    res.status(200).json({
      success: true,
      data: summary,
    });
  } catch (error) {
    logger.error("Erro ao obter status do sistema:", error);
    res.status(500).json({
      success: false,
      error: "Erro ao obter status do sistema",
    });
  }
}

/**
 * GET /api/status/sensors
 * Obtém status de todos os sensores
 */
export async function getSensorsStatus(req, res) {
  try {
    await statusService.initialize();

    const sensors = statusService.getAllSensorStatus();
    const summary = statusService.getSystemStatusSummary().sensors;

    res.status(200).json({
      success: true,
      summary,
      data: sensors,
    });
  } catch (error) {
    logger.error("Erro ao obter status dos sensores:", error);
    res.status(500).json({
      success: false,
      error: "Erro ao obter status dos sensores",
    });
  }
}

/**
 * GET /api/status/sensors/:sensor_id
 * Obtém status de um sensor específico
 */
export async function getSensorStatus(req, res) {
  try {
    await statusService.initialize();

    const { sensor_id } = req.params;
    const status = statusService.getSensorStatus(sensor_id);

    res.status(200).json({
      success: true,
      data: status,
    });
  } catch (error) {
    logger.error("Erro ao obter status do sensor:", error);
    res.status(500).json({
      success: false,
      error: "Erro ao obter status do sensor",
    });
  }
}

/**
 * GET /api/status/gateways
 * Obtém status de todos os gateways
 */
export async function getGatewaysStatus(req, res) {
  try {
    await statusService.initialize();

    const gateways = statusService.getAllGatewayStatus();
    const summary = statusService.getSystemStatusSummary().gateways;

    res.status(200).json({
      success: true,
      summary,
      data: gateways,
    });
  } catch (error) {
    logger.error("Erro ao obter status dos gateways:", error);
    res.status(500).json({
      success: false,
      error: "Erro ao obter status dos gateways",
    });
  }
}

/**
 * GET /api/status/gateways/:gateway_id
 * Obtém status de um gateway específico
 */
export async function getGatewayStatus(req, res) {
  try {
    await statusService.initialize();

    const { gateway_id } = req.params;
    const status = statusService.getGatewayStatus(gateway_id);

    res.status(200).json({
      success: true,
      data: status,
    });
  } catch (error) {
    logger.error("Erro ao obter status do gateway:", error);
    res.status(500).json({
      success: false,
      error: "Erro ao obter status do gateway",
    });
  }
}

/**
 * GET /api/status/config
 * Obtém configurações de timeout
 */
export async function getStatusConfig(req, res) {
  try {
    await statusService.initialize();

    const config = statusService.getConfig();

    res.status(200).json({
      success: true,
      data: config,
    });
  } catch (error) {
    logger.error("Erro ao obter configurações de status:", error);
    res.status(500).json({
      success: false,
      error: "Erro ao obter configurações",
    });
  }
}

/**
 * PUT /api/status/config
 * Atualiza configurações de timeout
 *
 * Body: {
 *   sensorTimeoutSec: 120,      // Timeout para sensor offline
 *   gatewayTimeoutSec: 60,      // Timeout para gateway offline
 *   warningThresholdSec: 60,    // Tempo para status warning
 *   checkIntervalSec: 30        // Intervalo de verificação
 * }
 */
export async function updateStatusConfig(req, res) {
  try {
    await statusService.initialize();

    const {
      sensorTimeoutSec,
      gatewayTimeoutSec,
      warningThresholdSec,
      checkIntervalSec,
    } = req.body;

    // Validar valores
    if (
      sensorTimeoutSec &&
      (sensorTimeoutSec < 10 || sensorTimeoutSec > 86400)
    ) {
      return res.status(400).json({
        success: false,
        error: "sensorTimeoutSec deve estar entre 10 e 86400 segundos",
      });
    }

    if (
      gatewayTimeoutSec &&
      (gatewayTimeoutSec < 10 || gatewayTimeoutSec > 86400)
    ) {
      return res.status(400).json({
        success: false,
        error: "gatewayTimeoutSec deve estar entre 10 e 86400 segundos",
      });
    }

    const newConfig = await statusService.updateConfig({
      sensorTimeoutSec,
      gatewayTimeoutSec,
      warningThresholdSec,
      checkIntervalSec,
    });

    logger.info("Configurações de timeout atualizadas", newConfig);

    res.status(200).json({
      success: true,
      message: "Configurações atualizadas com sucesso",
      data: newConfig,
    });
  } catch (error) {
    logger.error("Erro ao atualizar configurações:", error);
    res.status(500).json({
      success: false,
      error: "Erro ao atualizar configurações",
    });
  }
}

/**
 * DELETE /api/status/sensors/:sensor_id
 * Limpa status de um sensor (reset)
 */
export async function clearSensorStatus(req, res) {
  try {
    await statusService.initialize();

    const { sensor_id } = req.params;
    statusService.clearSensorStatus(sensor_id);

    res.status(200).json({
      success: true,
      message: `Status do sensor ${sensor_id} limpo`,
    });
  } catch (error) {
    logger.error("Erro ao limpar status do sensor:", error);
    res.status(500).json({
      success: false,
      error: "Erro ao limpar status",
    });
  }
}

/**
 * POST /api/status/reset
 * Limpa todos os status (reset geral)
 */
export async function resetAllStatus(req, res) {
  try {
    await statusService.initialize();

    statusService.clearAllStatus();

    logger.info("Todos os status foram resetados");

    res.status(200).json({
      success: true,
      message: "Todos os status foram limpos",
    });
  } catch (error) {
    logger.error("Erro ao resetar status:", error);
    res.status(500).json({
      success: false,
      error: "Erro ao resetar status",
    });
  }
}

/**
 * POST /api/status/register-sensors
 * Registra sensores conhecidos no sistema de status
 */
export async function registerKnownSensors(req, res) {
  try {
    await statusService.initialize();

    // Sensores padrão do sistema AGUADA
    const defaultSensors = [
      {
        sensor_id: "SEN_CON_01",
        mac_address: "20:6e:f1:6b:77:58",
        elemento_id: "RCON",
      },
      {
        sensor_id: "SEN_CAV_01",
        mac_address: "dc:06:75:67:6a:cc",
        elemento_id: "RCAV",
      },
      { sensor_id: "SEN_B03_01", mac_address: null, elemento_id: "RB03" },
      { sensor_id: "SEN_IE01_01", mac_address: null, elemento_id: "IE01" },
      { sensor_id: "SEN_IE02_01", mac_address: null, elemento_id: "IE02" },
    ];

    const sensors = req.body.sensors || defaultSensors;
    statusService.registerKnownSensors(sensors);

    res.status(200).json({
      success: true,
      message: `${sensors.length} sensores registrados`,
      data: statusService.getAllSensorStatus(),
    });
  } catch (error) {
    logger.error("Erro ao registrar sensores:", error);
    res.status(500).json({
      success: false,
      error: "Erro ao registrar sensores",
    });
  }
}

/**
 * POST /api/status/simulate
 * Simula um sensor enviando dados (para testes)
 */
export async function simulateSensor(req, res) {
  try {
    await statusService.initialize();

    const { sensor_id, mac, rssi, battery, value } = req.body;

    if (!sensor_id) {
      return res.status(400).json({
        success: false,
        error: "sensor_id é obrigatório",
      });
    }

    const status = statusService.simulateSensorOnline(sensor_id, {
      mac,
      rssi: rssi || -50,
      battery: battery || 5000,
      value: value || 0,
    });

    res.status(200).json({
      success: true,
      message: `Sensor ${sensor_id} simulado como online`,
      data: status,
    });
  } catch (error) {
    logger.error("Erro ao simular sensor:", error);
    res.status(500).json({
      success: false,
      error: "Erro ao simular sensor",
    });
  }
}

export default {
  getSystemStatus,
  getSensorsStatus,
  getSensorStatus,
  getGatewaysStatus,
  getGatewayStatus,
  getStatusConfig,
  updateStatusConfig,
  clearSensorStatus,
  resetAllStatus,
  registerKnownSensors,
  simulateSensor,
};
