# 🚀 AGUADA - Guia Completo de Setup do Zero

**Sistema de Telemetria de Sensores via Gateway ESP32**

Este guia vai te ajudar a montar o ambiente de desenvolvimento completo do sistema AGUADA do zero.

---

## 📋 Índice

1. [Pré-requisitos](#pré-requisitos)
2. [Estrutura do Sistema](#estrutura-do-sistema)
3. [Instalação do Banco de Dados](#instalação-do-banco-de-dados)
4. [Instalação do Backend](#instalação-do-backend)
5. [Instalação do Frontend](#instalação-do-frontend)
6. [Configuração do Gateway ESP32](#configuração-do-gateway-esp32)
7. [Testes e Validação](#testes-e-validação)
8. [Troubleshooting](#troubleshooting)

---

## 🔧 Pré-requisitos

### Software Necessário

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y \
  nodejs npm \
  postgresql postgresql-contrib \
  git curl \
  build-essential \
  python3 python3-pip

# Node.js 18+ (se não tiver)
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt install -y nodejs

# Verificar versões
node --version  # Deve ser 18+
npm --version
psql --version  # Deve ser 12+
```

### Hardware Necessário

- **ESP32-C3** (Gateway) - 1 unidade
- **ESP32-C3** (Sensores) - 2-5 unidades
- **Sensores Ultrassônicos** AJ-SR04M - 1 por sensor
- **Cabo USB** para gateway
- **Fonte 5V** para sensores

---

## 🏗️ Estrutura do Sistema

```
┌─────────────┐
│  Sensor     │ ESP-NOW (wireless)
│  ESP32-C3   │──────┐
└─────────────┘      │
                     │
┌─────────────┐      │     ┌──────────────┐     ┌────────────┐
│  Sensor     │ ESP-NOW   │   Gateway    │ USB │  Backend   │
│  ESP32-C3   │──────────→│   ESP32-C3   │────→│  Node.js   │
└─────────────┘           └──────────────┘     └────────────┘
                                                      │
                                                      ↓
                                              ┌────────────┐
                                              │ PostgreSQL │
                                              │ TimescaleDB│
                                              └────────────┘
                                                      │
                                                      ↓
                                              ┌────────────┐
                                              │  Frontend │
                                              │   React   │
                                              └────────────┘
```

**Fluxo de Dados:**
1. Sensores leem ultrassônico a cada 30s
2. Enviam via ESP-NOW para Gateway
3. Gateway recebe e envia via Serial USB para Backend
4. Backend processa e salva no PostgreSQL
5. Frontend consulta API e exibe dados

---

## 🗄️ Instalação do Banco de Dados

### 1. Instalar PostgreSQL + TimescaleDB

```bash
# Instalar PostgreSQL
sudo apt install -y postgresql postgresql-contrib

# Instalar TimescaleDB
sudo sh -c "echo 'deb https://packagecloud.io/timescale/timescaledb/ubuntu/ $(lsb_release -c -s) main' > /etc/apt/sources.list.d/timescaledb.list"
wget --quiet -O - https://packagecloud.io/timescale/timescaledb/gpgkey | sudo apt-key add -
sudo apt update
sudo apt install -y timescaledb-2-postgresql-15

# Configurar TimescaleDB
sudo timescaledb-tune
sudo systemctl restart postgresql
```

### 2. Criar Banco de Dados

```bash
# Entrar como postgres
sudo -u postgres psql

# No psql, executar:
CREATE USER aguada_user WITH PASSWORD 'aguada_pass_2025';
CREATE DATABASE aguada_db OWNER aguada_user;
\c aguada_db
CREATE SCHEMA aguada;
GRANT ALL PRIVILEGES ON SCHEMA aguada TO aguada_user;
\q
```

### 3. Criar Tabelas

```bash
# Copiar schema
cd /home/luciano/Área\ de\ trabalho/aguada

# Executar schema
sudo -u postgres psql -d aguada_db -f database/schema.sql

# Ou se não tiver schema.sql, criar manualmente:
sudo -u postgres psql -d aguada_db << 'EOF'
-- Criar schema
CREATE SCHEMA IF NOT EXISTS aguada;

-- Tabela de sensores
CREATE TABLE IF NOT EXISTS aguada.sensores (
    sensor_id VARCHAR(50) PRIMARY KEY,
    elemento_id VARCHAR(50) NOT NULL,
    node_mac VARCHAR(17),
    variavel VARCHAR(50) NOT NULL,
    tipo VARCHAR(50),
    status VARCHAR(20) DEFAULT 'ativo',
    ajuste_offset NUMERIC(10,2) DEFAULT 0,
    created_at TIMESTAMP DEFAULT NOW()
);

-- Tabela de leituras raw
CREATE TABLE IF NOT EXISTS aguada.leituras_raw (
    leitura_id SERIAL PRIMARY KEY,
    sensor_id VARCHAR(50) NOT NULL,
    elemento_id VARCHAR(50) NOT NULL,
    variavel VARCHAR(50) NOT NULL,
    valor NUMERIC(10,2) NOT NULL,
    unidade VARCHAR(10) DEFAULT 'cm',
    datetime TIMESTAMP DEFAULT NOW(),
    meta JSONB,
    fonte VARCHAR(20) DEFAULT 'sensor',
    autor VARCHAR(50),
    modo VARCHAR(20) DEFAULT 'automatica',
    observacao TEXT,
    FOREIGN KEY (sensor_id) REFERENCES aguada.sensores(sensor_id)
);

-- Criar índice para consultas rápidas
CREATE INDEX IF NOT EXISTS idx_leituras_datetime ON aguada.leituras_raw(datetime DESC);
CREATE INDEX IF NOT EXISTS idx_leituras_sensor ON aguada.leituras_raw(sensor_id, datetime DESC);

-- Habilitar TimescaleDB (se instalado)
SELECT create_hypertable('aguada.leituras_raw', 'datetime', if_not_exists => TRUE);

-- Conceder permissões
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA aguada TO aguada_user;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA aguada TO aguada_user;
EOF
```

### 4. Inserir Dados de Teste (Opcional)

```bash
sudo -u postgres psql -d aguada_db << 'EOF'
-- Inserir sensores de exemplo
INSERT INTO aguada.sensores (sensor_id, elemento_id, node_mac, variavel, tipo) VALUES
('SEN_CON_01', 'RCON', '20:6e:f1:6b:77:58', 'distance_cm', 'ultrassonico'),
('SEN_CAV_01', 'RCAV', 'dc:06:75:67:6a:cc', 'distance_cm', 'ultrassonico')
ON CONFLICT (sensor_id) DO NOTHING;
EOF
```

### 5. Testar Conexão

```bash
# Testar conexão
psql -h localhost -U aguada_user -d aguada_db -c "SELECT COUNT(*) FROM aguada.sensores;"

# Se pedir senha, usar: aguada_pass_2025
```

---

## 🖥️ Instalação do Backend

### 1. Preparar Diretório

```bash
cd /home/luciano/Área\ de\ trabalho/aguada/backend

# Limpar node_modules se existir
rm -rf node_modules package-lock.json
```

### 2. Criar package.json Simplificado

```bash
cat > package.json << 'EOF'
{
  "name": "aguada-backend",
  "version": "1.0.0",
  "type": "module",
  "description": "Backend AGUADA - Sistema de Telemetria",
  "main": "src/server.js",
  "scripts": {
    "start": "node src/server.js",
    "dev": "node --watch src/server.js"
  },
  "dependencies": {
    "express": "^4.18.2",
    "cors": "^2.8.5",
    "dotenv": "^16.3.1",
    "pg": "^8.11.3",
    "serialport": "^12.0.0",
    "@serialport/parser-readline": "^12.0.0",
    "ws": "^8.14.2",
    "node-fetch": "^3.3.2"
  }
}
EOF
```

### 3. Instalar Dependências

```bash
npm install
```

### 4. Criar Arquivo .env

```bash
cat > .env << 'EOF'
# Servidor
PORT=3000
NODE_ENV=development

# Banco de Dados
DB_HOST=localhost
DB_PORT=5432
DB_USER=aguada_user
DB_PASSWORD=aguada_pass_2025
DB_NAME=aguada_db

# Serial (Gateway USB)
SERIAL_PORT=/dev/ttyACM0
SERIAL_BAUD=115200

# CORS
CORS_ORIGIN=*
EOF
```

### 5. Criar Estrutura Mínima do Backend

```bash
# Criar diretórios
mkdir -p src/{config,controllers,services,routes,websocket}

# Criar arquivos essenciais
```

---

## 📝 Criar Backend Mínimo Funcional

Vou criar os arquivos essenciais do backend de forma simplificada:

### 1. Configuração do Banco

```bash
cat > src/config/database.js << 'EOF'
import pkg from 'pg';
const { Pool } = pkg;
import dotenv from 'dotenv';

dotenv.config();

const pool = new Pool({
  host: process.env.DB_HOST || 'localhost',
  port: parseInt(process.env.DB_PORT || '5432'),
  user: process.env.DB_USER || 'aguada_user',
  password: process.env.DB_PASSWORD || 'aguada_pass_2025',
  database: process.env.DB_NAME || 'aguada_db',
  max: 20,
  idleTimeoutMillis: 30000,
  connectionTimeoutMillis: 2000,
});

export async function testConnection() {
  try {
    await pool.query('SELECT 1');
    console.log('✅ Banco de dados conectado');
    return true;
  } catch (error) {
    console.error('❌ Erro ao conectar ao banco:', error.message);
    return false;
  }
}

export default pool;
EOF
```

### 2. Logger Simples

```bash
cat > src/config/logger.js << 'EOF'
const log = (level, message, data = {}) => {
  const timestamp = new Date().toISOString();
  const dataStr = Object.keys(data).length ? JSON.stringify(data) : '';
  console.log(`[${timestamp}] [${level}]: ${message} ${dataStr}`);
};

export default {
  info: (msg, data) => log('info', msg, data),
  error: (msg, data) => log('error', msg, data),
  warn: (msg, data) => log('warn', msg, data),
  debug: (msg, data) => log('debug', msg, data),
};
EOF
```

### 3. Serial Bridge Simplificado

```bash
cat > src/services/serial-bridge.js << 'EOF'
import { SerialPort } from 'serialport';
import { ReadlineParser } from '@serialport/parser-readline';
import logger from '../config/logger.js';
import fetch from 'node-fetch';

class SerialBridge {
  constructor(config = {}) {
    this.port = null;
    this.parser = null;
    this.isConnected = false;
    this.portPath = config.portPath || '/dev/ttyACM0';
    this.baudRate = config.baudRate || 115200;
    this.backendUrl = config.backendUrl || 'http://localhost:3000/api/telemetry';
    this.stats = {
      packetsReceived: 0,
      packetsSent: 0,
      errors: 0,
    };
  }

  connect() {
    try {
      logger.info(`[Serial Bridge] Conectando a ${this.portPath}...`);
      
      this.port = new SerialPort({
        path: this.portPath,
        baudRate: this.baudRate,
        autoOpen: false,
      });

      this.parser = this.port.pipe(new ReadlineParser({ delimiter: '\n' }));

      this.port.on('open', () => {
        this.isConnected = true;
        logger.info(`[Serial Bridge] ✅ Conectado`);
      });

      this.parser.on('data', (line) => {
        this.handleData(line.trim());
      });

      this.port.on('error', (err) => {
        this.stats.errors++;
        logger.error(`[Serial Bridge] Erro:`, err.message);
      });

      this.port.open((err) => {
        if (err) {
          logger.error(`[Serial Bridge] Erro ao abrir:`, err.message);
        }
      });
    } catch (error) {
      logger.error(`[Serial Bridge] Erro:`, error.message);
    }
  }

  async handleData(line) {
    if (!line || line.length < 10) return;

    try {
      const jsonMatch = line.match(/\{.*\}/);
      if (!jsonMatch) return;

      const data = JSON.parse(jsonMatch[0]);
      
      if (!data.mac || !data.type || data.value === undefined) return;

      this.stats.packetsReceived++;
      logger.info(`[Serial Bridge] 📡 Dados recebidos:`, {
        mac: data.mac,
        type: data.type,
        value: data.value,
      });

      await this.sendToBackend(data);
    } catch (error) {
      // Ignorar erros de parse (logs do ESP-IDF)
    }
  }

  async sendToBackend(data) {
    try {
      const response = await fetch(this.backendUrl, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data),
        timeout: 5000,
      });

      if (response.ok) {
        this.stats.packetsSent++;
        logger.info(`[Serial Bridge] ✅ Enviado ao backend`);
      }
    } catch (error) {
      this.stats.errors++;
      logger.error(`[Serial Bridge] ❌ Erro ao enviar:`, error.message);
    }
  }

  getStats() {
    return {
      ...this.stats,
      isConnected: this.isConnected,
      portPath: this.portPath,
    };
  }
}

export default SerialBridge;
EOF
```

### 4. Controller de Telemetria

```bash
cat > src/controllers/telemetry.controller.js << 'EOF'
import pool from '../config/database.js';
import logger from '../config/logger.js';

export async function receiveTelemetry(req, res) {
  try {
    const { mac, type, value, battery, rssi, uptime } = req.body;

    if (!mac || !type || value === undefined) {
      return res.status(400).json({
        success: false,
        error: 'Campos obrigatórios: mac, type, value',
      });
    }

    // Identificar sensor pelo MAC
    const sensorResult = await pool.query(
      `SELECT sensor_id, elemento_id FROM aguada.sensores 
       WHERE node_mac = $1 AND variavel = $2 LIMIT 1`,
      [mac, type]
    );

    if (sensorResult.rows.length === 0) {
      logger.warn(`Sensor não encontrado: MAC=${mac}, type=${type}`);
      return res.status(404).json({
        success: false,
        error: 'Sensor não registrado',
      });
    }

    const { sensor_id, elemento_id } = sensorResult.rows[0];

    // Converter valor (distance_cm vem multiplicado por 100)
    const valorReal = type === 'distance_cm' ? value / 100.0 : value;

    // Inserir leitura
    await pool.query(
      `INSERT INTO aguada.leituras_raw 
       (sensor_id, elemento_id, variavel, valor, unidade, datetime, meta, fonte, autor, modo)
       VALUES ($1, $2, $3, $4, $5, NOW(), $6, 'sensor', $7, 'automatica')`,
      [
        sensor_id,
        elemento_id,
        type,
        valorReal,
        type === 'distance_cm' ? 'cm' : 'boolean',
        JSON.stringify({ battery, rssi, uptime, raw_value: value }),
        mac,
      ]
    );

    logger.info('Telemetria recebida', { sensor_id, type, valor: valorReal });

    res.status(200).json({
      success: true,
      sensor_id,
      type,
      value: valorReal,
    });
  } catch (error) {
    logger.error('Erro ao receber telemetria:', error);
    res.status(500).json({
      success: false,
      error: 'Erro interno do servidor',
    });
  }
}
EOF
```

### 5. Controller de Leituras

```bash
cat > src/controllers/reading.controller.js << 'EOF'
import pool from '../config/database.js';

export async function getLatestReadings(req, res) {
  try {
    const result = await pool.query(`
      SELECT DISTINCT ON (sensor_id)
        sensor_id,
        elemento_id,
        variavel,
        valor,
        unidade,
        datetime
      FROM aguada.leituras_raw
      ORDER BY sensor_id, datetime DESC
    `);

    const readings = {};
    result.rows.forEach(row => {
      if (!readings[row.sensor_id]) {
        readings[row.sensor_id] = {
          sensor_id: row.sensor_id,
          elemento_id: row.elemento_id,
          variables: {},
        };
      }
      readings[row.sensor_id].variables[row.variavel] = {
        valor: row.valor.toString(),
        unidade: row.unidade,
        datetime: row.datetime.toISOString(),
      };
    });

    res.json({
      success: true,
      data: readings,
      timestamp: new Date().toISOString(),
    });
  } catch (error) {
    console.error('Erro ao buscar leituras:', error);
    res.status(500).json({
      success: false,
      error: 'Erro ao buscar leituras',
    });
  }
}
EOF
```

### 6. Rotas

```bash
cat > src/routes/api.routes.js << 'EOF'
import express from 'express';
import { receiveTelemetry } from '../controllers/telemetry.controller.js';
import { getLatestReadings } from '../controllers/reading.controller.js';

const router = express.Router();

router.post('/telemetry', receiveTelemetry);
router.get('/readings/latest', getLatestReadings);

router.get('/health', (req, res) => {
  res.json({
    status: 'ok',
    timestamp: new Date().toISOString(),
    service: 'aguada-backend',
  });
});

export default router;
EOF
```

### 7. Servidor Principal

```bash
cat > src/server.js << 'EOF'
import express from 'express';
import cors from 'cors';
import dotenv from 'dotenv';
import apiRoutes from './routes/api.routes.js';
import { testConnection } from './config/database.js';
import logger from './config/logger.js';
import SerialBridge from './services/serial-bridge.js';
import http from 'http';
import { WebSocketServer } from 'ws';

dotenv.config();

const app = express();
const PORT = process.env.PORT || 3000;

// Middlewares
app.use(cors());
app.use(express.json());

// Rotas
app.use('/api', apiRoutes);

// Health check
app.get('/', (req, res) => {
  res.json({
    service: 'AGUADA Backend',
    version: '1.0.0',
    status: 'running',
  });
});

// Criar servidor HTTP
const server = http.createServer(app);

// WebSocket
const wss = new WebSocketServer({ server, path: '/ws' });
wss.on('connection', (ws) => {
  logger.info('[WebSocket] Cliente conectado');
  ws.send(JSON.stringify({ type: 'connected', message: 'Conectado ao AGUADA' }));
});

// Serial Bridge
const serialBridge = new SerialBridge({
  portPath: process.env.SERIAL_PORT || '/dev/ttyACM0',
  baudRate: parseInt(process.env.SERIAL_BAUD || '115200'),
  backendUrl: `http://localhost:${PORT}/api/telemetry`,
});

// Iniciar servidor
async function start() {
  try {
    // Testar banco
    const dbOk = await testConnection();
    if (!dbOk) {
      logger.error('❌ Banco de dados não conectado');
      process.exit(1);
    }

    // Conectar Serial Bridge
    serialBridge.connect();

    // Iniciar servidor
    server.listen(PORT, () => {
      logger.info(`🚀 Servidor rodando na porta ${PORT}`);
      logger.info(`🔗 API: http://localhost:${PORT}/api`);
      logger.info(`🔌 WebSocket: ws://localhost:${PORT}/ws`);
      logger.info(`📡 Serial Bridge: ${serialBridge.portPath}`);
    });
  } catch (error) {
    logger.error('❌ Erro ao iniciar:', error);
    process.exit(1);
  }
}

start();
EOF
```

### 8. Testar Backend

```bash
# Iniciar backend
npm start

# Em outro terminal, testar:
curl http://localhost:3000/api/health
curl http://localhost:3000/api/readings/latest
```

---

## 🎨 Instalação do Frontend

### Opção 1: Frontend HTML Simples (Recomendado para começar)

```bash
cd /home/luciano/Área\ de\ trabalho/aguada

# Criar frontend simples
mkdir -p frontend-simple
cd frontend-simple

cat > index.html << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <title>AGUADA - Dashboard</title>
    <meta charset="UTF-8">
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: Arial, sans-serif;
            background: #f5f5f5;
            padding: 20px;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
        }
        h1 { color: #333; margin-bottom: 20px; }
        .sensors {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
        }
        .sensor-card {
            background: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        .sensor-card h3 { color: #2196F3; margin-bottom: 10px; }
        .sensor-card .value {
            font-size: 32px;
            font-weight: bold;
            color: #4CAF50;
            margin: 10px 0;
        }
        .status { padding: 5px 10px; border-radius: 4px; display: inline-block; }
        .status.online { background: #4CAF50; color: white; }
        .status.offline { background: #f44336; color: white; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🌊 AGUADA - Monitoramento de Reservatórios</h1>
        <div id="status">Carregando...</div>
        <div class="sensors" id="sensors"></div>
    </div>

    <script>
        const API_URL = 'http://localhost:3000/api';

        async function loadReadings() {
            try {
                const response = await fetch(`${API_URL}/readings/latest`);
                const data = await response.json();
                
                if (data.success) {
                    displaySensors(data.data);
                    document.getElementById('status').innerHTML = 
                        `<span class="status online">✅ Conectado</span> - Última atualização: ${new Date().toLocaleTimeString()}`;
                }
            } catch (error) {
                document.getElementById('status').innerHTML = 
                    `<span class="status offline">❌ Erro ao conectar</span>`;
                console.error('Erro:', error);
            }
        }

        function displaySensors(sensors) {
            const container = document.getElementById('sensors');
            container.innerHTML = '';

            Object.keys(sensors).forEach(sensorId => {
                const sensor = sensors[sensorId];
                const distance = sensor.variables?.distance_cm;
                
                const card = document.createElement('div');
                card.className = 'sensor-card';
                card.innerHTML = `
                    <h3>${sensor.elemento_id || sensorId}</h3>
                    <div class="value">${distance ? distance.valor + ' ' + distance.unidade : 'N/A'}</div>
                    <div>Sensor: ${sensorId}</div>
                    <div>Última leitura: ${distance ? new Date(distance.datetime).toLocaleString() : 'N/A'}</div>
                `;
                container.appendChild(card);
            });
        }

        // Carregar a cada 5 segundos
        loadReadings();
        setInterval(loadReadings, 5000);
    </script>
</body>
</html>
EOF

# Servir com Python
python3 -m http.server 8080

# Acessar: http://localhost:8080
```

### Opção 2: Frontend React (Avançado)

```bash
cd /home/luciano/Área\ de\ trabalho/aguada/frontend-react

# Se não tiver node_modules, instalar
npm install

# Criar .env
echo "VITE_API_URL=http://localhost:3000/api" > .env

# Iniciar
npm run dev
```

---

## 📡 Configuração do Gateway ESP32

### 1. Instalar ESP-IDF

```bash
# Seguir guia oficial: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/
# Ou usar Arduino IDE (mais simples)
```

### 2. Firmware do Gateway (Arduino IDE)

```cpp
// gateway_basic.ino
#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>

// MAC do gateway (verificar no Serial Monitor)
uint8_t gatewayMac[] = {0x80, 0xF1, 0xB2, 0x50, 0x2E, 0xC4};

void setup() {
  Serial.begin(115200);
  
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Erro ao inicializar ESP-NOW");
    return;
  }
  
  esp_now_register_recv_cb(onDataRecv);
  Serial.println("Gateway ESP-NOW pronto");
}

void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  char jsonStr[250];
  memcpy(jsonStr, incomingData, len);
  jsonStr[len] = '\0';
  
  // Enviar via Serial para Backend
  Serial.println(jsonStr);
}

void loop() {
  delay(100);
}
```

### 3. Flash no ESP32

```bash
# Usar Arduino IDE ou ESP-IDF
# Porta: /dev/ttyACM0
# Baud: 115200
```

---

## ✅ Testes e Validação

### 1. Testar Backend

```bash
# Terminal 1: Iniciar backend
cd backend
npm start

# Terminal 2: Testar endpoints
curl http://localhost:3000/api/health
curl http://localhost:3000/api/readings/latest

# Testar envio de telemetria
curl -X POST http://localhost:3000/api/telemetry \
  -H "Content-Type: application/json" \
  -d '{
    "mac": "20:6e:f1:6b:77:58",
    "type": "distance_cm",
    "value": 24480,
    "battery": 5000,
    "uptime": 120,
    "rssi": -50
  }'
```

### 2. Verificar Banco de Dados

```bash
psql -h localhost -U aguada_user -d aguada_db -c "
SELECT sensor_id, valor, datetime 
FROM aguada.leituras_raw 
ORDER BY datetime DESC 
LIMIT 10;
"
```

### 3. Testar Frontend

```bash
# Abrir navegador
http://localhost:8080  # Frontend HTML simples
# ou
http://localhost:3001  # Frontend React
```

---

## 🐛 Troubleshooting

### Backend não inicia

```bash
# Verificar se porta está em uso
sudo lsof -i :3000

# Verificar logs
cd backend
npm start 2>&1 | tee logs.txt
```

### Serial Bridge não conecta

```bash
# Verificar permissões
ls -la /dev/ttyACM0
sudo chmod 666 /dev/ttyACM0
# ou
sudo usermod -a -G dialout $USER
# (fazer logout/login)

# Verificar se dispositivo existe
dmesg | grep tty
```

### Banco de dados não conecta

```bash
# Verificar se PostgreSQL está rodando
sudo systemctl status postgresql

# Verificar conexão
psql -h localhost -U aguada_user -d aguada_db

# Verificar senha no .env
cat backend/.env
```

### Frontend não carrega dados

```bash
# Verificar CORS no backend
# Verificar console do navegador (F12)
# Verificar se API está respondendo
curl http://localhost:3000/api/readings/latest
```

---

## 📚 Próximos Passos

1. ✅ Sistema básico funcionando
2. Adicionar mais sensores
3. Implementar alertas
4. Adicionar gráficos históricos
5. Implementar autenticação
6. Deploy em produção

---

## 📞 Suporte

Se encontrar problemas:
1. Verificar logs do backend
2. Verificar logs do PostgreSQL
3. Verificar console do navegador (F12)
4. Verificar Serial Monitor do ESP32

**Boa sorte! 🚀**

