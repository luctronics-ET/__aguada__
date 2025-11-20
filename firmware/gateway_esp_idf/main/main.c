/**
 * AGUADA - Gateway v3.2 (ESP-IDF C)
 * ESP-NOW Receiver → WiFi HTTP POST Bridge
 * 
 * Features:
 * - Canal fixo 11 (configurado com SSID "luciano")
 * - Queue-based HTTP POST para backend
 * - Otimizado para baixo consumo de energia
 * - Preparado para módulo Ethernet ENC28J60
 * 
 * ESP32-C3 SuperMini
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_http_client.h"

#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define TAG "AGUADA_GATEWAY"

// ============================================================================
// CONFIGURAÇÕES
// ============================================================================

#define WIFI_SSID "luciano"
#define WIFI_PASS "Luciano19852012"
#define BACKEND_URL "http://192.168.0.117:3000/api/telemetry"
#define ESPNOW_CHANNEL 11  // Fixed channel for SSID "luciano"
#define LED_BUILTIN GPIO_NUM_8
#define HEARTBEAT_INTERVAL_MS 3000
#define MAX_PAYLOAD_SIZE 256

// ============================================================================
// TYPES
// ============================================================================

typedef struct {
    uint8_t src_addr[6];
    char payload[MAX_PAYLOAD_SIZE];
    int len;
} espnow_packet_t;

// ============================================================================
// GLOBALS
// ============================================================================

static uint8_t gateway_mac[6];
static bool wifi_connected = false;
static int64_t last_heartbeat = 0;
static bool led_state = false;
static QueueHandle_t espnow_queue = NULL;

// ============================================================================
// UTILITIES
// ============================================================================

static void mac_to_string(const uint8_t *mac, char *str) {
    snprintf(str, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// ============================================================================
// ESP-NOW CALLBACK (Fast - just enqueue)
// ============================================================================

static void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    if (!recv_info || !espnow_queue) {
        return;
    }

    espnow_packet_t packet = {0};
    memcpy(packet.src_addr, recv_info->src_addr, 6);
    
    if (len >= MAX_PAYLOAD_SIZE) {
        len = MAX_PAYLOAD_SIZE - 1;
    }
    memcpy(packet.payload, data, len);
    packet.payload[len] = '\0';
    packet.len = len;

    // Enqueue without blocking
    xQueueSendFromISR(espnow_queue, &packet, NULL);
}

// ============================================================================
// PACKET PROCESSING TASK (Process queue)
// ============================================================================

static void packet_processing_task(void *pvParameters) {
    espnow_packet_t packet;
    
    while (1) {
        // Wait for packet (block until available)
        if (xQueueReceive(espnow_queue, &packet, pdMS_TO_TICKS(1000))) {
            char src_mac_str[18];
            mac_to_string(packet.src_addr, src_mac_str);

            // Log received packet
            ESP_LOGI(TAG, "");
            ESP_LOGI(TAG, "╔════════════════════════════════════════════════════╗");
            ESP_LOGI(TAG, "║ ✓ ESP-NOW recebido de: %s (%d bytes)", src_mac_str, packet.len);
            ESP_LOGI(TAG, "╠════════════════════════════════════════════════════╣");
            ESP_LOGI(TAG, "║ Dados: %s", packet.payload);
            ESP_LOGI(TAG, "╚════════════════════════════════════════════════════╝");

            // Skip HTTP POST if WiFi not connected
            if (!wifi_connected) {
                ESP_LOGW(TAG, "⚠ WiFi desconectado - Dados não enviados");
                continue;
            }

            // Make HTTP POST request (timeout reduced to 3s to avoid blocking)
            esp_http_client_config_t config = {
                .url = BACKEND_URL,
                .method = HTTP_METHOD_POST,
                .timeout_ms = 3000,  // 3 seconds (reduced from 5s)
            };
            
            esp_http_client_handle_t client = esp_http_client_init(&config);
            if (client) {
                esp_http_client_set_header(client, "Content-Type", "application/json");
                esp_http_client_set_post_field(client, packet.payload, packet.len);
                
                esp_err_t err = esp_http_client_perform(client);
                if (err == ESP_OK) {
                    int status = esp_http_client_get_status_code(client);
                    if (status == 200 || status == 201) {
                        ESP_LOGI(TAG, "→ Enviado via HTTP (status=%d)", status);
                    } else {
                        ESP_LOGW(TAG, "✗ HTTP status=%d", status);
                    }
                } else {
                    ESP_LOGW(TAG, "✗ HTTP error: %s", esp_err_to_name(err));
                }
                esp_http_client_cleanup(client);
            } else {
                ESP_LOGW(TAG, "✗ Falha ao criar cliente HTTP");
            }
        }
    }
}

// ============================================================================
// WIFI EVENT HANDLER
// ============================================================================

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi started, connecting...");
        wifi_connected = false;
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WiFi disconnected, reconnecting...");
        wifi_connected = false;
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "✓ WiFi connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connected = true;
    }
}

// ============================================================================
// WIFI INIT (Full connection for HTTP)
// ============================================================================

static void wifi_init_sta(void) {
    ESP_LOGI(TAG, "Inicializando WiFi (modo STA completo)...");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize network interface
    ESP_ERROR_CHECK(esp_netif_init());
    
    // Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // Create default WiFi STA
    esp_netif_create_default_wifi_sta();

    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    // Configure WiFi
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    
    // Set WiFi power save mode to reduce heat (MODEM sleep allows ESP-NOW + WiFi)
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MIN_MODEM));
    
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Set fixed channel 11 for ESP-NOW (MUST be called AFTER esp_wifi_start())
    ESP_ERROR_CHECK(esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));
    
    // Reduce WiFi TX power to 15 dBm (60 = 15dBm, default is 80 = 20dBm)
    // Gateway is close to AP, doesn't need max power
    // MUST be called AFTER esp_wifi_start()
    ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(60));

    ESP_LOGI(TAG, "✓ WiFi inicializado (SSID: %s, Canal: %d, TX: 15dBm)", WIFI_SSID, ESPNOW_CHANNEL);
}

// ============================================================================
// ESP-NOW INIT
// ============================================================================

static void espnow_init(void) {
    ESP_LOGI(TAG, "Inicializando ESP-NOW...");

    // Get gateway MAC
    esp_wifi_get_mac(WIFI_IF_STA, gateway_mac);
    char mac_str[18];
    mac_to_string(gateway_mac, mac_str);
    ESP_LOGI(TAG, "Gateway MAC: %s", mac_str);

    // Initialize ESP-NOW
    ESP_ERROR_CHECK(esp_now_init());
    ESP_LOGI(TAG, "✓ ESP-NOW inicializado");

    // Register receive callback
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
    ESP_LOGI(TAG, "✓ Callback ESP-NOW registrado");

    // Add broadcast peer (FF:FF:FF:FF:FF:FF)
    esp_now_peer_info_t peer = {0};
    peer.channel = ESPNOW_CHANNEL;  // Fixed channel 11
    peer.encrypt = false;
    memset(peer.peer_addr, 0xFF, 6);

    ESP_ERROR_CHECK(esp_now_add_peer(&peer));
    ESP_LOGI(TAG, "✓ Peer broadcast adicionado (canal %d)", ESPNOW_CHANNEL);
}

// ============================================================================
// GPIO INIT
// ============================================================================

static void gpio_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_BUILTIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // Initial LED state (off)
    gpio_set_level(LED_BUILTIN, 0);

    // Blink 3x fast
    for (int i = 0; i < 3; i++) {
        gpio_set_level(LED_BUILTIN, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(LED_BUILTIN, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    ESP_LOGI(TAG, "✓ GPIO inicializado (LED=%d)", LED_BUILTIN);
}

// ============================================================================
// HEARTBEAT TASK
// ============================================================================

static void heartbeat_task(void *pvParameters) {
    while (1) {
        // LED heartbeat (blink every 3 seconds)
        if (esp_timer_get_time() - last_heartbeat >= HEARTBEAT_INTERVAL_MS * 1000) {
            last_heartbeat = esp_timer_get_time();
            led_state = !led_state;
            gpio_set_level(LED_BUILTIN, led_state ? 1 : 0);
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================================================
// APP MAIN
// ============================================================================

void app_main(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔═══════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║       AGUADA Gateway v3.2 (ESP-IDF C)                    ║");
    ESP_LOGI(TAG, "║       ESP-NOW + WiFi → HTTP Bridge                       ║");
    ESP_LOGI(TAG, "║       Canal fixo 11 (otimizado para baixo consumo)       ║");
    ESP_LOGI(TAG, "╚═══════════════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");

    // Create packet queue
    espnow_queue = xQueueCreate(10, sizeof(espnow_packet_t));
    if (!espnow_queue) {
        ESP_LOGE(TAG, "Falha ao criar fila ESP-NOW");
        return;
    }
    ESP_LOGI(TAG, "✓ Fila ESP-NOW criada (10 slots)");

    // Initialize GPIO
    gpio_init();

    // Initialize WiFi (full STA mode for HTTP)
    wifi_init_sta();
    
    // Wait for WiFi connection
    ESP_LOGI(TAG, "Aguardando conexão WiFi...");
    vTaskDelay(pdMS_TO_TICKS(3000));

    // Initialize ESP-NOW (after WiFi is up)
    espnow_init();

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "✓ Gateway inicializado e pronto!");
    ESP_LOGI(TAG, "  - Canal ESP-NOW: %d (fixo)", ESPNOW_CHANNEL);
    ESP_LOGI(TAG, "  - Aguardando dados dos sensores...");
    ESP_LOGI(TAG, "");

    // Create heartbeat task
    xTaskCreate(heartbeat_task, "heartbeat", 2048, NULL, 5, NULL);

    // Create packet processing task (HTTP POST)
    xTaskCreate(packet_processing_task, "packet_proc", 4096, NULL, 5, NULL);

    // Keep main task alive
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
