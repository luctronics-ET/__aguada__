import express from 'express';
import telemetryController from '../controllers/telemetry.controller.js';
import readingController from '../controllers/reading.controller.js';

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
 * GET /api/sensors/status
 * Status de todos os sensores (online/offline)
 */
router.get('/sensors/status', readingController.getSensorsStatus);

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
