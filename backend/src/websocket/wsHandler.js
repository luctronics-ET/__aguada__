import { WebSocketServer } from 'ws';
import logger from '../config/logger.js';

let wss = null;
const clients = new Set();
let readingsBuffer = [];
let broadcastTimer = null;
const BATCH_INTERVAL_MS = 200; // agrupar leituras rápidas
const MAX_BATCH_SIZE = 10;

/**
 * Initialize WebSocket server
 */
export function initWebSocket(server) {
  wss = new WebSocketServer({ 
    server,
    path: '/ws',
    perMessageDeflate: {
      zlibDeflateOptions: {
        chunkSize: 1024,
        level: 6,
        memLevel: 8,
      },
      clientNoContextTakeover: true,
      serverNoContextTakeover: true,
      serverMaxWindowBits: 10,
      concurrencyLimit: 10,
      threshold: 1024, // compress only >1KB
    },
  });

  wss.on('connection', (ws, req) => {
    const clientId = req.socket.remoteAddress + ':' + req.socket.remotePort;
    logger.info(`[WebSocket] Client connected: ${clientId}`);
    clients.add(ws);

    // Send welcome message
    ws.send(JSON.stringify({
      type: 'connected',
      message: 'Connected to AGUADA WebSocket',
      timestamp: new Date().toISOString()
    }));

    // Handle incoming messages
    ws.on('message', (message) => {
      try {
        const data = JSON.parse(message.toString());
        logger.info(`[WebSocket] Message from ${clientId}:`, data);
        
        // Echo back for now (can add custom handlers)
        ws.send(JSON.stringify({
          type: 'echo',
          data: data,
          timestamp: new Date().toISOString()
        }));
      } catch (error) {
        logger.error('[WebSocket] Error parsing message:', error);
      }
    });

    // Handle errors
    ws.on('error', (error) => {
      logger.error(`[WebSocket] Client error (${clientId}):`, error);
    });

    // Handle disconnect
    ws.on('close', () => {
      logger.info(`[WebSocket] Client disconnected: ${clientId}`);
      clients.delete(ws);
    });

    // Send ping every 30 seconds to keep connection alive
    const pingInterval = setInterval(() => {
      if (ws.readyState === ws.OPEN) {
        ws.ping();
      } else {
        clearInterval(pingInterval);
      }
    }, 30000);
  });

  logger.info('[WebSocket] Server initialized on /ws');
  return wss;
}

/**
 * Broadcast message to all connected clients
 */
export function broadcast(data) {
  if (!wss) {
    logger.warn('[WebSocket] Server not initialized');
    return;
  }

  const message = JSON.stringify(data);
  let sent = 0;

  clients.forEach((client) => {
    if (client.readyState === client.OPEN) {
      client.send(message, { compress: true });
      sent++;
    }
  });

  logger.debug(`[WebSocket] Broadcast to ${sent} clients:`, data.type);
}

/**
 * Broadcast new telemetry reading
 */
export function broadcastReading(reading) {
  readingsBuffer.push(reading);

  const flushBuffer = () => {
    if (readingsBuffer.length === 0) return;

    const batch = readingsBuffer.slice(0, MAX_BATCH_SIZE);
    readingsBuffer = readingsBuffer.slice(batch.length);

    broadcast({
      type: batch.length === 1 ? 'reading' : 'readings_batch',
      data: batch.length === 1 ? batch[0] : batch,
      count: batch.length,
      timestamp: new Date().toISOString()
    });

    if (readingsBuffer.length > 0) {
      broadcastTimer = setTimeout(flushBuffer, BATCH_INTERVAL_MS);
    } else {
      broadcastTimer = null;
    }
  };

  if (!broadcastTimer) {
    broadcastTimer = setTimeout(flushBuffer, BATCH_INTERVAL_MS);
  } else if (readingsBuffer.length >= MAX_BATCH_SIZE) {
    clearTimeout(broadcastTimer);
    broadcastTimer = null;
    flushBuffer();
  }
}

/**
 * Broadcast alert
 */
export function broadcastAlert(alert) {
  broadcast({
    type: 'alert',
    data: alert,
    timestamp: new Date().toISOString()
  });
}

/**
 * Broadcast system status
 */
export function broadcastStatus(status) {
  broadcast({
    type: 'status',
    data: status,
    timestamp: new Date().toISOString()
  });
}

/**
 * Get connected clients count
 */
export function getClientsCount() {
  return clients.size;
}

/**
 * Close WebSocket server
 */
export function closeWebSocket() {
  if (wss) {
    clients.forEach(client => client.close());
    clients.clear();
    wss.close();
    logger.info('[WebSocket] Server closed');
  }
}

export default {
  initWebSocket,
  broadcast,
  broadcastReading,
  broadcastAlert,
  broadcastStatus,
  getClientsCount,
  closeWebSocket
};
