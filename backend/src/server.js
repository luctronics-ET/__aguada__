import express from 'express';
import helmet from 'helmet';
import cors from 'cors';
import rateLimit from 'express-rate-limit';
import dotenv from 'dotenv';
import path from 'path';
import { fileURLToPath } from 'url';
import apiRoutes from './routes/api.routes.js';
import { testConnection } from './config/database.js';
import { connectRedis } from './config/redis.js';
import metricsMiddleware from './middleware/metrics.middleware.js';
import logger from './config/logger.js';

dotenv.config();

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const PROJECT_ROOT = path.resolve(__dirname, '../..');

const app = express();
const PORT = process.env.PORT || 3000;

// =============================================================================
// MIDDLEWARES
// =============================================================================

// Security headers
app.use(helmet());

// CORS
app.use(cors({
  origin: process.env.CORS_ORIGIN || '*',
  credentials: true,
}));

// JSON parser
app.use(express.json({ limit: '10mb' }));
app.use(express.urlencoded({ extended: true }));

// Rate limiting
const limiter = rateLimit({
  windowMs: parseInt(process.env.RATE_LIMIT_WINDOW_MS || '60000'),
  max: parseInt(process.env.RATE_LIMIT_MAX_REQUESTS || '60'),
  message: 'Muitas requisições. Tente novamente em breve.',
  standardHeaders: true,
  legacyHeaders: false,
});

app.use('/api/', limiter);

// Metrics middleware (deve vir antes do logger para capturar todas as requisições)
app.use(metricsMiddleware);

// Request logger
app.use((req, res, next) => {
  logger.info(`${req.method} ${req.path}`, {
    ip: req.ip,
    userAgent: req.get('user-agent'),
  });
  next();
});

// =============================================================================
// ROUTES
// =============================================================================

// Servir arquivos estáticos do frontend (antes das rotas de API)
const frontendPath = path.join(PROJECT_ROOT, 'frontend');
app.use(express.static(frontendPath));

// Rotas de API
app.use('/api', apiRoutes);

// API info endpoint (mantido para compatibilidade)
app.get('/api/info', (req, res) => {
  res.json({
    service: 'AGUADA Backend API',
    version: '1.0.0',
    status: 'running',
    endpoints: {
      telemetry: 'POST /api/telemetry',
      manual_reading: 'POST /api/manual-reading',
      calibration: 'POST /api/calibration',
      health: 'GET /api/health',
    },
  });
});

// 404 handler para rotas de API
app.use('/api/*', (req, res) => {
  res.status(404).json({
    success: false,
    error: 'Endpoint não encontrado',
    path: req.path,
  });
});

// SPA fallback - todas as rotas não-API vão para index.html
app.get('*', (req, res) => {
  // Servir index.html para rotas do frontend
  res.sendFile(path.join(frontendPath, 'index.html'));
});

// Error handler
app.use((err, req, res, next) => {
  logger.error('Erro não tratado:', err);
  res.status(500).json({
    success: false,
    error: 'Erro interno do servidor',
    message: process.env.NODE_ENV === 'development' ? err.message : undefined,
  });
});

// =============================================================================
// STARTUP
// =============================================================================

import http from 'http';
import { initWebSocket } from './websocket/wsHandler.js';
import SerialBridge from './services/serial-bridge.js';
import { initializeQueue, getReadingsWorker } from './services/queue.service.js';
import alertService from './services/alert.service.js';

async function startServer() {
  try {
    // Test database connection
    const dbOk = await testConnection();
    if (!dbOk) {
      throw new Error('Falha ao conectar ao banco de dados');
    }
    
    // Connect to Redis
    await connectRedis();
    
    // Inicializar queue worker (pode falhar silenciosamente se Redis não estiver disponível)
    await initializeQueue();
    
    // Iniciar sistema de alertas
    alertService.startMonitoring(60000); // Verificar a cada 1 minuto
    logger.info('✅ Sistema de alertas iniciado');
    
    // Create HTTP server
    const server = http.createServer(app);

    // Initialize WebSocket
    initWebSocket(server);
    
    // Initialize Serial Bridge (Gateway USB)
    const serialBridge = new SerialBridge({
      portPath: process.env.SERIAL_PORT || '/dev/ttyACM0',
      baudRate: parseInt(process.env.SERIAL_BAUD || '115200'),
      backendUrl: `http://localhost:${PORT}/api/telemetry`,
      autoReconnect: true,
    });
    
    serialBridge.connect();
    serialBridge.startStatusLogger(60000); // Log status a cada 1 min
    
    // Exportar serialBridge para uso em outros módulos (ex: health check)
    global.serialBridge = serialBridge;
    
    // Start HTTP server
    server.listen(PORT, () => {
      logger.info(`🚀 Servidor rodando na porta ${PORT}`);
      logger.info(`📊 Ambiente: ${process.env.NODE_ENV || 'development'}`);
      logger.info(`🔗 API: http://localhost:${PORT}/api`);
      logger.info(`🔌 WebSocket: ws://localhost:${PORT}/ws`);
      logger.info(`📡 Serial Bridge: ${serialBridge.portPath} @ ${serialBridge.baudRate} baud`);
    });
    
  } catch (error) {
    logger.error('❌ Falha ao iniciar servidor:', error);
    process.exit(1);
  }
}

// Handle graceful shutdown
async function gracefulShutdown() {
  logger.info('Encerrando servidor graciosamente...');
  
  // Parar sistema de alertas
  alertService.stopMonitoring();
  logger.info('Sistema de alertas encerrado');
  
  // Fechar worker da fila
  const readingsWorker = getReadingsWorker();
  if (readingsWorker) {
    await readingsWorker.close();
    logger.info('Queue worker encerrado');
  }
  
  process.exit(0);
}

process.on('SIGTERM', gracefulShutdown);
process.on('SIGINT', gracefulShutdown);

// Start
startServer();

export default app;
