#ifndef CONFIG_PINS_H
#define CONFIG_PINS_H

// =============================================================================
// ESP32-C3 SUPER MINI - PINOUT CONFIGURATION
// =============================================================================

// Sensor Ultrassônico AJ-SR04M
#define ULTRA_TRIG_PIN    GPIO_NUM_1   // GPIO1 - Trigger
#define ULTRA_ECHO_PIN    GPIO_NUM_0   // GPIO0 - Echo

// LED de Status
#define LED_BUILTIN       GPIO_NUM_8   // GPIO8 - LED onboard

// WiFi Configuration
#define WIFI_SSID         "AGUADA_NETWORK"
#define WIFI_PASSWORD     "aguada2025"

// MQTT Configuration
#define MQTT_BROKER       "192.168.1.100"
#define MQTT_PORT         1883
#define MQTT_USER         "aguada_node"
#define MQTT_PASS         "mqtt_pass"
#define MQTT_TOPIC_BASE   "aguada/telemetry"

// HTTP Fallback
#define HTTP_SERVER       "http://192.168.1.100:3000"
#define HTTP_ENDPOINT     "/api/telemetry"

// Timing Constants
#define TELEMETRY_INTERVAL_MS    30000   // 30 segundos
#define READING_INTERVAL_MS      10000   // 10 segundos para mediana
#define MEDIAN_SAMPLES           11      // Janela de mediana
#define WATCHDOG_TIMEOUT_SEC     120     // 2 minutos

// Node Configuration
#define NODE_NAME         "node_10"
#define SENSOR_ID         "SEN_CON_01"
#define ELEMENTO_ID       "res_cons"

// Sensor Physical Parameters
#define SENSOR_OFFSET_CM       40.0f    // Distância sensor → topo do reservatório
#define RESERVOIR_HEIGHT_CM    400.0f   // Altura total do reservatório
#define RESERVOIR_DIAMETER_CM  510.0f   // Diâmetro (para cálculo de volume)
#define RESERVOIR_FORMA        "cilindrica"

// AJ-SR04M Specifications
#define ULTRA_MIN_DISTANCE_CM  20.0f
#define ULTRA_MAX_DISTANCE_CM  450.0f
#define ULTRA_TIMEOUT_US       30000    // 30ms timeout
#define ULTRA_SOUND_SPEED_CM_US 0.0343f // 343 m/s = 0.0343 cm/µs

#endif // CONFIG_PINS_H
