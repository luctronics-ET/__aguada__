import express from 'express';
import telemetryController from '../controllers/telemetry.controller.js';

const router = express.Router();

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

/**
 * GET /api/health
 * Health check
 */
router.get('/health', (req, res) => {
  res.status(200).json({
    status: 'ok',
    timestamp: new Date().toISOString(),
    service: 'aguada-backend',
  });
});

export default router;
