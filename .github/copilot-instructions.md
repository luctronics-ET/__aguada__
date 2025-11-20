# AGUADA - AI Coding Agent Instructions

**⚠️ CRITICAL: Read `docs/RULES.md` FIRST before making ANY code changes!**

## Project Overview

AGUADA is an IoT hydraulic monitoring system for 5 water reservoirs using ESP32-C3 microcontrollers with ESP-NOW wireless communication. The system monitors water levels, valve states, and consumption patterns across:

- **5 Reservoirs**: RCON, RCAV, RB03, IE01, IE02
- **4 ESP32-C3 SuperMini** sensor nodes:
  - **3x TYPE_SINGLE_ULTRA** (node_sensor_10): RCON, RCAV, RB03 - 1 reservatório por ESP32
  - **1x TYPE_DUAL_ULTRA** (node_sensor_20): IE01 + IE02 - 2 reservatórios em 1 ESP32
- **ESP-NOW protocol** for sensor → gateway communication (up to 250m range)
- **Gateway ESP32-C3** converts ESP-NOW → MQTT/HTTP
- **Backend**: Node.js/Express → TimescaleDB (PostgreSQL)
- **Dashboard**: Grafana for visualization

---

## Essential Documents (Read These First!)

### 1. `docs/RULES.md` - **THE SOURCE OF TRUTH** (551 lines)
This is the single most important document. It contains:
- Hydraulic topology (Section 2) - reservoir specifications
- Data model and transmission protocol (Section 4) - **CRITICAL FOR FIRMWARE**
- Physical calculations (Section 5) - volume, pressure, flow
- Event detection rules (Section 6) - leaks, supply, alerts

**Key sections you MUST read before coding:**
- **Section 4.2**: Individual Variable Transmission (each variable sent separately)
- **Section 4.3**: Noise filtering (median, deadband, stability)
- **Section 4.4**: Data compression (raw vs processed tables)

### 2. `README.md` - Project Quick Start
- Architecture overview
- API endpoints
- Hardware specifications
- Quick commands

### 3. `docs/ESP32_C3_SUPER_MINI_PINOUT.md` - Hardware Reference
- GPIO pin assignments
- Voltage levels (3.3V logic)
- Boot modes and constraints

---

## Core Architecture Principles

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
{"mac":"20:6E:F1:6B:77:58","distance_cm":24480,"valve_in":1,"valve_out":0}
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
#define WIFI_SSID "luciano"
#define WIFI_PASS "Luciano19852012"
// Gateway IP: 192.168.0.124 (DHCP assigned, stable)
```

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

## Common Mistakes to Avoid

### ❌ Mistake 1: Not Reading RULES.md
**Impact**: Implementing wrong architecture, causing complete rewrite
**Fix**: Always start by reading `docs/RULES.md` sections 2, 4, 5, 6

### ❌ Mistake 2: Reservoir-Specific Firmware
**Wrong**:
```c
#ifdef RCON
  #define HEIGHT_CM 400
#endif
```
**Fix**: Use universal firmware, MAC-based identification

### ❌ Mistake 3: Aggregated JSON Payload
**Wrong**:
```json
{"mac":"XX:XX","distance":244,"valve_in":1,"valve_out":0}
```
**Fix**: Send each variable individually (see Section 4.2 in RULES.md)

### ❌ Mistake 4: Float Values in Transmission
**Wrong**:
```c
send_telemetry("distance_cm", 244.8);
```
**Fix**: Multiply by 100 → `send_telemetry("distance_cm", 24480)`

### ❌ Mistake 5: Wrong GPIO Pins
**Wrong**:
```c
#define TRIG_PIN GPIO_NUM_4  // Will cause hardware mismatch!
```
**Fix**: Use fixed pins from `config.h` (TRIG=1, ECHO=0, etc.)

### ❌ Mistake 6: IDF 5.x ESP-NOW Callback
**Wrong**:
```c
void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
```
**Fix**: Use IDF 6.x signature with `esp_now_send_info_t` struct

### ❌ Mistake 7: Adding esp_now to CMakeLists
**Wrong**:
```cmake
REQUIRES esp_wifi esp_now
```
**Fix**: Only `esp_wifi` (esp_now merged into it in IDF 6.x)

### ❌ Mistake 8: Ignoring Deadband Logic
**Wrong**: Send every reading
**Fix**: Only send when change > ±2cm or state changes (see `check_and_send_changes()`)

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

### Firmware (ESP32-C3 Sensor Nodes)
```
firmware/node_sensor_10/
├── main/
│   ├── main.c           # Universal firmware (375 lines)
│   ├── config.h         # GPIO pins, constants, settings
│   └── CMakeLists.txt   # Build configuration
├── CMakeLists.txt       # Project-level build config
└── sdkconfig            # ESP-IDF configuration (auto-generated)
```

### Backend (Node.js/Express)
```
backend/
├── server.js            # Express API
├── db/
│   ├── connection.js    # TimescaleDB client
│   └── queries.js       # SQL queries
└── routes/
    └── telemetry.js     # POST /api/telemetry
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

### Database Schema (TimescaleDB)
```sql
-- Raw readings (all received data)
CREATE TABLE leituras_raw (
  datetime TIMESTAMPTZ NOT NULL,
  sensor_id VARCHAR(20),
  mac_address VARCHAR(17),
  variable_type VARCHAR(20),
  value_int INTEGER,
  battery_mv INTEGER,
  uptime_sec INTEGER,
  rssi_dbm INTEGER,
  processed BOOLEAN DEFAULT FALSE
);

SELECT create_hypertable('leituras_raw', 'datetime');

-- Processed readings (compressed, only significant changes)
CREATE TABLE leituras_processadas (
  sensor_id VARCHAR(20),
  variable_type VARCHAR(20),
  data_inicio TIMESTAMPTZ,
  data_fim TIMESTAMPTZ,
  valor_int INTEGER,
  percentual FLOAT,
  PRIMARY KEY (sensor_id, variable_type, data_inicio)
);
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
| Pin | Function | Direction | Notes |
|-----|----------|-----------|-------|
| GPIO 0 | ECHO | INPUT | Ultrasonic echo pulse |
| GPIO 1 | TRIG | OUTPUT | Ultrasonic trigger |
| GPIO 2 | VALVE_IN | INPUT | Input valve state (0/1) |
| GPIO 3 | VALVE_OUT | INPUT | Output valve state (0/1) |
| GPIO 5 | SOUND_IN | INPUT | Water flow detector (0/1) |
| GPIO 8 | LED_STATUS | OUTPUT | Heartbeat LED (blink 3s) |

### Value Multipliers
| Variable | Type | Multiplier | Example |
|----------|------|------------|---------|
| distance_cm | float → int | × 100 | 244.8 → 24480 |
| valve_in | bool → int | none | 0 or 1 |
| valve_out | bool → int | none | 0 or 1 |
| sound_in | bool → int | none | 0 or 1 |
| battery | int | none | 5000 (mV) |
| uptime | int | none | seconds |
| rssi | int | none | -50 (dBm) |

### Transmission Thresholds
| Variable | Threshold | Heartbeat |
|----------|-----------|-----------|
| distance_cm | ±2 cm | 30s |
| valve_in | Any change | 30s |
| valve_out | Any change | 30s |
| sound_in | Any change | 30s |

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

**Last Updated**: 2024-12-19  
**Firmware Version**: v1.0.0  
**ESP-IDF Version**: 6.1.0 (ff97953b)  
**Devices Flashed**: 2/5 (MACs: 20:6e:f1:6b:77:58, dc:06:75:67:6a:cc)
