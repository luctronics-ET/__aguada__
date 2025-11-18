import { WebSocketServer } from 'ws';
import logger from '../config/logger.js';

let wss = null;
const clients = new Set();

/**
 * Initialize WebSocket server
 */
export function initWebSocket(server) {
  wss = new WebSocketServer({ 
    server,
    path: '/ws'
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
      client.send(message);
      sent++;
    }
  });

  logger.debug(`[WebSocket] Broadcast to ${sent} clients:`, data.type);
}

/**
 * Broadcast new telemetry reading
 */
export function broadcastReading(reading) {
  broadcast({
    type: 'reading',
    data: reading,
    timestamp: new Date().toISOString()
  });
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
