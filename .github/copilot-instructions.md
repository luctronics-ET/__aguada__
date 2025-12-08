# AGUADA - AI Coding Agent Instructions

**⚠️ CRITICAL: Read `docs/RULES.md` FIRST before making ANY code changes!**

## 🎯 Quick Navigation

| Component | Purpose | Tech Stack | Quick Command |
|-----------|---------|-----------|---|
| **Firmware** | ESP32-C3 sensor nodes (4 MCUs) | C, ESP-IDF 5.x | `cd firmware/node_sensor_10 && idf.py build` |
| **Backend** | Node.js/Express API + MQTT listener | Node 18+, Express, PostgreSQL | `cd backend && npm run dev` (port 3000) |
| **Database** | TimescaleDB for time-series telemetry | PostgreSQL 15+ | Connection: `host:5432, user:aguada` |
| **Frontend** | HTML/JS dashboard | Vanilla JS, PWA | `file://frontend/index.html` |
| **Gateway** | ESP32-C3 converts ESP-NOW→MQTT/HTTP | C, ESP-IDF 5.x | `cd firmware/gateway_esp_idf && idf.py flash` |

---

## 🏗️ System Architecture (30-second overview)

```
4 × ESP32-C3 (Sensors) ─ESP-NOW─┐
    ├─ 3× node_sensor_10         ├─→ 1× ESP32-C3    ┌─→ MQTT/HTTP  ┌─→ PostgreSQL/
    │  (RCON, RCAV, RB03)        │    (Gateway)  ─→ │ Backend API  │  TimescaleDB
    └─ 1× node_sensor_20         │                   │ Node.js      │  (Hypertables)
       (IE01+IE02 dual)          │                   └─→ Dashboard  │
                                 └───────────────────────────────────┘
```

**Key data flows:**
1. **Sensors → Gateway**: ESP-NOW broadcast every 30s (distance_cm, valve states, sound)
2. **Gateway → Backend**: HTTP POST to `/api/telemetry` with individual variables (not aggregated!)
3. **Backend → DB**: Inserts into `aguada.leituras_raw` (hypertable), calculates events
4. **DB → Frontend**: API queries via `/api/readings/latest`, `/api/stats/*`

---

## 📋 Project Overview

AGUADA is an IoT hydraulic monitoring system for **5 water reservoirs** using:
- **4 ESP32-C3 SuperMini** sensor nodes (wireless ESP-NOW, no WiFi on sensors)
- **5 Reservoirs**: RCON, RCAV, RB03, IE01, IE02 (different physical sizes)
- **Individual variable transmission** - Each measurement sent as separate JSON (critical!)
- **Data compression** - Only significant changes trigger transmission (deadband ±2cm)
- **Real-time monitoring** - 30-second heartbeat, event detection (leaks, supply, alerts)

**Firmware types:**
- **TYPE_SINGLE_ULTRA** (node_sensor_10): 1 ultrasonic sensor per node (RCON, RCAV, RB03) - 3 nodes total
- **TYPE_DUAL_ULTRA** (node_sensor_20): 2 ultrasonic sensors per node (IE01 + IE02) - 1 node total

---

## 🚨 Golden Rules (MUST follow!)

1. **Read `docs/RULES.md`** before any firmware/protocol changes (source of truth)
2. **Individual variable transmission** - Send each variable separately (see Section 4.2)
   - ✅ `{"type":"distance_cm","value":24480}` 
   - ❌ `{"distance_cm":244.8, "valve_in":1}` 
3. **Integer values only** - No floats in transmission (multiply cm by 100)
   - ✅ `244.8 cm → value:24480`
   - ❌ `value:244.8`
4. **Fixed GPIO pins** - Never change TRIG=1, ECHO=0, VALVE_IN=2, etc. (hardware is fixed)
5. **Universal firmware** - Same binary for all TYPE_SINGLE_ULTRA nodes (MAC-based ID, not hardcoded)
6. **ESP-IDF 5.x patterns** - Don't mix with 6.x signatures (callback: `esp_now_send_cb`)
7. **Deadband logic** - Only send when change > ±2cm or state changes (save bandwidth)
8. **30-second heartbeat** - Send last known values even if unchanged

---

## 🔧 Developer Workflows

### Build & Flash Firmware

```bash
# Build only (verify compilation)
cd firmware/node_sensor_10
idf.py set-target esp32c3  # First time only
idf.py build

# Flash + Monitor (typical workflow)
idf.py -p /dev/ttyACM0 flash monitor

# Clean rebuild after changes
idf.py fullclean && idf.py build
```

### Backend Development

```bash
# Start development server (auto-reload)
cd backend
npm run dev  # Runs on http://localhost:3000

# Test API endpoint
curl -X POST http://localhost:3000/api/telemetry \
  -H "Content-Type: application/json" \
  -d '{"mac":"20:6E:F1:6B:77:58","type":"distance_cm","value":24480}'

# View logs
tail -f backend/logs/*.log

# Check database
psql -h localhost -U aguada -d aguada -c "SELECT * FROM aguada.leituras_raw LIMIT 5;"
```

### Docker Setup (Full Stack)

```bash
# Start all services
docker compose up -d

# Verify services
docker compose ps

# View logs
docker compose logs -f backend
docker compose logs -f postgres

# Stop all
docker compose down
```

### Run Tests

```bash
cd backend
npm test

# Run specific test file
npm test -- sensors.controller.test.js
```

---

## 📊 Data Protocol (Critical Understanding)

### Individual Variable Transmission (RULES.md Section 4.2)

**✅ CORRECT**: Send each variable as separate JSON

```json
// Node sends 4 separate transmissions (distance_cm, valve_in, valve_out, sound_in):
{"mac":"20:6E:F1:6B:77:58","type":"distance_cm","value":24480,"battery":5000,"uptime":3,"rssi":-50}
{"mac":"20:6E:F1:6B:77:58","type":"valve_in","value":1,"battery":5000,"uptime":3,"rssi":-50}
{"mac":"20:6E:F1:6B:77:58","type":"valve_out","value":0,"battery":5000,"uptime":3,"rssi":-50}
{"mac":"20:6E:F1:6B:77:58","type":"sound_in","value":0,"battery":5000,"uptime":3,"rssi":-50}
```

**❌ WRONG**: Don't aggregate variables (backend expects individual transmissions)

```json
{"mac":"20:6E:F1:6B:77:58","distance_cm":24480,"valve_in":1,"valve_out":0}  // DON'T DO THIS
```

### TYPE_DUAL_ULTRA (IE01+IE02) - Use Prefixes

```json
// Same ESP32 sends variables with reservoir prefix:
{"mac":"XX:XX:XX:XX:XX:XX","type":"IE01_distance_cm","value":25480}
{"mac":"XX:XX:XX:XX:XX:XX","type":"IE02_distance_cm","value":18350}
{"mac":"XX:XX:XX:XX:XX:XX","type":"IE01_valve_in","value":1}
{"mac":"XX:XX:XX:XX:XX:XX","type":"IE02_valve_in","value":1}
// ... 8 variables total (2 reservoirs × 4 variables)
```

### Value Encoding Rules

| Variable | Type | Format | Example |
|----------|------|--------|---------|
| `distance_cm` | float→int | × 100 | 244.8 cm → 24480 |
| `valve_in`, `valve_out`, `sound_in` | bool | 0 or 1 | 0 (off) or 1 (on) |
| `battery` | int | mV | 5000 (5V DC source) |
| `uptime` | int | seconds | 3600 (1 hour) |
| `rssi` | int | dBm | -50 (signal strength) |

### Transmission Thresholds (RULES.md Section 4.3)

- **distance_cm**: Change > ±2 cm (deadband) - ignore small variations
- **valve_in/valve_out/sound_in**: Any state change (0↔1) - immediate transmission
- **Heartbeat**: Every 30 seconds - send last known values even if unchanged

**Deadband implementation pattern:**

```c
// Only send if change exceeds threshold
if (abs(new_distance - last_distance) >= DEADBAND_CM * 100) {
    send_telemetry("distance_cm", new_distance);
    last_distance = new_distance;
}

// For digital inputs (valves): send on any change
if (new_valve_state != last_valve_state) {
    send_telemetry("valve_in", new_valve_state);
    last_valve_state = new_valve_state;
}
```

---

## 🔌 API Endpoints (Backend)

### Core Telemetry Endpoints

```bash
POST /api/telemetry
# Receives individual variables from ESP32 nodes
Body: {"mac":"XX:XX:XX:XX:XX:XX","type":"distance_cm","value":24480,...}

POST /api/manual-reading
# Operator manually enters reading (calibration verification)

POST /api/calibration
# Records sensor calibration event
```

### Query Endpoints

```bash
GET /api/readings/latest
# Returns last reading for each sensor

GET /api/readings/raw?limit=100&offset=0
# Paginated raw readings (all transmissions)

GET /api/readings/history/:sensor_id?start=2025-12-01&end=2025-12-05
# Sensor history filtered by date range

GET /api/stats/daily
# Daily consumption/supply statistics

GET /api/sensors
# List all sensors (MAC, type, element_id, status)

GET /api/alerts?status=active
# Active alerts (leak, supply, thresholds)

GET /api/system/health
# System health check (DB, Redis, services)
```

### Backend Architecture Pattern

```
backend/src/
├── server.js                 # Express app, middleware setup
├── routes/api.routes.js      # Route definitions (POST, GET endpoints)
├── controllers/              # Business logic per domain
│   ├── telemetry.controller.js
│   ├── reading.controller.js
│   ├── sensors.controller.js
│   └── alerts.controller.js
├── services/                 # External integrations
│   └── export.service.js
├── middleware/               # Auth, validation, logging
├── schemas/                  # Zod validation
├── config/                   # DB, Redis, logger setup
└── utils/                    # Helper functions
```

---

## 🗄️ Database Schema Key Tables

```sql
-- Telemetry (hypertable - time-series)
aguada.leituras_raw (leitura_id, sensor_id, elemento_id, variavel, valor, datetime)
   └─ Partitioned by datetime (TimescaleDB)

-- Configuration
aguada.sensores (sensor_id, elemento_id, node_mac, tipo, variavel, status)
aguada.elementos (elemento_id, tipo, nome, descricao, parametros, status)

-- Events
aguada.eventos (evento_id, tipo, elemento_id, detalhe, datetime_inicio, datetime_fim)

-- Important: Always use schema prefix in queries!
SELECT * FROM aguada.leituras_raw WHERE sensor_id = 'RCON';  -- ✅ CORRECT
SELECT * FROM leituras_raw WHERE sensor_id = 'RCON';        -- ❌ WRONG
```

---

## 🔍 Understanding Firmware Architecture

### Firmware Types

AGUADA utiliza **2 tipos de firmware** para os nodes ESP32-C3:

#### TYPE_SINGLE_ULTRA (node_sensor_10)

- **Firmware**: `firmware/node_sensor_10/`
- **Reservatórios**: RCON, RCAV, RB03
- **Sensores**: 1 ultrassônico AJ-SR04M por ESP32
- **GPIOs**: TRIG=1, ECHO=0, VALVE_IN=2, VALVE_OUT=3, SOUND=5, LED=8
- **Variáveis enviadas**: `distance_cm`, `valve_in`, `valve_out`, `sound_in`
- **Total**: 3 ESP32-C3 (1 por reservatório)

#### TYPE_DUAL_ULTRA (node_sensor_20)

- **Firmware**: `firmware/node_sensor_20/`
- **Reservatórios**: IE01 + IE02 (cisternas Ilha do Engenho)
- **Sensores**: 2 ultrassônicos AJ-SR04M no mesmo ESP32
- **GPIOs**:
  - IE01: TRIG=0, ECHO=1, VALVE_IN=7, VALVE_OUT=8, SOUND=5
  - IE02: TRIG=2, ECHO=3, VALVE_IN=9, VALVE_OUT=10, SOUND=6
  - LED=8 (compartilhado)
- **Variáveis enviadas**: `IE01_distance_cm`, `IE02_distance_cm`, `IE01_valve_in`, `IE01_valve_out`, `IE02_valve_in`, `IE02_valve_out`, `IE01_sound_in`, `IE02_sound_in`
- **Total**: 1 ESP32-C3 (monitora 2 reservatórios)

**Total de ESP32-C3 no sistema**: 4 microcontroladores

### Universal vs Specific Firmware

**node_sensor_10 (TYPE_SINGLE_ULTRA)** - Universal:

```c
// Firmware identifies itself by MAC address (auto-detected)
esp_efuse_mac_get_default(node_mac);

// Backend maps MAC → reservoir (not firmware's job)
// 20:6E:F1:6B:77:58 → RCON
// DC:06:75:67:6A:CC → RCAV
// TBD → RB03
```

**node_sensor_20 (TYPE_DUAL_ULTRA)** - Specific:

```c
// Envia dados com prefixo do reservatório
send_telemetry("IE01_distance_cm", ie01_distance);
send_telemetry("IE02_distance_cm", ie02_distance);

// Backend extrai IE01 e IE02 do prefixo no "type"
```

**❌ WRONG**: Don't create reservoir-specific firmware for TYPE_SINGLE_ULTRA

```c
// DON'T DO THIS for RCON/RCAV/RB03:
#define NODE_ID "RCON"
#define RESERVOIR_HEIGHT_CM 400
```

### Individual Variable Transmission

**✅ CORRECT (TYPE_SINGLE_ULTRA)**: Send each variable type separately

```json
// Three separate transmissions:
{"mac":"20:6E:F1:6B:77:58","type":"distance_cm","value":24480,"battery":5000,"uptime":3,"rssi":-50}
{"mac":"20:6E:F1:6B:77:58","type":"valve_in","value":1,"battery":5000,"uptime":3,"rssi":-50}
{"mac":"20:6E:F1:6B:77:58","type":"valve_out","value":0,"battery":5000,"uptime":3,"rssi":-50}
```

**✅ CORRECT (TYPE_DUAL_ULTRA)**: Send with reservoir prefix

```json
// IE01 and IE02 variables from same MAC:
{"mac":"XX:XX:XX:XX:XX:XX","type":"IE01_distance_cm","value":25480,"battery":5000,"uptime":3,"rssi":-50}
{"mac":"XX:XX:XX:XX:XX:XX","type":"IE02_distance_cm","value":18350,"battery":5000,"uptime":3,"rssi":-50}
{"mac":"XX:XX:XX:XX:XX:XX","type":"IE01_valve_in","value":1,"battery":5000,"uptime":3,"rssi":-50}
{"mac":"XX:XX:XX:XX:XX:XX","type":"IE02_valve_in","value":1,"battery":5000,"uptime":3,"rssi":-50}
// ... etc (8 variables total)
```

**❌ WRONG**: Don't aggregate variables

```json
// DON'T DO THIS:
{
  "mac": "20:6E:F1:6B:77:58",
  "distance_cm": 24480,
  "valve_in": 1,
  "valve_out": 0
}
```

### Integer-Only Values

**✅ CORRECT**: Multiply floats by 100

```c
// Distance: 244.8 cm → transmit as 24480
int distance_cm_x100 = read_distance() * VALUE_MULTIPLIER;
send_telemetry("distance_cm", distance_cm_x100);
```

**❌ WRONG**: Don't send floats

```c
// DON'T DO THIS:
float distance = 244.8;
send_telemetry("distance_cm", distance);  // Will truncate!
```

### Fixed GPIO Pin Assignments

#### TYPE_SINGLE_ULTRA (node_sensor_10)

**All 3 nodes (RCON, RCAV, RB03) use identical GPIO configuration**:

```c
#define TRIG_PIN GPIO_NUM_1      // Ultrasonic trigger
#define ECHO_PIN GPIO_NUM_0      // Ultrasonic echo
#define VALVE_IN_PIN GPIO_NUM_2  // Input valve state
#define VALVE_OUT_PIN GPIO_NUM_3 // Output valve state
#define SOUND_IN_PIN GPIO_NUM_5  // Water flow detector
#define LED_STATUS GPIO_NUM_8    // Heartbeat LED
```

#### TYPE_DUAL_ULTRA (node_sensor_20)

**One ESP32 monitors IE01 + IE02 with different GPIOs**:

```c
// IE01 GPIOs
#define IE01_TRIG_PIN GPIO_NUM_0
#define IE01_ECHO_PIN GPIO_NUM_1
#define IE01_SOUND_PIN GPIO_NUM_5
#define IE01_VALVE_IN_PIN GPIO_NUM_7
#define IE01_VALVE_OUT_PIN GPIO_NUM_8

// IE02 GPIOs
#define IE02_TRIG_PIN GPIO_NUM_2
#define IE02_ECHO_PIN GPIO_NUM_3
#define IE02_SOUND_PIN GPIO_NUM_6
#define IE02_VALVE_IN_PIN GPIO_NUM_9
#define IE02_VALVE_OUT_PIN GPIO_NUM_10

// Shared LED
#define LED_STATUS GPIO_NUM_8  // Same as IE01_VALVE_OUT (careful!)
```

**❌ NEVER change GPIO pins per device** - this would require different PCBs

---

## ESP-IDF 6.x Specific Patterns

### Framework Version

```bash
ESP-IDF 6.1.0 (Git ff97953b)
Location: /home/luciano/esp/esp-idf/
```

### Build Commands

```bash
cd firmware/node_sensor_10
idf.py set-target esp32c3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

### ESP-NOW Callback Signature (IDF 6.x)

**✅ CORRECT** (IDF 6.x):

```c
void espnow_send_cb(const esp_now_send_info_t *info, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        packets_sent++;
    } else {
        packets_failed++;
    }
}
```

**❌ WRONG** (IDF 5.x - old signature):

```c
// This won't compile in IDF 6.x!
void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
    // ...
}
```

### Component Dependencies

**✅ CORRECT** CMakeLists.txt for IDF 6.x:

```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_event nvs_flash esp_system driver esp_timer esp_driver_gpio
)
# Note: esp_now is part of esp_wifi in IDF 6.x
```

**❌ WRONG**: Don't add `esp_now` separately

```cmake
# DON'T DO THIS in IDF 6.x:
REQUIRES esp_wifi esp_now  # esp_now doesn't exist as separate component!
```

---

## Data Protocol Specifications

### Transmission Rules (from RULES.md Section 4.2)

**When to send:**

- `distance_cm`: Change > ±2 cm (deadband)
- `valve_in`, `valve_out`, `sound_in`: Any state change (0↔1)
- **Heartbeat**: Every 30 seconds (send last known values even if unchanged)

**Value encoding:**

- `distance_cm`: Integer (actual cm × 100)
  - Example: 244.8 cm → 24480
  - Example: 180.5 cm → 18050
- `valve_in`, `valve_out`, `sound_in`: 0 (closed/off) or 1 (open/on)
- `battery`: Integer in mV (5V DC = 5000)
- `uptime`: Integer in seconds since boot
- `rssi`: Integer in dBm (negative, e.g., -50)

### Noise Filtering (from RULES.md Section 4.3)

```c
// Median filter configuration
#define SAMPLES_FOR_MEDIAN 11      // Take 11 samples
#define SAMPLE_INTERVAL_MS 200     // 200ms between samples (total: 2.2 seconds)

// Deadband (ignore small variations)
#define DEADBAND_CM 2              // ±2 cm threshold

// Implementation:
int read_distance_filtered(void) {
    int samples[SAMPLES_FOR_MEDIAN];
    int valid_count = 0;

    for (int i = 0; i < SAMPLES_FOR_MEDIAN; i++) {
        int dist = read_ultrasonic_distance();
        if (dist > 0) {
            samples[valid_count++] = dist;
        }
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
    }

    if (valid_count < 5) return -1;  // Not enough valid samples

    // Sort and return median
    qsort(samples, valid_count, sizeof(int), compare);
    return samples[valid_count / 2];
}
```

---

## Hardware Specifications

### ESP32-C3 SuperMini

- **Architecture**: RISC-V 32-bit @ 160MHz
- **Flash**: 4MB XMC
- **GPIO Voltage**: 3.3V (NOT 5V tolerant!)
- **Connection**: /dev/ttyACM0 (USB-C)
- **Power**: 5V DC input, 3.3V regulated output

### AJ-SR04M Ultrasonic Sensor

- **Model**: Waterproof ultrasonic distance sensor
- **Range**: 20-450 cm
- **Trigger**: 10μs pulse on TRIG_PIN (GPIO 1)
- **Echo**: Pulse width on ECHO_PIN (GPIO 0)
- **Timeout**: 30ms (30,000μs)
- **Speed of sound**: 343 m/s (0.0343 cm/μs)

**Calculation formula:**

```c
// Send 10μs trigger pulse
gpio_set_level(TRIG_PIN, 1);
esp_rom_delay_us(10);
gpio_set_level(TRIG_PIN, 0);

// Measure echo pulse duration (in microseconds)
uint32_t duration_us = pulseIn(ECHO_PIN, HIGH, TIMEOUT_US);

// Calculate distance in cm (multiplied by 100 for integer transmission)
// Formula: distance = (duration × 0.034) / 2
// Simplified: (duration × 343) / 20000 (already includes ×100 multiplier)
int distance_cm_x100 = (int)((duration_us * 343) / 20000);
```

**Error codes:**

- Return `-1`: Timeout (sensor didn't respond) → transmit `0`
- Return `-2`: Out of range (< 20cm or > 450cm) → transmit `1`
- Return `> 0`: Valid distance (already × 100)

### ESP-NOW Configuration

```c
static uint8_t gateway_mac[6] = {0x80, 0xf1, 0xb2, 0x50, 0x2e, 0xc4};
#define ESPNOW_CHANNEL 1           // 2.4GHz channel
// No encryption (for simplicity and speed)
// Queue size: 6 messages
// Retries: 3 attempts with 1s delay
```

---

## Network Configuration

### WiFi (Gateway Only)

```c
#define WIFI_SSID "YOUR_SSID"          // Configure in menuconfig or secrets.h
#define WIFI_PASS "YOUR_PASSWORD"      // Never commit real credentials!
// Gateway IP: 192.168.0.124 (DHCP assigned, stable)
```

> ⚠️ **SECURITY**: Credenciais reais devem estar em `secrets.h` (não versionado) ou configuradas via `idf.py menuconfig`

### Backend Endpoints

```bash
# HTTP POST
POST http://192.168.0.100:3000/api/telemetry
Content-Type: application/json

# MQTT
mqtt://192.168.0.100:1883
Topic: aguada/telemetry/{mac_address}/{variable_type}
QoS: 1 (at least once)
```

---

## 📝 Common Development Patterns

### Adding New Endpoint (Backend)

1. **Create controller method** in `backend/src/controllers/your.controller.js`:
```javascript
export const yourMethod = async (req, res) => {
  // Validate input with Zod
  const data = requestSchema.parse(req.body);
  
  // Query database with schema prefix
  const result = await db.query('SELECT * FROM aguada.sensores WHERE ...');
  
  // Return JSON
  res.json({ success: true, data: result });
};
```

2. **Register route** in `backend/src/routes/api.routes.js`:
```javascript
router.post('/your-endpoint', yourController.yourMethod);
```

3. **Test with curl**:
```bash
curl -X POST http://localhost:3000/api/your-endpoint \
  -H "Content-Type: application/json" \
  -d '{"key":"value"}'
```

### Adding Firmware Variable (ESP32)

1. **Add to transmission** in `firmware/node_sensor_10/main/main.c`:
```c
// Read sensor
int my_value = read_my_sensor();

// Only send if changed (deadband logic)
if (abs(my_value - last_my_value) >= DEADBAND_THRESHOLD) {
    send_telemetry("my_variable", my_value);
    last_my_value = my_value;
}
```

2. **Backend receives** via POST /api/telemetry:
```json
{"mac":"XX:XX","type":"my_variable","value":12345}
```

3. **Stored in** `aguada.leituras_raw` (sensor_id, variavel, valor)

### Debugging Node Communication

```bash
# Terminal 1: Monitor Gateway
cd firmware/gateway_esp_idf
idf.py -p /dev/ttyUSB0 monitor | grep -E "Dequeued|HTTP POST"

# Terminal 2: Monitor Sensor Node
cd firmware/node_sensor_10
idf.py -p /dev/ttyACM0 monitor | grep -E "distance_cm|valve"

# Terminal 3: Check Backend logs
tail -f backend/logs/*.log | grep -i telemetry

# Terminal 4: Check database inserts
psql -h localhost -U aguada -d aguada -c "SELECT * FROM aguada.leituras_raw ORDER BY datetime DESC LIMIT 5;" | watch -n 1
```

---

## ⚡ Quick Fixes for Common Issues

### "ESP-NOW packets not reaching gateway"
1. Check gateway MAC: `80:f1:b2:50:2e:c4` (hardcoded in sensor firmware)
2. Verify channel: Both must use channel 1
3. Check range: ESP-NOW range is 250m max (less through walls)
4. Monitor gateway: `idf.py -p /dev/ttyUSB0 monitor` for reception logs

### "Ultrasonic sensor returns 0 (timeout)"
1. Verify GPIO connections: TRIG→GPIO1, ECHO→GPIO0
2. Check sensor power: AJ-SR04M needs 5V (separate from ESP32 3.3V logic)
3. Verify trigger pulse: Must be exactly 10μs
4. Check timeout setting: 30ms should work for 450cm max

### "Backend not receiving telemetry"
1. Verify POST /api/telemetry endpoint is reached: `curl -X POST http://localhost:3000/api/telemetry -d '{"test":"data"}'`
2. Check logs: `tail -f backend/logs/*.log`
3. Verify database connection: `psql -h localhost -U aguada -d aguada -c "SELECT 1;"`
4. Check gateway→backend HTTP: Monitor gateway serial for "HTTP POST" messages

### "Database queries returning empty"
1. Always use schema prefix: `aguada.leituras_raw` not `leituras_raw`
2. Check table exists: `\dt aguada.*` in psql
3. Verify data inserted: `SELECT COUNT(*) FROM aguada.leituras_raw;`
4. Check datetime range: Queries might have old timestamps

---

## 🚀 Deployment Considerations

### Environment Variables (`.env` file)

```bash
# Backend
PORT=3000
DB_HOST=localhost
DB_PORT=5432
DB_NAME=aguada
DB_USER=aguada
DB_PASSWORD=<change_me>

# Gateway WiFi
WIFI_SSID=<your_network>
WIFI_PASSWORD=<your_password>

# MQTT (if used)
MQTT_BROKER=192.168.0.100
MQTT_PORT=1883

# Frontend
API_BASE_URL=http://localhost:3000
```

⚠️ **Never commit `.env` with real credentials!**

### Production Checklist

- [ ] Read `docs/RULES.md` completely (sections 2, 4, 5, 6)
- [ ] All 5 ESP32 nodes flashed with correct firmware
- [ ] Gateway MAC and channel matches node configuration
- [ ] Database initialized with `schema.sql`
- [ ] Backend environment variables set
- [ ] Frontend API_BASE_URL points to correct backend
- [ ] SSL certificates configured (if HTTPS required)
- [ ] Monitoring/logging configured
- [ ] Backup strategy for TimescaleDB

---

## 📚 Reference Documentation

### Essential Files (Read These!)

| File | Purpose | Size | Key Sections |
|------|---------|------|--------------|
| `docs/RULES.md` | Source of truth - system spec | 586 lines | 2 (Topology), 4 (Protocol), 5 (Calcs), 6 (Events) |
| `README.md` | Project overview | 552 lines | Architecture, Setup, API endpoints |
| `QUICKSTART.md` | 5-minute startup guide | 249 lines | Backend, sensors, validation |
| `docs/ESP32_C3_SUPER_MINI_PINOUT.md` | Hardware reference | varies | GPIO pins, voltage specs, boot modes |
| `backend/README.md` | Backend API docs | 228 lines | Installation, endpoints, config |
| `firmware/node_sensor_10/README.md` | Sensor firmware | 275 lines | Build, flash, debug |

### Key GitHub Labels/Issues

When creating issues or PRs:
- `firmware/*` - Changes to ESP32 code
- `backend/*` - Changes to Node.js API
- `frontend/*` - Changes to HTML/JS dashboard
- `database/*` - Schema or migration changes
- `docs/*` - Documentation updates

---

## 🎯 For New Developers

1. **First Day**: Read `docs/RULES.md` sections 2 & 4 (30 min)
2. **Setup**: Run QUICKSTART.md steps 1-2 (15 min)
3. **Understand**: Review `backend/src/routes/api.routes.js` (endpoints overview)
4. **Try**: Send test POST to `/api/health` endpoint
5. **Deep Dive**: Pick one component (firmware/backend/database) and trace data flow

---

## ⚠️ Common Mistakes to Avoid

### ❌ Mistake 1: Not Reading RULES.md
**Impact**: Implementing wrong architecture, causing complete rewrite
**Fix**: Always start by reading `docs/RULES.md` sections 2, 4, 5, 6

### ❌ Mistake 2: Reservoir-Specific Firmware
**Wrong**: Creating separate firmware for RCON/RCAV/RB03
**Fix**: Use universal firmware, MAC-based identification (backend maps MAC→reservoir)

### ❌ Mistake 3: Aggregated JSON Payload
**Wrong**: `{"mac":"XX:XX","distance_cm":244,"valve_in":1}` (all vars in 1 JSON)
**Fix**: Send each variable individually (see RULES.md Section 4.2)

### ❌ Mistake 4: Float Values in Transmission
**Wrong**: `send_telemetry("distance_cm", 244.8)`
**Fix**: Multiply by 100 → `send_telemetry("distance_cm", 24480)`

### ❌ Mistake 5: Wrong GPIO Pins
**Wrong**: Changing TRIG or ECHO pins to different GPIO numbers
**Fix**: Use fixed pins from `config.h` (TRIG=1, ECHO=0 - hardware is soldered)

### ❌ Mistake 6: Missing Schema Prefix in SQL
**Wrong**: `SELECT * FROM leituras_raw`
**Fix**: `SELECT * FROM aguada.leituras_raw`

### ❌ Mistake 7: Ignoring Deadband Logic
**Wrong**: Sending every sensor reading (wastes bandwidth, causes DB bloat)
**Fix**: Only send when change > ±2cm for distance (RULES.md 4.3)

---

## Debugging Workflow

### Build Process

```bash
cd firmware/node_sensor_10
idf.py set-target esp32c3  # First time only
idf.py build               # Compile

# Expected output:
# aguada_node10.bin binary size 0xb9570 bytes (759,152 bytes)
# 0x46a90 bytes (289,424 bytes / 28%) free
```

### Flashing and Monitoring

```bash
# Flash to ESP32-C3
idf.py -p /dev/ttyACM0 flash

# Monitor serial output
idf.py -p /dev/ttyACM0 monitor

# Combined (flash + monitor)
idf.py -p /dev/ttyACM0 flash monitor

# Exit monitor: Ctrl+]
```

### Expected Boot Sequence

```
I (403) AGUADA_NODE: GPIO inicializado (TRIG=1, ECHO=0, VALVE_IN=2, VALVE_OUT=3, SOUND=5)
I (1752) AGUADA_NODE: Node MAC: 20:6E:F1:6B:77:58
I (1753) AGUADA_NODE: ESP-NOW OK - Gateway: 80:F1:B2:50:2E:C4
I (4162) AGUADA_NODE: → {"mac":"20:6E:F1:6B:77:58","type":"distance_cm","value":24480,...}
I (4166) AGUADA_NODE: → {"mac":"20:6E:F1:6B:77:58","type":"valve_in","value":1,...}
I (4176) AGUADA_NODE: → {"mac":"20:6E:F1:6B:77:58","type":"valve_out","value":0,...}
I (4186) AGUADA_NODE: → {"mac":"20:6E:F1:6B:77:58","type":"sound_in","value":0,...}
```

### Troubleshooting

**Problem**: "esp_now.h: No such file"
**Solution**: In IDF 6.x, use `#include "esp_now.h"` (included via esp_wifi)

**Problem**: Callback compilation error
**Solution**: Update signature to IDF 6.x:

```c
void espnow_send_cb(const esp_now_send_info_t *info, esp_now_send_status_t status)
```

**Problem**: Ultrasonic sensor returns "Nenhuma amostra válida"
**Check**:

1. GPIO connections: TRIG → GPIO1, ECHO → GPIO0
2. Power: AJ-SR04M needs 5V (separate from ESP32 3.3V logic)
3. Timing: Ensure 10μs trigger pulse
4. Timeout: 30ms should be adequate for 450cm max range

**Problem**: ESP-NOW packets not reaching gateway
**Check**:

1. Gateway MAC: `80:f1:b2:50:2e:c4` (hardcoded)
2. Channel: 1 (must match gateway)
3. Gateway powered on and within range (< 250m)
4. Monitor gateway serial output for reception logs

---

## File Structure

### Project Overview

```
aguada/
├── .github/
│   └── copilot-instructions.md  # Este arquivo (instruções para AI)
├── backend/                      # API Node.js/Express
│   ├── src/
│   │   ├── server.js             # Entry point
│   │   ├── config/               # Configurações
│   │   ├── controllers/          # Lógica de negócio
│   │   ├── routes/               # Rotas API
│   │   ├── services/             # Serviços
│   │   ├── middleware/           # Middlewares
│   │   ├── schemas/              # Validação
│   │   ├── utils/                # Utilidades
│   │   └── websocket/            # Real-time
│   ├── scripts/
│   ├── logs/
│   ├── package.json
│   └── Dockerfile
├── config/                       # Configurações JSON globais
│   ├── network_topology.json
│   ├── reservoirs.json
│   ├── sensors.json
│   └── thresholds.json
├── database/                     # SQL schemas
│   ├── schema.sql                # Schema principal
│   ├── init.sql                  # Inicialização
│   └── sample-data.sql/          # Dados de exemplo
├── docker/                       # Docker configs
│   ├── Dockerfile.backend        # Build do backend
│   ├── nginx.conf                # Configuração Nginx
│   ├── certs/                    # Certificados SSL
│   ├── mosquitto/                # MQTT broker
│   │   └── config/mosquitto.conf
│   ├── grafana/                  # Grafana dashboards
│   │   └── provisioning/
│   └── postgres/                 # TimescaleDB configs
├── docs/                         # Documentação técnica
│   ├── RULES.md                  # ⚠️ FONTE DA VERDADE
│   ├── CHANGELOG.md              # Histórico de mudanças
│   ├── SETUP.md                  # Guia de instalação
│   └── ESP32_C3_SUPER_MINI_PINOUT.md
├── Documents/                    # Documentação operacional
│   ├── formularios/
│   ├── instrucoes/
│   └── relatorios/
├── firmware/                     # Código ESP32-C3
│   ├── node_sensor_10/           # TYPE_SINGLE_ULTRA
│   ├── node_sensor_20/           # TYPE_DUAL_ULTRA
│   ├── gateway_esp_idf/          # Gateway ESP-IDF
│   └── gateway_00_arduino/       # Gateway Arduino (legacy)
├── frontend/                     # Frontend HTML/JS
│   ├── index.html
│   ├── painel.html
│   ├── assets/
│   ├── components/
│   └── config/
├── mcp-server/                   # MCP Server TypeScript
│   ├── src/
│   ├── package.json
│   └── tsconfig.json
├── scripts/                      # Scripts de automação
│   ├── install.sh
│   ├── deploy.sh
│   └── backup.sh
├── .env.example                  # Variáveis de ambiente (exemplo)
├── docker-compose.yml            # Docker Compose principal
├── QUICKSTART.md                 # Guia rápido
└── README.md
```

### Firmware (ESP32-C3 Sensor Nodes)

```
firmware/
├── node_sensor_10/              # TYPE_SINGLE_ULTRA (RCON, RCAV, RB03)
│   ├── main/
│   │   ├── main.c               # Universal firmware
│   │   ├── config.h             # GPIO pins, constants
│   │   └── CMakeLists.txt
│   ├── CMakeLists.txt
│   └── sdkconfig                # ESP-IDF config (auto-generated)
├── node_sensor_20/              # TYPE_DUAL_ULTRA (IE01+IE02)
│   ├── main/
│   │   ├── main.c
│   │   └── CMakeLists.txt
│   └── CMakeLists.txt
├── gateway_esp_idf/             # Gateway ESP-NOW → WiFi/MQTT
│   └── main/
└── SENSOR_GATEWAY_FLOW.md       # Diagrama de fluxo
```

### Backend Controllers

```
backend/src/controllers/
├── telemetry.controller.js      # POST /api/telemetry (ESP32 data)
├── reading.controller.js        # GET /api/readings/*
├── sensors.controller.js        # CRUD /api/sensors
├── alerts.controller.js         # /api/alerts
├── stats.controller.js          # /api/stats/*
├── system.controller.js         # /api/system/*
├── gateway.controller.js        # /api/gateway/*
└── database.controller.js       # /api/database/*
```

### Database (PostgreSQL/TimescaleDB)

```
database/
└── schema.sql           # Hypertables, indexes, functions
```

### Documentation

```
docs/
├── RULES.md             # ⚠️ THE SOURCE OF TRUTH
├── ESP32_C3_SUPER_MINI_PINOUT.md
└── SENSOR_GATEWAY_FLOW.md
```

---

## Key Code Patterns

### Ultrasonic Distance Reading

```c
int read_ultrasonic_distance(void) {
    // Send 10μs trigger pulse
    gpio_set_level(TRIG_PIN, 0);
    esp_rom_delay_us(2);
    gpio_set_level(TRIG_PIN, 1);
    esp_rom_delay_us(10);
    gpio_set_level(TRIG_PIN, 0);

    // Wait for echo pulse (timeout: 30ms)
    int64_t start_time = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 0) {
        if ((esp_timer_get_time() - start_time) > TIMEOUT_US) {
            return -1;  // Timeout (sensor didn't respond)
        }
    }

    int64_t pulse_start = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 1) {
        if ((esp_timer_get_time() - pulse_start) > TIMEOUT_US) {
            return -1;  // Timeout
        }
    }
    int64_t pulse_end = esp_timer_get_time();

    uint32_t duration = (uint32_t)(pulse_end - pulse_start);

    // Calculate distance (cm × 100 for integer transmission)
    // Speed of sound: 343 m/s = 0.0343 cm/μs
    // Formula: distance = (duration × 0.034) / 2
    // Simplified: (duration × 343) / 20000
    int distance_cm_x100 = (int)((duration * 343) / 20000);

    // Validate range (20-450 cm)
    if (distance_cm_x100 < (MIN_DISTANCE_CM * 100) ||
        distance_cm_x100 > (MAX_DISTANCE_CM * 100)) {
        return -2;  // Out of range
    }

    return distance_cm_x100;
}
```

### Individual Variable Transmission

```c
void send_telemetry(const char *type, int value) {
    char payload[200];
    uint32_t uptime = esp_timer_get_time() / 1000000;  // Convert μs to seconds
    int rssi = -50;  // TODO: Read actual WiFi RSSI
    int battery = 5000;  // 5V DC source

    snprintf(payload, sizeof(payload),
             "{\"mac\":\"%s\",\"type\":\"%s\",\"value\":%d,"
             "\"battery\":%d,\"uptime\":%lu,\"rssi\":%d}",
             node_mac_str, type, value, battery, uptime, rssi);

    ESP_LOGI(TAG, "→ %s", payload);

    esp_err_t result = esp_now_send(gateway_mac, (uint8_t *)payload, strlen(payload));
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "ESP-NOW send error: %d", result);
    }
}
```

### Deadband Logic (Send Only on Change)

```c
void check_and_send_changes(void) {
    // Read ultrasonic sensor (median filtered)
    int distance_cm = read_distance_filtered();

    if (distance_cm > 0) {
        // First reading or change beyond deadband (±2cm)
        if (last_distance_cm < 0 ||
            abs(distance_cm - last_distance_cm) >= (DEADBAND_CM * VALUE_MULTIPLIER)) {
            send_telemetry("distance_cm", distance_cm);
            last_distance_cm = distance_cm;
        }
    } else if (distance_cm == -1) {
        // Timeout (sensor didn't respond)
        send_telemetry("distance_cm", 0);
        last_distance_cm = -1;
    } else if (distance_cm == -2) {
        // Out of range (sensor responded but invalid)
        send_telemetry("distance_cm", 1);
        last_distance_cm = -2;
    }

    // Check valve states (send on any state change)
    uint8_t valve_in = gpio_get_level(VALVE_IN_PIN);
    if (last_valve_in == 255 || valve_in != last_valve_in) {
        send_telemetry("valve_in", valve_in);
        last_valve_in = valve_in;
    }

    uint8_t valve_out = gpio_get_level(VALVE_OUT_PIN);
    if (last_valve_out == 255 || valve_out != last_valve_out) {
        send_telemetry("valve_out", valve_out);
        last_valve_out = valve_out;
    }

    uint8_t sound_in = gpio_get_level(SOUND_IN_PIN);
    if (last_sound_in == 255 || sound_in != last_sound_in) {
        send_telemetry("sound_in", sound_in);
        last_sound_in = sound_in;
    }
}
```

### Heartbeat Task (30-Second Interval)

```c
void telemetry_task(void *pvParameters) {
    while (1) {
        check_and_send_changes();  // Only sends if values changed
        vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_INTERVAL_MS));  // 30 seconds
    }
}
```

---

## Backend Implementation

### API Endpoint (POST /api/telemetry)

```javascript
// Expected payload:
{
  "mac": "20:6E:F1:6B:77:58",
  "type": "distance_cm",
  "value": 24480,
  "battery": 5000,
  "uptime": 3,
  "rssi": -50
}

// Backend maps MAC to reservoir:
const MAC_TO_RESERVOIR = {
  "20:6E:F1:6B:77:58": "RCON",
  "DC:06:75:67:6A:CC": "RCAV",
  // ... etc
};
```

---

## Testing Procedures

### 1. ESP-NOW Communication Test

```bash
# Terminal 1: Monitor gateway
idf.py -p /dev/ttyUSB0 monitor  # Gateway port

# Terminal 2: Flash and monitor sensor node
idf.py -p /dev/ttyACM0 flash monitor

# Expected: Gateway logs show received packets
```

### 2. Ultrasonic Sensor Test

```c
// Add debug logging in main.c
int distance = read_ultrasonic_distance();
ESP_LOGI(TAG, "Distance: %d (raw), %d.%02d cm",
         distance, distance/100, distance%100);
```

### 3. Backend API Test

```bash
# Test POST endpoint
curl -X POST http://192.168.0.100:3000/api/telemetry \
  -H "Content-Type: application/json" \
  -d '{
    "mac":"20:6E:F1:6B:77:58",
    "type":"distance_cm",
    "value":24480,
    "battery":5000,
    "uptime":10,
    "rssi":-50
  }'
```

### 4. Database Verification

```sql
-- Check raw readings
SELECT * FROM leituras_raw ORDER BY datetime DESC LIMIT 10;

-- Check processed readings
SELECT * FROM leituras_processadas
WHERE sensor_id = 'RCON'
ORDER BY data_inicio DESC LIMIT 10;
```

---

## Development Workflow

### Creating New Features

1. **Read RULES.md first** - Understand specification
2. **Check existing patterns** - Review `main.c` for similar code
3. **Test incrementally** - Build → Flash → Monitor after each change
4. **Verify logs** - Use `ESP_LOGI/ESP_LOGE` for debugging
5. **Document changes** - Update README.md if architecture changes

### Modifying Firmware

1. **Never change GPIO pins** - Hardware is fixed
2. **Never change transmission format** - Backend depends on it
3. **Always use integers** - No floats in transmission
4. **Test with multiple nodes** - Ensure universal firmware works
5. **Check binary size** - Must fit in 1MB partition (currently 28% free)

### Git Workflow

```bash
# Before committing firmware changes
cd firmware/node_sensor_10
idf.py build  # Ensure it compiles

# Don't commit build artifacts
# .gitignore already excludes: build/, sdkconfig.old, *.bin
```

---

## Quick Reference

### GPIO Pin Map (All Nodes)

| Pin    | Function   | Direction | Notes                     |
| ------ | ---------- | --------- | ------------------------- |
| GPIO 0 | ECHO       | INPUT     | Ultrasonic echo pulse     |
| GPIO 1 | TRIG       | OUTPUT    | Ultrasonic trigger        |
| GPIO 2 | VALVE_IN   | INPUT     | Input valve state (0/1)   |
| GPIO 3 | VALVE_OUT  | INPUT     | Output valve state (0/1)  |
| GPIO 5 | SOUND_IN   | INPUT     | Water flow detector (0/1) |
| GPIO 8 | LED_STATUS | OUTPUT    | Heartbeat LED (blink 3s)  |

### Value Multipliers

| Variable    | Type        | Multiplier | Example       |
| ----------- | ----------- | ---------- | ------------- |
| distance_cm | float → int | × 100      | 244.8 → 24480 |
| valve_in    | bool → int  | none       | 0 or 1        |
| valve_out   | bool → int  | none       | 0 or 1        |
| sound_in    | bool → int  | none       | 0 or 1        |
| battery     | int         | none       | 5000 (mV)     |
| uptime      | int         | none       | seconds       |
| rssi        | int         | none       | -50 (dBm)     |

### Transmission Thresholds

| Variable    | Threshold  | Heartbeat |
| ----------- | ---------- | --------- |
| distance_cm | ±2 cm      | 30s       |
| valve_in    | Any change | 30s       |
| valve_out   | Any change | 30s       |
| sound_in    | Any change | 30s       |

### ESP-IDF Commands

```bash
# Setup (first time)
idf.py set-target esp32c3

# Build
idf.py build

# Flash
idf.py -p /dev/ttyACM0 flash

# Monitor
idf.py -p /dev/ttyACM0 monitor

# Clean
idf.py fullclean

# Menuconfig
idf.py menuconfig
```

---

## Summary: Golden Rules

1. **📖 ALWAYS read `docs/RULES.md` before coding** - It defines the entire system
2. **🎯 Universal firmware only** - Same binary for all 5 reservoirs (MAC-based ID)
3. **📤 Individual variable transmission** - One JSON per variable type
4. **🔢 Integer values only** - Multiply floats by 100 before transmission
5. **📌 Fixed GPIO pins** - Never change TRIG=1, ECHO=0, VALVE_IN=2, etc.
6. **🔧 ESP-IDF 6.x patterns** - Use new callback signatures and component structure
7. **📊 Deadband logic** - Only send when change > ±2cm or state changes
8. **💓 30-second heartbeat** - Send last known values even if unchanged
9. **🐛 Debug with ESP_LOGI** - Use logging instead of printf
10. **✅ Test incrementally** - Build → Flash → Monitor after each change

---

## Frontend Architecture (HTML/CSS/JS - Legacy)

### Legacy Frontend Structure

```
frontend/
├── index.html           # Dashboard principal
├── painel.html          # Painel de controle
├── alerts.html          # Sistema de alertas
├── history.html         # Histórico de leituras
├── config.html          # Configurações do sistema
├── consumo.html         # Análise de consumo
├── abastecimento.html   # Controle de abastecimento
├── manutencao.html      # Gestão de manutenção
├── mapa.html            # Mapa da rede hidráulica
├── dados.html           # Explorador de dados
├── system.html          # Status do sistema
├── documentacao.html    # Documentação
├── service-worker.js    # PWA offline support
├── assets/              # CSS, JS, imagens
├── components/          # Web components reutilizáveis
└── config/              # Configurações JSON
```

### Key Frontend Patterns

- Vanilla JavaScript com ES6+ modules
- PWA com service worker para offline
- CSS moderno com variáveis e grid
- Fetch API para comunicação com backend
- LocalStorage para cache de dados

---

## Backend Architecture (Node.js/Express)

### Backend File Structure

```
backend/
├── src/
│   ├── server.js              # Express entry point
│   ├── config/                # Configurações (db, mqtt, etc.)
│   ├── controllers/           # Lógica de negócio
│   │   ├── telemetry.controller.js   # Recebe dados ESP32
│   │   ├── reading.controller.js     # Leituras e histórico
│   │   ├── sensors.controller.js     # CRUD sensores
│   │   ├── alerts.controller.js      # Sistema de alertas
│   │   ├── stats.controller.js       # Estatísticas
│   │   ├── system.controller.js      # Health/metrics
│   │   ├── gateway.controller.js     # Métricas gateway
│   │   └── database.controller.js    # Acesso direto BD
│   ├── routes/
│   │   └── api.routes.js      # Definição de rotas
│   ├── services/              # Serviços (export, etc.)
│   ├── middleware/            # Validação, auth, etc.
│   ├── schemas/               # Validação de dados
│   ├── utils/                 # Funções utilitárias
│   └── websocket/             # WebSocket para real-time
├── scripts/                   # Scripts de manutenção
├── logs/                      # Arquivos de log
├── package.json
└── Dockerfile
```

### API Endpoints

#### Telemetry (POST)

```bash
# Recebe telemetria dos ESP32
POST /api/telemetry
Body: {"mac":"XX:XX","type":"distance_cm","value":24480,...}

# Leitura manual
POST /api/manual-reading

# Calibração
POST /api/calibration
```

#### Readings (GET)

```bash
# Últimas leituras
GET /api/readings/latest

# Leituras raw com paginação
GET /api/readings/raw?limit=100&offset=0

# Resumo diário
GET /api/readings/daily-summary

# Histórico de sensor
GET /api/readings/history/:sensor_id?start=&end=

# Export CSV
GET /api/readings/export?format=csv
```

#### Sensors

```bash
# Listar todos
GET /api/sensors

# Status (online/offline)
GET /api/sensors/status

# Sensor específico
GET /api/sensors/:sensor_id

# Atualizar sensor
PUT /api/sensors/:sensor_id
```

#### Alerts

```bash
# Listar alertas
GET /api/alerts?status=active&type=level

# Resumo
GET /api/alerts/summary

# Criar alerta
POST /api/alerts

# Resolver alerta
PUT /api/alerts/:alert_id/resolve

# Export
GET /api/alerts/export
```

#### Statistics

```bash
# Estatísticas diárias
GET /api/stats/daily

# Consumo
GET /api/stats/consumption

# Sensores
GET /api/stats/sensors

# Eventos
GET /api/stats/events
```

#### System

```bash
# Health check
GET /api/health
GET /api/system/health

# Logs
GET /api/system/logs

# Métricas
GET /api/system/metrics

# Alertas do sistema
GET /api/system/alerts

# Restart (admin)
POST /api/system/restart
```

#### Gateway

```bash
# Recebe métricas
POST /api/gateway/metrics

# Obtém métricas
GET /api/gateway/metrics
```

#### Database Direct Access

```bash
# Lista tabelas
GET /api/database/tables

# Dados de tabela
GET /api/database/table/:tableName?limit=100
```

---

## Database Schema (PostgreSQL/TimescaleDB)

### Schema: `aguada`

#### Tabelas de Configuração

```sql
-- Usuários do sistema
aguada.usuarios (usuario_id, nome, email, role, ativo, criado_em, ultimo_login)

-- Elementos hidráulicos (reservatórios, bombas, válvulas)
aguada.elementos (elemento_id, tipo, nome, descricao, coordenadas, parametros, status)
-- tipo: 'reservatorio', 'bomba', 'valvula', 'rede'

-- Portas de elementos (entradas/saídas)
aguada.portas (porta_id, elemento_id, nome, tipo, descricao)

-- Conexões entre portas (grafo hidráulico)
aguada.conexoes (conexao_id, porta_origem_id, porta_destino_id, ativo)

-- Sensores
aguada.sensores (sensor_id, elemento_id, node_mac, tipo, modelo, variavel, unidade, gpio_config, status)

-- Configurações por elemento
aguada.elemento_configs (elemento_id, deadband, window_size, stability_stddev, nivel_critico_percent)
```

#### Tabelas de Telemetria (Hypertables)

```sql
-- Leituras brutas (TODAS as leituras recebidas)
aguada.leituras_raw (leitura_id, sensor_id, elemento_id, variavel, valor, unidade, meta, fonte, datetime, processed)
-- Particionada por datetime (TimescaleDB)

-- Leituras processadas (APENAS mudanças significativas)
aguada.leituras_processadas (proc_id, elemento_id, variavel, valor, volume_m3, percentual, data_inicio, data_fim)

-- Estados de equipamentos
aguada.estados_equipamentos (estado_id, elemento_id, tipo, estado, datetime)
-- estado: 'ON'/'OFF', 'ABERTA'/'FECHADA'
```

#### Tabelas de Eventos

```sql
-- Eventos detectados
aguada.eventos (evento_id, tipo, elemento_id, detalhe, causa_provavel, nivel_confianca, datetime_inicio, datetime_fim)

-- Anomalias em investigação
aguada.anomalias (anomalia_id, tipo, elemento_id, descricao, nivel_alerta, inicio, fim, status)
```

#### Tabelas de Auditoria

```sql
-- Calibrações
aguada.calibracoes (calibracao_id, sensor_id, elemento_id, valor_referencia, valor_sensor, ajuste_aplicado, datetime)

-- Log de auditoria
aguada.auditoria (log_id, tabela, operacao, registro_id, usuario, dados_anteriores, dados_novos, datetime)
```

#### Tabelas de Relatórios

```sql
-- Relatórios diários
aguada.relatorios_diarios (relatorio_id, data, volume_consumido_total_l, volume_abastecido_total_l, eventos_registrados)

-- Consumo diário por reservatório
aguada.consumo_diario (consumo_id, data, elemento_id, volume_inicial_m3, volume_final_m3, consumo_total_l)
```

### Importante: Schema Prefix

**SEMPRE usar prefixo `aguada.` nas queries SQL:**

```sql
-- ✅ CORRETO
SELECT * FROM aguada.leituras_raw WHERE sensor_id = 'RCON_nivel';

-- ❌ ERRADO
SELECT * FROM leituras_raw WHERE sensor_id = 'RCON_nivel';
```

---

## Docker Configuration

### docker-compose.yml

```yaml
services:
  postgres:
    image: timescale/timescaledb:latest-pg16
    ports: ["5433:5432"]
    environment:
      POSTGRES_DB: aguada
      POSTGRES_USER: aguada
      POSTGRES_PASSWORD: ${DB_PASSWORD:-aguada123}
    volumes:
      - postgres_data:/var/lib/postgresql/data
      - ./database/schema.sql:/docker-entrypoint-initdb.d/01-schema.sql

  mosquitto:
    image: eclipse-mosquitto:2
    ports: ["${MQTT_PORT:-1883}:1883", "${MQTT_WS_PORT:-9001}:9001"]
    volumes:
      - ./docker/mosquitto/config:/mosquitto/config
      - mosquitto_data:/mosquitto/data

  redis:
    image: redis:7-alpine
    ports: ["${REDIS_PORT:-6379}:6379"]

  grafana:
    image: grafana/grafana:latest
    ports: ["${GRAFANA_PORT:-3001}:3000"]
    volumes:
      - ./docker/grafana/provisioning:/etc/grafana/provisioning
```

### Comandos Docker

```bash
# Iniciar todos os serviços
docker compose up -d

# Ver logs
docker compose logs -f backend

# Reiniciar um serviço
docker compose restart backend

# Parar tudo
docker compose down

# Rebuild após mudanças
docker compose build --no-cache backend
docker compose up -d backend
```

### Backend Start (Desenvolvimento Local)

```bash
# Desenvolvimento
cd backend
npm install
npm run dev  # porta 3000

# Produção
npm start
```

### Database Connection

```javascript
// backend/src/config/database.js
// Credenciais via variáveis de ambiente (.env)
{
  host: process.env.DB_HOST || 'localhost',
  port: process.env.DB_PORT || 5433,
  database: process.env.DB_NAME || 'aguada',
  user: process.env.DB_USER,      // Nunca hardcode!
  password: process.env.DB_PASS   // Nunca hardcode!
}
```

> ⚠️ **SECURITY**: Use arquivo `.env` (não versionado) para credenciais

---

## MCP Server (Model Context Protocol)

### MCP Server Structure

```
mcp-server/
├── src/
│   └── index.ts           # Servidor MCP TypeScript
├── package.json
├── tsconfig.json
└── README.md
```

### MCP Tools Disponíveis

```typescript
// Ferramentas para acesso a dados via MCP
const mcpTools = [
  "get_reservoir_status", // Status atual dos reservatórios
  "get_latest_readings", // Últimas leituras de sensores
  "get_events", // Eventos recentes do sistema
  "get_sensor_history", // Histórico de um sensor
  "query_database", // Query SQL customizada
  "get_system_health", // Health check do sistema
  "get_consumption_stats", // Estatísticas de consumo
  "get_alerts", // Alertas ativos
];
```

### Compilação e Execução

```bash
cd mcp-server
npm install
npm run build  # Compila TypeScript
npm start      # Inicia servidor MCP
```

---

## Common Development Tasks

### Adicionar Novo Endpoint API

1. Criar/editar controller em `backend/src/controllers/`
2. Registrar rota em `backend/src/routes/api.routes.js`
3. Testar com curl ou Postman
4. Documentar neste arquivo

### Adicionar Nova Tabela no Banco

1. Adicionar DDL em `database/schema.sql`
2. Executar SQL no PostgreSQL
3. Atualizar controller relacionado
4. Atualizar tipos no frontend

### Debug Frontend

1. Abrir DevTools (F12)
2. Verificar Network tab para falhas de API
3. Verificar Console para erros JS
4. Testar endpoints com curl primeiro

### Debug Backend

1. Verificar logs: `tail -f backend/logs/*.log`
2. Testar endpoint: `curl http://localhost:3000/api/health`
3. Verificar conexão DB: `psql -h localhost -p 5433 -U aguada -d aguada`
4. Monitor serial ESP32: `idf.py -p /dev/ttyACM0 monitor`

---

**Last Updated**: 2025-12-05  
**System Version**: 2.1.0  
**Firmware Version**: v1.0.0  
**ESP-IDF Version**: 6.1.0 (ff97953b)  
**Backend Version**: Node.js + Express (porta 3000)  
**Database**: PostgreSQL 16 + TimescaleDB (porta 5433)  
**Devices Flashed**: 2/5 (MACs: 20:6e:f1:6b:77:58, dc:06:75:67:6a:cc)
