import pool from '../config/database.js';
import logger from '../config/logger.js';
import { getClientsCount } from '../websocket/wsHandler.js';
import metricsService from '../services/metrics.service.js';
import alertService from '../services/alert.service.js';
import os from 'os';

/**
 * GET /api/system/health
 * Get system health status
 */
export async function getSystemHealth(req, res) {
  try {
    // Check database
    let dbStatus = 'down';
    let dbLatency = null;
    try {
      const dbStart = Date.now();
      await pool.query('SELECT 1');
      dbLatency = Date.now() - dbStart;
      dbStatus = dbLatency < 100 ? 'healthy' : 'degraded';
    } catch (error) {
      logger.error('Database health check failed:', error);
    }

    // Get database stats
    const dbStats = await pool.query(`
      SELECT 
        (SELECT COUNT(*) FROM aguada.leituras_raw) as total_readings,
        (SELECT COUNT(*) FROM aguada.sensores) as total_sensors,
        (SELECT COUNT(*) FROM aguada.alertas WHERE resolvido = FALSE) as active_alerts,
        (SELECT COUNT(*) FROM aguada.eventos WHERE datetime > NOW() - INTERVAL '24 hours') as recent_events
    `);

    // System info
    const systemInfo = {
      hostname: os.hostname(),
      platform: os.platform(),
      arch: os.arch(),
      cpus: os.cpus().length,
      totalMemory: Math.round(os.totalmem() / 1024 / 1024 / 1024), // GB
      freeMemory: Math.round(os.freemem() / 1024 / 1024 / 1024), // GB
      uptime: Math.round(os.uptime()),
      nodeVersion: process.version,
      processUptime: Math.round(process.uptime())
    };

    // WebSocket stats
    const wsStats = {
      connected_clients: getClientsCount()
    };

    // Serial Bridge stats (se disponível)
    let serialBridgeStats = null;
    if (global.serialBridge) {
      const stats = global.serialBridge.getStats();
      serialBridgeStats = {
        status: stats.isConnected ? 'connected' : 'disconnected',
        port: stats.portPath,
        packets_received: stats.packetsReceived,
        packets_sent: stats.packetsSent,
        errors: stats.errors,
        uptime_sec: stats.uptime,
        last_packet_time: stats.lastPacketTime
      };
    }

    res.status(200).json({
      success: true,
      status: dbStatus === 'healthy' ? 'healthy' : 'degraded',
      timestamp: new Date().toISOString(),
      components: {
        database: {
          status: dbStatus,
          latency_ms: dbLatency
        },
        websocket: {
          status: 'healthy',
          clients: wsStats.connected_clients
        },
        serial_bridge: serialBridgeStats || {
          status: 'not_available'
        },
        api: {
          status: 'healthy'
        }
      },
      stats: dbStats.rows[0],
      system: systemInfo
    });
  } catch (error) {
    logger.error('Error getting system health:', error);
    res.status(500).json({
      success: false,
      status: 'error',
      error: 'Erro ao obter status do sistema'
    });
  }
}

/**
 * GET /api/system/logs
 * Get recent system logs
 */
export async function getSystemLogs(req, res) {
  try {
    const { 
      level = 'info',
      limit = 100,
      offset = 0
    } = req.query;

    // In production, read from actual log files or logging service
    // For now, return mock data
    const logs = [];
    
    res.status(200).json({
      success: true,
      count: logs.length,
      data: logs,
      message: 'Log retrieval from files not implemented yet. Use external logging service.'
    });
  } catch (error) {
    logger.error('Error getting system logs:', error);
    res.status(500).json({
      success: false,
      error: 'Erro ao obter logs do sistema'
    });
  }
}

/**
 * GET /api/system/metrics
 * Get system performance metrics (completo com métricas de aplicação)
 */
export async function getSystemMetrics(req, res) {
  try {
    // Obter métricas de aplicação
    const appMetrics = await metricsService.getMetrics();

    // Métricas de sistema
    const systemMetrics = {
      timestamp: new Date().toISOString(),
      process: {
        pid: process.pid,
        uptime: Math.round(process.uptime()),
        memory: process.memoryUsage(),
        cpu: process.cpuUsage()
      },
      system: {
        hostname: os.hostname(),
        platform: os.platform(),
        arch: os.arch(),
        cpus: os.cpus().length,
        loadavg: os.loadavg(),
        totalMemory: os.totalmem(),
        freeMemory: os.freemem(),
        uptime: os.uptime()
      }
    };

    res.status(200).json({
      success: true,
      data: {
        ...systemMetrics,
        application: appMetrics
      }
    });
  } catch (error) {
    logger.error('Error getting system metrics:', error);
    res.status(500).json({
      success: false,
      error: 'Erro ao obter métricas do sistema'
    });
  }
}

/**
 * GET /api/system/alerts
 * Get current system alerts
 */
export async function getSystemAlerts(req, res) {
  try {
    const { history = false, limit = 50 } = req.query;

    if (history === 'true') {
      // Retornar histórico de alertas
      const alertHistory = alertService.getHistory(parseInt(limit));
      const stats = alertService.getStats();

      res.status(200).json({
        success: true,
        history: alertHistory,
        stats,
        timestamp: new Date().toISOString()
      });
    } else {
      // Retornar alertas atuais
      const alerts = await metricsService.checkAlerts();

      res.status(200).json({
        success: true,
        count: alerts.length,
        alerts,
        timestamp: new Date().toISOString()
      });
    }
  } catch (error) {
    logger.error('Error getting system alerts:', error);
    res.status(500).json({
      success: false,
      error: 'Erro ao obter alertas do sistema'
    });
  }
}

/**
 * POST /api/system/restart
 * Restart application (requires admin privileges)
 */
export async function restartSystem(req, res) {
  try {
    logger.warn('System restart requested');
    
    res.status(200).json({
      success: true,
      message: 'Sistema será reiniciado em 5 segundos'
    });

    // Give time for response to be sent
    setTimeout(() => {
      logger.info('Restarting system...');
      process.exit(0); // Process manager (PM2, systemd) should restart
    }, 5000);
  } catch (error) {
    logger.error('Error restarting system:', error);
    res.status(500).json({
      success: false,
      error: 'Erro ao reiniciar sistema'
    });
  }
}

export default {
  getSystemHealth,
  getSystemLogs,
  getSystemMetrics,
  getSystemAlerts,
  restartSystem,
};
