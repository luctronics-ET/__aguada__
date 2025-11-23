#!/usr/bin/env node
/**
 * Monitor Serial Simples - Testa recepção de dados do gateway
 * Não envia ao backend, apenas exibe no console
 */

import { SerialPort } from 'serialport';
import { ReadlineParser } from '@serialport/parser-readline';

const PORT_PATH = process.env.SERIAL_PORT || '/dev/ttyACM0';
const BAUD_RATE = parseInt(process.env.SERIAL_BAUD || '115200');

console.log('================================================');
console.log('📡 AGUADA - Monitor Serial (Somente Leitura)');
console.log('================================================');
console.log(`Porta: ${PORT_PATH}`);
console.log(`Baud:  ${BAUD_RATE}`);
console.log('');
console.log('Aguardando dados do gateway...');
console.log('Pressione Ctrl+C para sair');
console.log('================================================');
console.log('');

const port = new SerialPort({
  path: PORT_PATH,
  baudRate: BAUD_RATE,
});

const parser = port.pipe(new ReadlineParser({ delimiter: '\n' }));

let packetCount = 0;

parser.on('data', (line) => {
  line = line.trim();
  
  // Tentar extrair JSON
  const jsonMatch = line.match(/\{.*\}/);
  if (jsonMatch) {
    try {
      const data = JSON.parse(jsonMatch[0]);
      
      if (data.mac && data.type && data.value !== undefined) {
        packetCount++;
        
        console.log(`\n[${new Date().toLocaleTimeString()}] Pacote #${packetCount}`);
        console.log('─────────────────────────────────────────────');
        console.log(`MAC:      ${data.mac}`);
        console.log(`Tipo:     ${data.type}`);
        console.log(`Valor:    ${data.value} ${getUnit(data.type)}`);
        console.log(`Bateria:  ${data.battery} mV`);
        console.log(`Uptime:   ${data.uptime}s (${formatUptime(data.uptime)})`);
        console.log(`RSSI:     ${data.rssi} dBm`);
        
        // Converter distance_cm para formato legível
        if (data.type === 'distance_cm') {
          const cm = data.value / 100;
          console.log(`         → ${cm.toFixed(2)} cm`);
        }
        
        console.log('─────────────────────────────────────────────');
      }
    } catch (error) {
      // Não é JSON válido, ignorar
    }
  }
});

port.on('error', (err) => {
  console.error('❌ Erro na porta serial:', err.message);
  process.exit(1);
});

port.on('open', () => {
  console.log('✅ Porta serial aberta com sucesso!');
  console.log('');
});

// Helper functions
function getUnit(type) {
  const units = {
    'distance_cm': 'cm×100',
    'valve_in': '(0/1)',
    'valve_out': '(0/1)',
    'sound_in': '(0/1)',
    'battery': 'mV',
    'rssi': 'dBm',
  };
  return units[type] || '';
}

function formatUptime(seconds) {
  const days = Math.floor(seconds / 86400);
  const hours = Math.floor((seconds % 86400) / 3600);
  const mins = Math.floor((seconds % 3600) / 60);
  const secs = seconds % 60;
  
  const parts = [];
  if (days > 0) parts.push(`${days}d`);
  if (hours > 0) parts.push(`${hours}h`);
  if (mins > 0) parts.push(`${mins}m`);
  parts.push(`${secs}s`);
  
  return parts.join(' ');
}

// Graceful shutdown
process.on('SIGINT', () => {
  console.log('\n\n================================================');
  console.log(`📊 Estatísticas da Sessão`);
  console.log('================================================');
  console.log(`Pacotes recebidos: ${packetCount}`);
  console.log('================================================');
  console.log('\n👋 Encerrando monitor...\n');
  port.close();
  process.exit(0);
});
