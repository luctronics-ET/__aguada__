/**
 * AGUADA - Gateway WiFi Configuration
 * ESP32-C3 SuperMini - Recebe ESP-NOW e envia MQTT/HTTP
 */

#ifndef GATEWAY_CONFIG_H
#define GATEWAY_CONFIG_H

// Gateway Identification
#define GATEWAY_ID          "gateway_00"
#define FIRMWARE_VERSION    "v2.0.0"

// WiFi Configuration
#define WIFI_SSID          "luciano"
#define WIFI_PASSWORD      "Luciano19852012"

// MQTT Configuration
#define MQTT_BROKER_URL    "mqtt://192.168.0.100"  // Ajuste para seu servidor
#define MQTT_BROKER_PORT   1883
#define MQTT_TOPIC_TELEMETRY  "aguada/telemetry"
#define MQTT_TOPIC_STATUS     "aguada/gateway/status"
#define MQTT_CLIENT_ID     GATEWAY_ID

// HTTP Fallback Configuration  
#define HTTP_BACKEND_URL   "http://192.168.0.100:3000/api/telemetry"

// ESP-NOW Configuration
#define ESPNOW_CHANNEL     1
#define ESPNOW_QUEUE_SIZE  10

// LED Pins
#define LED_WIFI           GPIO_NUM_10
#define LED_MQTT           GPIO_NUM_2
#define LED_ESPNOW         GPIO_NUM_8

// Timing
#define HEARTBEAT_INTERVAL_MS     60000  // 1 minuto
#define RECONNECT_INTERVAL_MS     5000   // 5 segundos

#endif // GATEWAY_CONFIG_H
