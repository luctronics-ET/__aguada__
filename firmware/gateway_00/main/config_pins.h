#ifndef CONFIG_PINS_H
#define CONFIG_PINS_H

// =============================================================================
// ESP32-C3 SUPER MINI - GATEWAY CONFIGURATION
// =============================================================================

// LED de Status
#define LED_BUILTIN       GPIO_NUM_8   // GPIO8 - LED onboard
#define LED_WIFI          GPIO_NUM_10  // LED WiFi status
#define LED_MQTT          GPIO_NUM_2   // LED MQTT status

// Serial para comunicação com nodes (opcional)
#define SERIAL_RX_PIN     GPIO_NUM_20
#define SERIAL_TX_PIN     GPIO_NUM_21

// WiFi Configuration
#define WIFI_SSID         "AGUADA_NETWORK"
#define WIFI_PASSWORD     "aguada2025"

// MQTT Configuration
#define MQTT_BROKER       "192.168.1.100"
#define MQTT_PORT         1883
#define MQTT_USER         "aguada_node"
#define MQTT_PASS         "mqtt_pass"
#define MQTT_TOPIC_BASE   "aguada/telemetry"
#define MQTT_TOPIC_CMD    "aguada/command"
#define MQTT_TOPIC_STATUS "aguada/status"

// HTTP Fallback
#define HTTP_SERVER       "http://192.168.1.100:3000"
#define HTTP_ENDPOINT     "/api/telemetry"

// Gateway Configuration
#define GATEWAY_NAME      "gateway_00"
#define MAX_NODES         10
#define QUEUE_SIZE        50
#define RETRY_ATTEMPTS    3

// Timing Constants
#define HEARTBEAT_INTERVAL_MS    5000    // 5 segundos
#define QUEUE_CHECK_INTERVAL_MS  1000    // 1 segundo
#define WATCHDOG_TIMEOUT_SEC     60      // 1 minuto
#define NODE_TIMEOUT_SEC         300     // 5 minutos (considerar node offline)

// BLE Configuration (opcional - para receber de nodes BLE)
#define BLE_SCAN_DURATION_SEC    5
#define BLE_SCAN_INTERVAL_MS     30000   // 30 segundos

#endif // CONFIG_PINS_H
