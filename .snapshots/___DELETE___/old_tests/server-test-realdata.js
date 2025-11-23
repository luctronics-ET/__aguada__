#!/usr/bin/env node
/**
 * AGUADA - Backend Simplificado (Sem Database)
 * Para teste rápido de dados reais via Serial Bridge
 */

import express from 'express';
import cors from 'cors';
import { SerialPort } from 'serialport';
import { ReadlineParser } from '@serialport/parser-readline';
import { WebSocketServer } from 'ws';
import http from 'http';

const app = express();
const PORT = 3000;

// Middleware
app.use(cors());
app.use(express.json());

// In-memory storage
const latestReadings = {};
const readingsHistory = [];

// Sensor MAC mapping
const MAC_TO_SENSOR = {
  '20:6e:f1:6b:77:58': { sensor_id: 'SEN_CON_01', elemento_id: 'RCON', nome: 'Castelo Consumo' },
  'dc:06:75:67:6a:cc': { sensor_id: 'SEN_CAV_01', elemento_id: 'RCAV', nome: 'Castelo Incêndio' },
};

// API Routes
app.get('/', (req, res) => {
  res.json({
    service: 'AGUADA Backend (Modo Teste - Sem Database)',
    status: 'running',
    sensors: Object.keys(latestReadings).length,
    totalReadings: readingsHistory.length,
  });
});

app.get('/api/health', (req, res) => {
  res.json({ status: 'ok', timestamp: new Date().toISOString() });
});

app.get('/api/readings/latest', (req, res) => {
  const formattedData = {};
  
  Object.entries(latestReadings).forEach(([mac, readings]) => {
    const sensor = MAC_TO_SENSOR[mac.toLowerCase()];
    if (!sensor) return;
    
    formattedData[sensor.sensor_id] = {
      sensor_id: sensor.sensor_id,
      elemento_id: sensor.elemento_id,
      nome_sensor: sensor.nome,
      mac_address: mac,
      variables: readings,
    };
  });
  
  res.json({
    success: true,
    data: formattedData,
    timestamp: new Date().toISOString(),
  });
});

app.get('/api/sensors/status', (req, res) => {
  const status = Object.entries(latestReadings).map(([mac, readings]) => {
    const sensor = MAC_TO_SENSOR[mac.toLowerCase()];
    const lastReading = readings.distance_cm || {};
    const lastTime = lastReading.datetime ? new Date(lastReading.datetime) : null;
    const minutesAgo = lastTime ? Math.floor((Date.now() - lastTime) / 60000) : 9999;
    
    return {
      sensor_id: sensor?.sensor_id || mac,
      elemento_id: sensor?.elemento_id || mac,
      mac_address: mac,
      ultima_leitura: lastReading.datetime || null,
      status: minutesAgo < 5 ? 'online' : 'offline',
      minutos_sem_dados: minutesAgo,
    };
  });
  
  res.json({
    success: true,
    data: status,
  });
});

app.post('/api/telemetry', (req, res) => {
  const { mac, type, value, battery, uptime, rssi } = req.body;
  
  if (!mac || !type || value === undefined) {
    return res.status(400).json({ success: false, error: 'Dados inválidos' });
  }
  
  const macLower = mac.toLowerCase();
  const sensor = MAC_TO_SENSOR[macLower];
  
  // Store latest reading
  if (!latestReadings[macLower]) {
    latestReadings[macLower] = {};
  }
  
  latestReadings[macLower][type] = {
    valor: value,
    unidade: type === 'distance_cm' ? 'cm' : type.includes('valve') || type.includes('sound') ? 'bool' : '',
    datetime: new Date().toISOString(),
    battery,
    uptime,
    rssi,
  };
  
  // Store in history
  readingsHistory.push({
    timestamp: new Date().toISOString(),
    mac: macLower,
    sensor_id: sensor?.sensor_id,
    elemento_id: sensor?.elemento_id,
    type,
    value,
    battery,
    uptime,
    rssi,
  });
  
  // Keep only last 1000 readings
  if (readingsHistory.length > 1000) {
    readingsHistory.shift();
  }
  
  console.log(`[${new Date().toLocaleTimeString()}] 📡 ${sensor?.elemento_id || mac}: ${type} = ${value}`);
  
  // Broadcast via WebSocket
  broadcastToClients({
    type: 'telemetry',
    data: {
      elemento_id: sensor?.elemento_id,
      sensor_id: sensor?.sensor_id,
      variavel: type,
      valor: value,
      timestamp: new Date().toISOString(),
    },
  });
  
  res.json({
    success: true,
    sensor_id: sensor?.sensor_id,
    elemento_id: sensor?.elemento_id,
    timestamp: new Date().toISOString(),
  });
});

// WebSocket
const server = http.createServer(app);
const wss = new WebSocketServer({ server, path: '/ws' });

const clients = new Set();

wss.on('connection', (ws) => {
  console.log('🔌 Cliente WebSocket conectado');
  clients.add(ws);
  
  ws.on('close', () => {
    clients.delete(ws);
    console.log('🔌 Cliente WebSocket desconectado');
  });
});

function broadcastToClients(message) {
  const data = JSON.stringify(message);
  clients.forEach((client) => {
    if (client.readyState === 1) { // OPEN
      client.send(data);
    }
  });
}

// Serial Bridge
const SERIAL_PORT = process.env.SERIAL_PORT || '/dev/ttyACM0';
const SERIAL_BAUD = parseInt(process.env.SERIAL_BAUD || '115200');

let serialPort = null;

function connectSerial() {
  try {
    serialPort = new SerialPort({
      path: SERIAL_PORT,
      baudRate: SERIAL_BAUD,
      autoOpen: false,
    });
    
    const parser = serialPort.pipe(new ReadlineParser({ delimiter: '\n' }));
    
    parser.on('data', async (line) => {
      line = line.trim();
      
      const jsonMatch = line.match(/\{.*\}/);
      if (jsonMatch) {
        try {
          const data = JSON.parse(jsonMatch[0]);
          
          if (data.mac && data.type && data.value !== undefined) {
            // Send to API
            await fetch(`http://localhost:${PORT}/api/telemetry`, {
              method: 'POST',
              headers: { 'Content-Type': 'application/json' },
              body: JSON.stringify(data),
            }).catch(() => {});
          }
        } catch (error) {
          // Ignore JSON parse errors
        }
      }
    });
    
    serialPort.on('error', (err) => {
      console.error('❌ Erro serial:', err.message);
    });
    
    serialPort.on('close', () => {
      console.log('⚠️  Porta serial fechada, reconectando em 5s...');
      setTimeout(connectSerial, 5000);
    });
    
    serialPort.open((err) => {
      if (err) {
        console.error('❌ Erro ao abrir porta serial:', err.message);
        setTimeout(connectSerial, 5000);
      } else {
        console.log(`✅ Serial Bridge conectado: ${SERIAL_PORT} @ ${SERIAL_BAUD} baud`);
      }
    });
    
  } catch (error) {
    console.error('❌ Erro ao criar porta serial:', error.message);
    setTimeout(connectSerial, 5000);
  }
}

// Start server
server.listen(PORT, () => {
  console.log('');
  console.log('================================================');
  console.log('🚀 AGUADA Backend Simplificado (Dados Reais)');
  console.log('================================================');
  console.log(`🌐 HTTP:       http://localhost:${PORT}`);
  console.log(`🔌 WebSocket:  ws://localhost:${PORT}/ws`);
  console.log(`📡 Serial:     ${SERIAL_PORT} @ ${SERIAL_BAUD} baud`);
  console.log('================================================');
  console.log('');
  console.log('✅ Servidor iniciado sem database');
  console.log('✅ Dados armazenados em memória (volátil)');
  console.log('');
  console.log('Aguardando telemetria dos sensores...');
  console.log('');
  
  // Connect serial after server is ready
  setTimeout(connectSerial, 1000);
});

// Graceful shutdown
process.on('SIGTERM', () => {
  console.log('Encerrando servidor...');
  if (serialPort) serialPort.close();
  server.close();
  process.exit(0);
});

process.on('SIGINT', () => {
  console.log('\nEncerrando servidor...');
  if (serialPort) serialPort.close();
  server.close();
  process.exit(0);
});
