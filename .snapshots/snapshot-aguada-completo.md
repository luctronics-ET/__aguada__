# AGUADA - Snapshot Completo do Projeto

> Gerado em: 2025-12-05
> Sistema de Monitoramento Hidráulico IoT

---

## 📁 Estrutura do Projeto

```
aguada/
├── backend/                    # Backend Node.js/Express
│   ├── src/
│   │   ├── server.js          # Servidor principal
│   │   ├── config/            # Database, Redis, Logger
│   │   ├── controllers/       # Controladores REST
│   │   ├── routes/            # Rotas API
│   │   ├── services/          # Lógica de negócio
│   │   ├── middleware/        # Middlewares
│   │   └── utils/             # Utilitários
│   └── package.json
├── config/                     # Configurações JSON
│   ├── reservoirs.json        # Definição dos reservatórios
│   ├── sensors.json           # Mapeamento de sensores
│   └── thresholds.json        # Limites e alertas
├── database/                   # PostgreSQL/TimescaleDB
│   └── schema.sql             # DDL completo
├── firmware/                   # ESP32-C3 (ESP-IDF 5.x)
│   ├── node_sensor_10/        # Firmware node único (RCON, RCAV, RB03)
│   ├── node_sensor_20/        # Firmware node duplo (IE01 + IE02)
│   └── gateway_esp_idf/       # Gateway ESP-NOW → MQTT
├── frontend/                   # Frontend HTML/JS
│   ├── index.html             # Dashboard principal
│   ├── painel.html            # Painel de controle
│   └── assets/                # CSS, JS, imagens
├── docs/                       # Documentação
│   ├── RULES.md               # Regras e padrões (FONTE DE VERDADE)
│   └── SETUP.md               # Guia de instalação
└── docker-compose.yml          # Orquestração Docker
```

---

## 📋 Visão Geral do Sistema

AGUADA é um sistema supervisório IoT para monitoramento e gestão de redes hídricas:

- **5 reservatórios** monitorados (RCON, RCAV, RB03, IE01, IE02)
- **Casa de Bombas N03** com reservatório intermediário
- **2 bombas de recalque** (B03E elétrica, B03D diesel)
- **Válvulas de controle** (entrada, saída, manobra)
- **Sensores ultrassônicos** AJ-SR04M para medição de nível
- **Nodes ESP32-C3 SuperMini** para telemetria
- **Comunicação ESP-NOW** sensor → gateway (até 250m)
- **Gateway ESP32-C3** converte ESP-NOW → MQTT
- **Backend PostgreSQL/TimescaleDB** para persistência
- **Dashboard Web** para visualização

---

## 🏗️ Arquitetura

```
┌─────────────┐
│  ESP32-C3   │──ESP-NOW──┐
│ + AJ-SR04M  │           │
│  (Sensor)   │           │
└─────────────┘           │     ┌──────────────┐      ┌────────────┐     ┌────────────┐
                          ├────→│   Gateway    │─────→│  Backend   │────→│ PostgreSQL │
┌─────────────┐           │     │   ESP32-C3   │ MQTT │  Node.js   │     │ TimescaleDB│
│  ESP32-C3   │──ESP-NOW──┤     └──────────────┘      └────────────┘     └────────────┘
│ + AJ-SR04M  │           │                                  │                    │
│  (Sensor)   │           │                                  ↓                    ↓
┌─────────────┐           │                          ┌──────────────┐     ┌────────────┐
│  ESP32-C3   │──ESP-NOW──┘                          │   Frontend   │←────│   Redis    │
│ + AJ-SR04M  │                                      │   Web/PWA    │     │   Queue    │
│  (Sensor)   │                                      └──────────────┘     └────────────┘
└─────────────┘
```

---

## 🗄️ Reservatórios

| ID   | Nome                | Tipo       | Capacidade | Altura | Diâmetro/Dimensões |
| ---- | ------------------- | ---------- | ---------- | ------ | ------------------ |
| RCON | Castelo de Consumo  | Cilíndrico | 80 m³      | 400 cm | Ø 510 cm           |
| RCAV | Castelo de Incêndio | Cilíndrico | 80 m³      | 400 cm | Ø 510 cm           |
| RB03 | Reservatório CB N03 | Cilíndrico | 80 m³      | 400 cm | Ø 510 cm           |
| IE01 | Cisterna IE 1       | Retangular | 254 m³     | 240 cm | 1040 × 407 cm      |
| IE02 | Cisterna IE 2       | Retangular | 254 m³     | 240 cm | 1040 × 407 cm      |

---

## 📡 Tipos de Firmware

### TYPE_SINGLE_ULTRA (node_sensor_10)

- **Reservatórios**: RCON, RCAV, RB03
- **Sensores**: 1 ultrassônico por ESP32
- **GPIOs**: TRIG=1, ECHO=0, VALVE_IN=2, VALVE_OUT=3, SOUND=5, LED=8
- **Variáveis**: `distance_cm`, `valve_in`, `valve_out`, `sound_in`

### TYPE_DUAL_ULTRA (node_sensor_20)

- **Reservatórios**: IE01 + IE02 (1 ESP32 para ambos)
- **Sensores**: 2 ultrassônicos no mesmo ESP32
- **Variáveis**: `IE01_distance_cm`, `IE02_distance_cm`, válvulas e som prefixados

---

## 📦 Arquivos Principais

### backend/src/server.js

```javascript
import express from "express";
import helmet from "helmet";
import cors from "cors";
import rateLimit from "express-rate-limit";
import dotenv from "dotenv";
import path from "path";
import { fileURLToPath } from "url";
import apiRoutes from "./routes/api.routes.js";
import { testConnection } from "./config/database.js";
import { connectRedis } from "./config/redis.js";
import metricsMiddleware from "./middleware/metrics.middleware.js";
import logger from "./config/logger.js";

dotenv.config();

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const PROJECT_ROOT = path.resolve(__dirname, "../..");

const app = express();
const PORT = process.env.PORT || 3000;

// =============================================================================
// MIDDLEWARES
// =============================================================================

// Security headers
app.use(helmet());

// CORS
app.use(
  cors({
    origin: process.env.CORS_ORIGIN || "*",
    credentials: true,
  })
);

// JSON parser
app.use(express.json({ limit: "10mb" }));
app.use(express.urlencoded({ extended: true }));

// Rate limiting
const limiter = rateLimit({
  windowMs: parseInt(process.env.RATE_LIMIT_WINDOW_MS || "60000"),
  max: parseInt(process.env.RATE_LIMIT_MAX_REQUESTS || "60"),
  message: "Muitas requisições. Tente novamente em breve.",
  standardHeaders: true,
  legacyHeaders: false,
});

app.use("/api/", limiter);

// Metrics middleware
app.use(metricsMiddleware);

// Request logger
app.use((req, res, next) => {
  logger.info(`${req.method} ${req.path}`, {
    ip: req.ip,
    userAgent: req.get("user-agent"),
  });
  next();
});

// =============================================================================
// ROUTES
// =============================================================================

// Servir arquivos estáticos do frontend
const frontendPath = path.join(PROJECT_ROOT, "frontend");
app.use(express.static(frontendPath));

// Rotas de API
app.use("/api", apiRoutes);
```

---

### backend/src/routes/api.routes.js

```javascript
import express from "express";
import telemetryController from "../controllers/telemetry.controller.js";
import readingController from "../controllers/reading.controller.js";
import sensorsController from "../controllers/sensors.controller.js";
import alertsController from "../controllers/alerts.controller.js";
import statsController from "../controllers/stats.controller.js";
import systemController from "../controllers/system.controller.js";
import gatewayController from "../controllers/gateway.controller.js";
import databaseController from "../controllers/database.controller.js";
import exportService from "../services/export.service.js";

const router = express.Router();

// ============================================================================
// TELEMETRY (POST)
// ============================================================================

router.post("/telemetry", telemetryController.receiveTelemetry);
router.post("/manual-reading", telemetryController.receiveManualReading);
router.post("/calibration", telemetryController.receiveCalibration);

// ============================================================================
// READINGS (GET)
// ============================================================================

router.get("/readings/latest", readingController.getLatestReadings);
router.get("/readings/raw", readingController.getRawReadings);
router.get("/readings/daily-summary", readingController.getDailySummary);
router.get("/readings/history/:sensor_id", readingController.getReadingHistory);
router.get("/readings/export", exportService.exportReadings);

// ============================================================================
// SENSORS
// ============================================================================

router.get("/sensors", sensorsController.getAllSensors);
router.get("/sensors/status", sensorsController.getSensorsStatus);
router.get("/sensors/:sensor_id", sensorsController.getSensorById);
router.put("/sensors/:sensor_id", sensorsController.updateSensor);

// ============================================================================
// ALERTS
// ============================================================================

router.get("/alerts", alertsController.getAlerts);
router.get("/alerts/summary", alertsController.getAlertsSummary);
router.post("/alerts", alertsController.createAlert);
router.put("/alerts/:alert_id/resolve", alertsController.resolveAlert);

// ============================================================================
// STATISTICS
// ============================================================================

router.get("/stats/daily", statsController.getDailyStats);
router.get("/stats/consumption", statsController.getConsumptionStats);
router.get("/stats/sensors", statsController.getSensorsStats);

export default router;
```

---

### database/schema.sql

```sql
-- AGUADA - Schema PostgreSQL/TimescaleDB

-- Extensões
CREATE EXTENSION IF NOT EXISTS timescaledb;

-- Schema
CREATE SCHEMA IF NOT EXISTS aguada;
SET search_path = aguada, public;

-- Elementos hidráulicos (reservatórios, bombas, válvulas)
CREATE TABLE elementos (
  elemento_id VARCHAR(50) PRIMARY KEY,
  tipo VARCHAR(20) NOT NULL,  -- 'reservatorio', 'bomba', 'valvula', 'rede'
  nome VARCHAR(100) NOT NULL,
  descricao TEXT,
  coordenadas JSONB,
  parametros JSONB,
  status VARCHAR(20) DEFAULT 'ativo',
  criado_em TIMESTAMPTZ DEFAULT NOW(),
  atualizado_em TIMESTAMPTZ DEFAULT NOW()
);

-- Sensores
CREATE TABLE sensores (
  sensor_id VARCHAR(50) PRIMARY KEY,
  elemento_id VARCHAR(50) REFERENCES elementos(elemento_id),
  node_mac VARCHAR(17) NOT NULL,
  tipo VARCHAR(20) NOT NULL,  -- 'ultrassonico', 'pressao', 'vazao'
  modelo VARCHAR(50),
  variavel VARCHAR(20) NOT NULL,  -- 'nivel_cm', 'pressao_bar'
  unidade VARCHAR(10),
  gpio_config JSONB,
  precisao NUMERIC(5,2),
  range_min NUMERIC(10,2),
  range_max NUMERIC(10,2),
  frequencia_leitura_sec INTEGER DEFAULT 10,
  ultima_calibracao TIMESTAMPTZ,
  ajuste_offset NUMERIC(10,2) DEFAULT 0,
  status VARCHAR(20) DEFAULT 'ativo',
  criado_em TIMESTAMPTZ DEFAULT NOW()
);

-- Leituras brutas (hypertable TimescaleDB)
CREATE TABLE leituras_raw (
  leitura_id BIGSERIAL,
  sensor_id VARCHAR(50) NOT NULL REFERENCES sensores(sensor_id),
  elemento_id VARCHAR(50) NOT NULL REFERENCES elementos(elemento_id),
  variavel VARCHAR(20) NOT NULL,
  valor NUMERIC(10,2) NOT NULL,
  unidade VARCHAR(10),
  meta JSONB,
  fonte VARCHAR(20) NOT NULL,  -- 'sensor', 'usuario', 'sistema'
  autor VARCHAR(100),
  modo VARCHAR(20),
  observacao TEXT,
  datetime TIMESTAMPTZ NOT NULL,
  processed BOOLEAN DEFAULT FALSE,
  criado_em TIMESTAMPTZ DEFAULT NOW()
);

SELECT create_hypertable('leituras_raw', 'datetime', if_not_exists => TRUE);

-- Índices
CREATE INDEX idx_leituras_raw_sensor_datetime ON leituras_raw(sensor_id, datetime DESC);
CREATE INDEX idx_leituras_raw_elemento_datetime ON leituras_raw(elemento_id, datetime DESC);
CREATE INDEX idx_leituras_raw_processed ON leituras_raw(processed) WHERE NOT processed;
```

---

### firmware/node_sensor_10/main/main.c

```c
/**
 * AGUADA - Firmware Universal para Sensor Nodes
 *
 * Hardware: ESP32-C3 SuperMini + AJ-SR04M
 * Protocolo: ESP-NOW → Gateway → MQTT
 *
 * Recursos por Node:
 * - 1 sensor ultrassônico (distance_cm)
 * - 2 válvulas digitais (valve_in, valve_out)
 * - 1 detector de som (sound_in - água entrando)
 * - RSSI, Battery, Uptime
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_now.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "config.h"

static const char *TAG = "AGUADA_NODE";

// Variáveis globais
static uint8_t node_mac[6];
static char node_mac_str[18];

// Últimos valores conhecidos
static int last_distance_cm = -1;
static uint8_t last_valve_in = 255;
static uint8_t last_valve_out = 255;
static uint8_t last_sound_in = 255;

// Inicialização GPIO
void init_gpio(void) {
    // Sensor ultrassônico
    gpio_reset_pin(TRIG_PIN);
    gpio_set_direction(TRIG_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(TRIG_PIN, 0);

    gpio_reset_pin(ECHO_PIN);
    gpio_set_direction(ECHO_PIN, GPIO_MODE_INPUT);

    // Válvulas (INPUT - apenas leitura de estado)
    gpio_reset_pin(VALVE_IN_PIN);
    gpio_set_direction(VALVE_IN_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(VALVE_IN_PIN, GPIO_PULLDOWN_ONLY);

    gpio_reset_pin(VALVE_OUT_PIN);
    gpio_set_direction(VALVE_OUT_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(VALVE_OUT_PIN, GPIO_PULLDOWN_ONLY);

    // Detector de som (INPUT)
    gpio_reset_pin(SOUND_IN_PIN);
    gpio_set_direction(SOUND_IN_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SOUND_IN_PIN, GPIO_PULLDOWN_ONLY);

    // LED Status
    gpio_reset_pin(LED_STATUS);
    gpio_set_direction(LED_STATUS, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_STATUS, 0);
}

// Leitura sensor ultrassônico
int read_ultrasonic_distance(void) {
    // Enviar pulso TRIG (10us)
    gpio_set_level(TRIG_PIN, 0);
    esp_rom_delay_us(2);
    gpio_set_level(TRIG_PIN, 1);
    esp_rom_delay_us(10);
    gpio_set_level(TRIG_PIN, 0);

    // Aguardar ECHO subir
    int64_t timeout = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 0) {
        if ((esp_timer_get_time() - timeout) > TIMEOUT_US) {
            return -1;  // Timeout
        }
    }

    // Medir duração do pulso ECHO
    int64_t start = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 1) {
        if ((esp_timer_get_time() - start) > TIMEOUT_US) {
            return -1;
        }
    }
    int64_t duration = esp_timer_get_time() - start;

    // Calcular distância (cm * 100)
    int distance_cm_x100 = (int)((duration * 343) / 200);

    // Validar range
    if (distance_cm_x100 < (MIN_DISTANCE_CM * 100) || distance_cm_x100 > (MAX_DISTANCE_CM * 100)) {
        return -2;  // Fora de range
    }

    return distance_cm_x100;
}

// Filtro de mediana
int read_distance_filtered(void) {
    int samples[SAMPLES_FOR_MEDIAN];
    int valid_samples = 0;

    for (int i = 0; i < SAMPLES_FOR_MEDIAN; i++) {
        int dist = read_ultrasonic_distance();
        if (dist > 0) {
            samples[valid_samples++] = dist;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    if (valid_samples == 0) return -1;

    // Ordenar e retornar mediana
    // ... (bubble sort)
    return samples[valid_samples / 2];
}
```

---

### config/reservoirs.json

```json
{
  "reservoirs": [
    {
      "id": "RCON",
      "nome": "Castelo de Consumo",
      "alias": "CON",
      "tipo": "cilindrico",
      "capacidade_l": 80000,
      "dimensoes": {
        "altura_cm": 400,
        "diametro_cm": 510,
        "hsensor_cm": 40
      },
      "niveis": {
        "critico_percent": 10,
        "alerta_percent": 20,
        "normal_min_percent": 20,
        "normal_max_percent": 95
      }
    },
    {
      "id": "RCAV",
      "nome": "Castelo de Incêndio",
      "alias": "CAV",
      "tipo": "cilindrico",
      "capacidade_l": 80000,
      "dimensoes": {
        "altura_cm": 400,
        "diametro_cm": 510,
        "hsensor_cm": 20
      },
      "niveis": {
        "critico_percent": 70,
        "alerta_percent": 75,
        "normal_min_percent": 75,
        "normal_max_percent": 95
      }
    },
    {
      "id": "RB03",
      "nome": "Reservatório Casa de Bombas N03",
      "alias": "B03",
      "tipo": "cilindrico",
      "capacidade_l": 80000,
      "dimensoes": {
        "altura_cm": 400,
        "diametro_cm": 510
      }
    },
    {
      "id": "IE01",
      "nome": "Cisterna Ilha do Engenho 1",
      "tipo": "retangular",
      "capacidade_l": 254000,
      "dimensoes": {
        "altura_cm": 240,
        "comprimento_cm": 1040,
        "largura_cm": 407
      }
    },
    {
      "id": "IE02",
      "nome": "Cisterna Ilha do Engenho 2",
      "tipo": "retangular",
      "capacidade_l": 254000,
      "dimensoes": {
        "altura_cm": 240,
        "comprimento_cm": 1040,
        "largura_cm": 407
      }
    }
  ]
}
```

---

## 📡 Protocolo de Transmissão

### Formato JSON (ESP32 → Gateway → Backend)

```json
{
  "mac": "20:6E:F1:6B:77:58",
  "type": "distance_cm",
  "value": 24480,
  "battery": 5000,
  "uptime": 3600,
  "rssi": -50
}
```

### Regras de Transmissão

- **distance_cm**: Envia quando mudança > ±2 cm (deadband)
- **valve_in/valve_out/sound_in**: Envia em qualquer mudança de estado (0↔1)
- **Heartbeat**: A cada 30 segundos (envia últimos valores mesmo sem mudança)
- **Valores inteiros**: distance_cm multiplicado por 100 (244.8 cm → 24480)

---

## 🔧 Comandos Úteis

### Backend

```bash
cd backend
npm install
npm run dev          # Desenvolvimento
npm start            # Produção
```

### Firmware

```bash
cd firmware/node_sensor_10
idf.py set-target esp32c3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

### Docker

```bash
docker-compose up -d              # Iniciar todos os serviços
docker-compose logs -f backend    # Ver logs do backend
```

### Database

```bash
PGPASSWORD=aguada_pass_2025 psql -U aguada_user -d aguada_db -h localhost -p 5433
```

---

## 📊 API Endpoints

| Método | Endpoint                  | Descrição                |
| ------ | ------------------------- | ------------------------ |
| POST   | /api/telemetry            | Receber telemetria ESP32 |
| GET    | /api/readings/latest      | Últimas leituras         |
| GET    | /api/readings/history/:id | Histórico de um sensor   |
| GET    | /api/sensors              | Listar sensores          |
| GET    | /api/sensors/status       | Status online/offline    |
| GET    | /api/alerts               | Listar alertas           |
| POST   | /api/alerts               | Criar alerta             |
| GET    | /api/stats/daily          | Estatísticas diárias     |
| GET    | /api/stats/consumption    | Análise de consumo       |

---

## 🎯 Regras Importantes (docs/RULES.md)

1. **Firmware universal**: Mesmo binário para todos os nós do mesmo tipo
2. **Transmissão individual**: Cada variável em JSON separado
3. **Valores inteiros**: Multiplicar floats por 100
4. **GPIOs fixos**: Nunca mudar pinos (TRIG=1, ECHO=0, etc.)
5. **ESP-IDF 5.x**: Usar assinaturas de callback corretas
6. **Deadband 2cm**: Só transmitir mudanças significativas
7. **Heartbeat 30s**: Manter conexão viva

---

_Snapshot gerado para contexto de IA - AGUADA v2.0_

---

.snapshots/snapshot-aguada-completo.md
