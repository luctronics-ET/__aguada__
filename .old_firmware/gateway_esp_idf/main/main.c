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
#define METRICS_URL "http://192.168.0.117:3000/api/gateway/metrics"
#define ESPNOW_CHANNEL 11  // Fixed channel for SSID "luciano"
#define METRICS_INTERVAL_MS 60000  // Enviar métricas a cada 60 segundos
#define LED_BUILTIN GPIO_NUM_2  // ESP32 DevKit V1 uses GPIO2 for LED (GPIO8 is invalid on ESP32)
#define HEARTBEAT_INTERVAL_MS 3000
#define MAX_PAYLOAD_SIZE 256
#define MAX_RETRY_ATTEMPTS 3
#define RETRY_BACKOFF_BASE_MS 1000  // 1s, 2s, 4s

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
// Note: last_metrics_send removed - metrics_task uses vTaskDelay instead
static bool led_state = false;
static QueueHandle_t espnow_queue = NULL;

// Buffer para pacotes quando queue está cheia (fallback em memória)
// TODO: Implementar SPIFFS/LittleFS para persistência real
#define FALLBACK_BUFFER_SIZE 20
static espnow_packet_t fallback_buffer[FALLBACK_BUFFER_SIZE];
static int fallback_buffer_count = 0;

// Métricas do gateway
static struct {
    uint32_t packets_received;      // Total de pacotes ESP-NOW recebidos
    uint32_t packets_sent;          // Total de pacotes HTTP enviados com sucesso
    uint32_t packets_failed;        // Total de pacotes HTTP que falharam
    uint32_t packets_dropped;       // Total de pacotes descartados
    uint32_t http_errors;           // Total de erros HTTP
    uint32_t queue_full_count;      // Vezes que a queue ficou cheia
    int64_t last_packet_time;       // Timestamp do último pacote recebido
    int64_t last_success_time;      // Timestamp do último envio bem-sucedido
} gateway_metrics = {0};

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

    // Atualizar métricas
    gateway_metrics.packets_received++;
    gateway_metrics.last_packet_time = esp_timer_get_time();
    
    // Enqueue without blocking
    BaseType_t result = xQueueSendFromISR(espnow_queue, &packet, NULL);
    
    // Se queue cheia, tentar salvar em buffer de fallback
    if (result != pdTRUE) {
        gateway_metrics.queue_full_count++;
        if (fallback_buffer_count < FALLBACK_BUFFER_SIZE) {
            memcpy(&fallback_buffer[fallback_buffer_count], &packet, sizeof(espnow_packet_t));
            fallback_buffer_count++;
            ESP_LOGW(TAG, "Queue cheia - pacote salvo em buffer (total: %d)", fallback_buffer_count);
        } else {
            gateway_metrics.packets_dropped++;
            ESP_LOGE(TAG, "Queue e buffer cheios - pacote descartado!");
        }
    }
}

// ============================================================================
// HTTP POST TASK (Process queue and forward to backend)
// ============================================================================

static void http_post_task(void *pvParameters) {
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
            
            // Output JSON to Serial for backend Serial Bridge
            // Use printf for clean output without ESP-IDF prefix
            printf("TELEMETRY:%s\n", packet.payload);
            fflush(stdout);

            // Skip HTTP POST if WiFi not connected
            if (!wifi_connected) {
                ESP_LOGW(TAG, "⚠ WiFi desconectado - usando Serial Bridge");
                continue;
            }

            // Retry com backoff exponencial
            bool success = false;
            for (int attempt = 0; attempt < MAX_RETRY_ATTEMPTS && !success; attempt++) {
                if (attempt > 0) {
                    // Backoff exponencial: 1s, 2s, 4s
                    int delay_ms = RETRY_BACKOFF_BASE_MS * (1 << (attempt - 1));
                    ESP_LOGW(TAG, "Tentativa %d/%d após %dms...", attempt + 1, MAX_RETRY_ATTEMPTS, delay_ms);
                    vTaskDelay(pdMS_TO_TICKS(delay_ms));
                }

                // Make HTTP POST request (timeout increased to 5s for better reliability)
                esp_http_client_config_t config = {
                    .url = BACKEND_URL,
                    .method = HTTP_METHOD_POST,
                    .timeout_ms = 5000,  // 5 seconds (increased for better reliability)
                };
                
                esp_http_client_handle_t client = esp_http_client_init(&config);
                if (client) {
                    esp_http_client_set_header(client, "Content-Type", "application/json");
                    esp_http_client_set_post_field(client, packet.payload, packet.len);
                    
                    esp_err_t err = esp_http_client_perform(client);
                    if (err == ESP_OK) {
                        int status = esp_http_client_get_status_code(client);
                        if (status == 200 || status == 201) {
                            gateway_metrics.packets_sent++;
                            gateway_metrics.last_success_time = esp_timer_get_time();
                            ESP_LOGI(TAG, "→ Enviado via HTTP (status=%d, tentativa %d)", status, attempt + 1);
                            success = true;
                        } else {
                            gateway_metrics.http_errors++;
                            ESP_LOGW(TAG, "✗ HTTP status=%d (tentativa %d)", status, attempt + 1);
                        }
                    } else {
                        gateway_metrics.http_errors++;
                        ESP_LOGW(TAG, "✗ HTTP error: %s (tentativa %d)", esp_err_to_name(err), attempt + 1);
                    }
                    esp_http_client_cleanup(client);
                } else {
                    ESP_LOGW(TAG, "✗ Falha ao criar cliente HTTP (tentativa %d)", attempt + 1);
                }
            }

            if (!success) {
                gateway_metrics.packets_failed++;
                ESP_LOGE(TAG, "✗ Falha ao enviar após %d tentativas - pacote descartado", MAX_RETRY_ATTEMPTS);
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
        
        // Tentar reenviar pacotes do buffer de fallback
        if (fallback_buffer_count > 0) {
            ESP_LOGI(TAG, "Reenviando %d pacotes do buffer...", fallback_buffer_count);
            for (int i = 0; i < fallback_buffer_count; i++) {
                // Tentar adicionar à queue novamente
                if (xQueueSend(espnow_queue, &fallback_buffer[i], pdMS_TO_TICKS(100)) == pdTRUE) {
                    fallback_buffer_count--;
                    // Shift array
                    for (int j = i; j < fallback_buffer_count; j++) {
                        fallback_buffer[j] = fallback_buffer[j + 1];
                    }
                    i--; // Re-check this index
                }
            }
            if (fallback_buffer_count > 0) {
                ESP_LOGW(TAG, "%d pacotes ainda no buffer após reconexão", fallback_buffer_count);
            }
        }
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
    
    // Note: Channel is set automatically by the AP when connected
    // ESP-NOW will use the same channel as WiFi STA
    // Don't call esp_wifi_set_channel() here - it fails during STA connection
    
    // Reduce WiFi TX power to 15 dBm (60 = 15dBm, default is 80 = 20dBm)
    // Gateway is close to AP, doesn't need max power
    // MUST be called AFTER esp_wifi_start()
    esp_err_t tx_err = esp_wifi_set_max_tx_power(60);
    if (tx_err != ESP_OK) {
        ESP_LOGW(TAG, "Não foi possível reduzir TX power: %d", tx_err);
    }

    ESP_LOGI(TAG, "✓ WiFi inicializado (SSID: %s)", WIFI_SSID);
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
    peer.channel = 0;  // Use current WiFi channel (set by AP)
    peer.encrypt = false;
    memset(peer.peer_addr, 0xFF, 6);

    ESP_ERROR_CHECK(esp_now_add_peer(&peer));
    ESP_LOGI(TAG, "✓ Peer broadcast adicionado (canal automático)");
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
// METRICS TASK (Send metrics to backend periodically)
// ============================================================================

static void metrics_task(void *pvParameters) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(METRICS_INTERVAL_MS));
        
        if (!wifi_connected) {
            continue;
        }

        // Calcular uso da queue
        UBaseType_t queue_messages = uxQueueMessagesWaiting(espnow_queue);
        int queue_usage_percent = (queue_messages * 100) / 50; // 50 é o tamanho da queue

        // Preparar JSON de métricas
        char metrics_json[512];
        snprintf(metrics_json, sizeof(metrics_json),
            "{"
            "\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\","
            "\"metrics\":{"
            "\"packets_received\":%lu,"
            "\"packets_sent\":%lu,"
            "\"packets_failed\":%lu,"
            "\"packets_dropped\":%lu,"
            "\"http_errors\":%lu,"
            "\"queue_full_count\":%lu,"
            "\"queue_usage_percent\":%d,"
            "\"wifi_connected\":%s,"
            "\"last_packet_time\":%lld,"
            "\"last_success_time\":%lld,"
            "\"uptime_seconds\":%lld"
            "}"
            "}",
            gateway_mac[0], gateway_mac[1], gateway_mac[2], gateway_mac[3], gateway_mac[4], gateway_mac[5],
            gateway_metrics.packets_received,
            gateway_metrics.packets_sent,
            gateway_metrics.packets_failed,
            gateway_metrics.packets_dropped,
            gateway_metrics.http_errors,
            gateway_metrics.queue_full_count,
            queue_usage_percent,
            wifi_connected ? "true" : "false",
            gateway_metrics.last_packet_time / 1000000, // Converter para segundos
            gateway_metrics.last_success_time / 1000000,
            esp_timer_get_time() / 1000000
        );

        // Enviar métricas via HTTP POST
        esp_http_client_config_t config = {
            .url = METRICS_URL,
            .method = HTTP_METHOD_POST,
            .timeout_ms = 3000,
        };
        
        esp_http_client_handle_t client = esp_http_client_init(&config);
        if (client) {
            esp_http_client_set_header(client, "Content-Type", "application/json");
            esp_http_client_set_post_field(client, metrics_json, strlen(metrics_json));
            
            esp_err_t err = esp_http_client_perform(client);
            if (err == ESP_OK) {
                int status = esp_http_client_get_status_code(client);
                if (status == 200 || status == 201) {
                    ESP_LOGI(TAG, "✓ Métricas enviadas");
                }
            }
            esp_http_client_cleanup(client);
        }
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

    // Create packet queue (increased to 50 slots for better buffering)
    espnow_queue = xQueueCreate(50, sizeof(espnow_packet_t));
    if (!espnow_queue) {
        ESP_LOGE(TAG, "Falha ao criar fila ESP-NOW");
        return;
    }
    ESP_LOGI(TAG, "✓ Fila ESP-NOW criada (50 slots)");

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

    // Create HTTP POST task (process queue and forward data)
    xTaskCreate(http_post_task, "http_post", 4096, NULL, 5, NULL);

    // Create metrics task (send metrics periodically)
    xTaskCreate(metrics_task, "metrics", 4096, NULL, 3, NULL);

    // Keep main task alive
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
