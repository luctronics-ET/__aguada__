import logger from '../config/logger.js';
import pool from '../config/database.js';

// Armazenar métricas do gateway em memória (últimas 24h)
const gatewayMetrics = new Map();

/**
 * POST /api/gateway/metrics
 * Recebe métricas do gateway
 */
export async function receiveGatewayMetrics(req, res) {
  try {
    const { mac, metrics } = req.body;

    if (!mac || !metrics) {
      return res.status(400).json({
        success: false,
        error: 'MAC e métricas são obrigatórios',
      });
    }

    // Armazenar métricas
    gatewayMetrics.set(mac, {
      ...metrics,
      lastUpdate: new Date(),
      mac,
    });

    logger.debug('Métricas do gateway recebidas', { mac, metrics });

    return res.status(200).json({
      success: true,
      message: 'Métricas recebidas',
    });
  } catch (error) {
    logger.error('Erro ao receber métricas do gateway:', error);
    return res.status(500).json({
      success: false,
      error: 'Erro ao processar métricas',
    });
  }
}

/**
 * GET /api/gateway/metrics
 * Obtém métricas de todos os gateways
 */
export async function getGatewayMetrics(req, res) {
  try {
    const metrics = Array.from(gatewayMetrics.values())
      .filter(m => {
        // Filtrar métricas antigas (> 5 minutos sem atualização)
        const age = Date.now() - new Date(m.lastUpdate).getTime();
        return age < 5 * 60 * 1000;
      })
      .map(m => ({
        mac: m.mac,
        packets_received: m.packets_received || 0,
        packets_sent: m.packets_sent || 0,
        packets_failed: m.packets_failed || 0,
        packets_dropped: m.packets_dropped || 0,
        http_errors: m.http_errors || 0,
        queue_full_count: m.queue_full_count || 0,
        queue_usage_percent: m.queue_usage_percent || 0,
        wifi_connected: m.wifi_connected || false,
        last_packet_time: m.last_packet_time,
        last_success_time: m.last_success_time,
        lastUpdate: m.lastUpdate,
        uptime_seconds: m.uptime_seconds || 0,
      }));

    return res.status(200).json({
      success: true,
      count: metrics.length,
      gateways: metrics,
      timestamp: new Date().toISOString(),
    });
  } catch (error) {
    logger.error('Erro ao obter métricas do gateway:', error);
    return res.status(500).json({
      success: false,
      error: 'Erro ao obter métricas',
    });
  }
}

export default {
  receiveGatewayMetrics,
  getGatewayMetrics,
};

