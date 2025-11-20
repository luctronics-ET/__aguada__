import express from 'express';
import telemetryController from '../controllers/telemetry.controller.js';
import readingController from '../controllers/reading.controller.js';
import sensorsController from '../controllers/sensors.controller.js';
import alertsController from '../controllers/alerts.controller.js';
import statsController from '../controllers/stats.controller.js';
import systemController from '../controllers/system.controller.js';
import gatewayController from '../controllers/gateway.controller.js';
import exportService from '../services/export.service.js';

const router = express.Router();

// ============================================================================
// TELEMETRY (POST)
// ============================================================================

/**
 * POST /api/telemetry
 * Recebe telemetria dos nodes ESP32 via MQTT/HTTP
 */
router.post('/telemetry', telemetryController.receiveTelemetry);

/**
 * POST /api/manual-reading
 * Registra leitura manual feita por operador
 */
router.post('/manual-reading', telemetryController.receiveManualReading);

/**
 * POST /api/calibration
 * Registra calibração de sensor
 */
router.post('/calibration', telemetryController.receiveCalibration);

// ============================================================================
// READINGS (GET)
// ============================================================================

/**
 * GET /api/readings/latest
 * Obter últimas leituras de todos os sensores
 */
router.get('/readings/latest', readingController.getLatestReadings);

/**
 * GET /api/readings/raw
 * Obter leituras raw com paginação
 */
router.get('/readings/raw', readingController.getRawReadings);

/**
 * GET /api/readings/daily-summary
 * Resumo diário (min, max, média)
 */
router.get('/readings/daily-summary', readingController.getDailySummary);

/**
 * GET /api/readings/history/:sensor_id
 * Histórico de leituras de um sensor
 */
router.get('/readings/history/:sensor_id', readingController.getReadingHistory);

/**
 * GET /api/readings/export
 * Export readings to CSV
 */
router.get('/readings/export', exportService.exportReadings);

// ============================================================================
// SENSORS
// ============================================================================

/**
 * GET /api/sensors
 * Get all sensors
 */
router.get('/sensors', sensorsController.getAllSensors);

/**
 * GET /api/sensors/status
 * Status de todos os sensores (online/offline)
 */
router.get('/sensors/status', sensorsController.getSensorsStatus);

/**
 * GET /api/sensors/:sensor_id
 * Get sensor by ID
 */
router.get('/sensors/:sensor_id', sensorsController.getSensorById);

/**
 * PUT /api/sensors/:sensor_id
 * Update sensor configuration
 */
router.put('/sensors/:sensor_id', sensorsController.updateSensor);

// ============================================================================
// ALERTS
// ============================================================================

/**
 * GET /api/alerts
 * Get all alerts with filters
 */
router.get('/alerts', alertsController.getAlerts);

/**
 * GET /api/alerts/summary
 * Get alerts summary
 */
router.get('/alerts/summary', alertsController.getAlertsSummary);

/**
 * POST /api/alerts
 * Create new alert
 */
router.post('/alerts', alertsController.createAlert);

/**
 * PUT /api/alerts/:alert_id/resolve
 * Mark alert as resolved
 */
router.put('/alerts/:alert_id/resolve', alertsController.resolveAlert);

/**
 * GET /api/alerts/export
 * Export alerts to CSV
 */
router.get('/alerts/export', exportService.exportAlerts);

// ============================================================================
// STATISTICS
// ============================================================================

/**
 * GET /api/stats/daily
 * Get daily statistics
 */
router.get('/stats/daily', statsController.getDailyStats);

/**
 * GET /api/stats/consumption
 * Get consumption statistics
 */
router.get('/stats/consumption', statsController.getConsumptionStats);

/**
 * GET /api/stats/sensors
 * Get sensors statistics
 */
router.get('/stats/sensors', statsController.getSensorsStats);

/**
 * GET /api/stats/events
 * Get events statistics
 */
router.get('/stats/events', statsController.getEventsStats);

// ============================================================================
// SYSTEM
// ============================================================================

/**
 * GET /api/system/health
 * System health check
 */
router.get('/system/health', systemController.getSystemHealth);

/**
 * GET /api/system/logs
 * Get system logs
 */
router.get('/system/logs', systemController.getSystemLogs);

/**
 * GET /api/system/metrics
 * Get system performance metrics
 */
router.get('/system/metrics', systemController.getSystemMetrics);

/**
 * GET /api/system/alerts
 * Get current system alerts
 */
router.get('/system/alerts', systemController.getSystemAlerts);

/**
 * POST /api/system/restart
 * Restart system (admin only)
 */
router.post('/system/restart', systemController.restartSystem);

// ============================================================================
// GATEWAY
// ============================================================================

/**
 * POST /api/gateway/metrics
 * Recebe métricas do gateway
 */
router.post('/gateway/metrics', gatewayController.receiveGatewayMetrics);

/**
 * GET /api/gateway/metrics
 * Obtém métricas de todos os gateways
 */
router.get('/gateway/metrics', gatewayController.getGatewayMetrics);

// ============================================================================
// HEALTH
// ============================================================================

/**
 * GET /api/health
 * Health check
 */
router.get('/health', (req, res) => {
  res.status(200).json({
    status: 'ok',
    timestamp: new Date().toISOString(),
    service: 'aguada-backend',
    version: '1.0.0',
  });
});

export default router;
