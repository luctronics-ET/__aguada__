/**
 * AGUADA Gateway OTA v1.0
 * ESP32 DevKit v1 - Gateway/Repetidor com OTA
 *
 * Features:
 * - ESP-NOW Receiver (recebe dados dos nodes sensores)
 * - ESP-NOW Repeater (retransmite para outros gateways)
 * - WiFi HTTP POST para backend
 * - OTA (Over-The-Air) firmware update
 * - Mesh-ready (múltiplos gateways)
 * - Métricas e health check
 *
 * Hardware: ESP32 DevKit v1 (WROOM-32)
 * LED: GPIO 2 (onboard)
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
#include "esp_https_ota.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include "esp_system.h"

#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define TAG "AGUADA_GW_OTA"

// ============================================================================
// CONFIGURAÇÕES
// ============================================================================

// WiFi
#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PASS CONFIG_WIFI_PASSWORD

// Backend
#define BACKEND_HOST CONFIG_BACKEND_HOST
#define BACKEND_PORT CONFIG_BACKEND_PORT
#define TELEMETRY_ENDPOINT "/api/telemetry"
#define METRICS_ENDPOINT "/api/gateway/metrics"
#define OTA_ENDPOINT "/api/firmware/gateway"

// ESP-NOW
#define ESPNOW_CHANNEL CONFIG_ESPNOW_CHANNEL

// Gateway Mode
#define GATEWAY_MODE_PRIMARY 0  // Envia direto para backend
#define GATEWAY_MODE_REPEATER 1 // Retransmite para outro gateway
#define GATEWAY_MODE CONFIG_GATEWAY_MODE

// Timing
#define METRICS_INTERVAL_MS 60000
#define OTA_CHECK_INTERVAL_MS 300000 // 5 minutos
#define HEARTBEAT_INTERVAL_MS 3000
#define RETRY_BACKOFF_BASE_MS 1000

// Hardware (ESP32 DevKit v1)
#define LED_BUILTIN GPIO_NUM_2
#define MAX_PAYLOAD_SIZE 256
#define MAX_RETRY_ATTEMPTS 3

// Queue
#define QUEUE_SIZE 100
#define FALLBACK_BUFFER_SIZE 50

// Firmware version
#define FIRMWARE_VERSION "1.0.0"
#define GATEWAY_TYPE "OTA_01"

// ============================================================================
// TYPES
// ============================================================================

typedef struct
{
    uint8_t src_addr[6];
    char payload[MAX_PAYLOAD_SIZE];
    int len;
    int64_t recv_time;
    uint8_t hop_count; // Para mesh - quantos saltos o pacote deu
} espnow_packet_t;

typedef struct
{
    uint8_t mac[6];
    int rssi;
    int64_t last_seen;
    uint32_t packets_received;
    bool is_gateway; // True se for outro gateway (não sensor)
} peer_info_t;

// ============================================================================
// GLOBALS
// ============================================================================

static uint8_t gateway_mac[6];
static char gateway_mac_str[18];
static bool wifi_connected = false;
static int64_t last_heartbeat = 0;
static int64_t last_metrics_send = 0;
static int64_t last_ota_check = 0;
static bool led_state = false;
static QueueHandle_t espnow_queue = NULL;
static SemaphoreHandle_t http_mutex = NULL;

// Peers conhecidos (outros gateways para mesh)
#define MAX_PEERS 10
static peer_info_t known_peers[MAX_PEERS];
static int peer_count = 0;

// Buffer de fallback
static espnow_packet_t fallback_buffer[FALLBACK_BUFFER_SIZE];
static int fallback_buffer_count = 0;

// Métricas
static struct
{
    uint32_t packets_received;
    uint32_t packets_sent;
    uint32_t packets_failed;
    uint32_t packets_dropped;
    uint32_t packets_repeated; // Pacotes retransmitidos (mesh)
    uint32_t http_errors;
    uint32_t queue_full_count;
    uint32_t ota_checks;
    uint32_t ota_updates;
    int64_t last_packet_time;
    int64_t last_success_time;
    int64_t boot_time;
} metrics = {0};

// OTA
static bool ota_in_progress = false;
static char current_firmware_version[32] = FIRMWARE_VERSION;

// ============================================================================
// UTILITIES
// ============================================================================

static void mac_to_string(const uint8_t *mac, char *str)
{
    snprintf(str, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void build_url(char *url, size_t size, const char *endpoint)
{
    snprintf(url, size, "http://%s:%d%s", BACKEND_HOST, BACKEND_PORT, endpoint);
}

// ============================================================================
// PEER MANAGEMENT (Mesh support)
// ============================================================================

static peer_info_t *find_or_add_peer(const uint8_t *mac, bool is_gateway)
{
    // Procurar peer existente
    for (int i = 0; i < peer_count; i++)
    {
        if (memcmp(known_peers[i].mac, mac, 6) == 0)
        {
            known_peers[i].last_seen = esp_timer_get_time();
            known_peers[i].packets_received++;
            return &known_peers[i];
        }
    }

    // Adicionar novo peer
    if (peer_count < MAX_PEERS)
    {
        peer_info_t *peer = &known_peers[peer_count++];
        memcpy(peer->mac, mac, 6);
        peer->last_seen = esp_timer_get_time();
        peer->packets_received = 1;
        peer->is_gateway = is_gateway;

        char mac_str[18];
        mac_to_string(mac, mac_str);
        ESP_LOGI(TAG, "Novo peer: %s (%s)", mac_str, is_gateway ? "gateway" : "sensor");

        return peer;
    }

    return NULL;
}

// ============================================================================
// ESP-NOW CALLBACK
// ============================================================================

static void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    if (!recv_info || !espnow_queue || ota_in_progress)
    {
        return;
    }

    espnow_packet_t packet = {0};
    memcpy(packet.src_addr, recv_info->src_addr, 6);

    if (len >= MAX_PAYLOAD_SIZE)
    {
        len = MAX_PAYLOAD_SIZE - 1;
    }
    memcpy(packet.payload, data, len);
    packet.payload[len] = '\0';
    packet.len = len;
    packet.recv_time = esp_timer_get_time();
    packet.hop_count = 0; // TODO: extrair do payload se presente

    // Atualizar métricas
    metrics.packets_received++;
    metrics.last_packet_time = packet.recv_time;

    // Registrar peer
    find_or_add_peer(recv_info->src_addr, false);

    // Enqueue
    BaseType_t result = xQueueSendFromISR(espnow_queue, &packet, NULL);

    if (result != pdTRUE)
    {
        metrics.queue_full_count++;
        if (fallback_buffer_count < FALLBACK_BUFFER_SIZE)
        {
            memcpy(&fallback_buffer[fallback_buffer_count++], &packet, sizeof(espnow_packet_t));
            ESP_LOGW(TAG, "Queue cheia - buffer: %d", fallback_buffer_count);
        }
        else
        {
            metrics.packets_dropped++;
            ESP_LOGE(TAG, "Buffer cheio - pacote descartado!");
        }
    }
}

// ============================================================================
// ESP-NOW REPEATER (Mesh)
// ============================================================================

#if GATEWAY_MODE == GATEWAY_MODE_REPEATER

static uint8_t primary_gateway_mac[6] = CONFIG_PRIMARY_GATEWAY_MAC;

static void repeat_packet(espnow_packet_t *packet)
{
    if (packet->hop_count >= 3)
    {
        // Evitar loops infinitos
        ESP_LOGW(TAG, "Pacote atingiu hop limit");
        return;
    }

    // Incrementar hop count no payload (TODO: implementar formato)
    packet->hop_count++;

    esp_err_t result = esp_now_send(primary_gateway_mac,
                                    (uint8_t *)packet->payload,
                                    packet->len);

    if (result == ESP_OK)
    {
        metrics.packets_repeated++;
        ESP_LOGI(TAG, "→ Pacote repetido (hop %d)", packet->hop_count);
    }
    else
    {
        ESP_LOGE(TAG, "✗ Erro ao repetir: %s", esp_err_to_name(result));
    }
}

#endif

// ============================================================================
// HTTP POST
// ============================================================================

static bool http_post_json(const char *endpoint, const char *json, int len)
{
    if (!wifi_connected || ota_in_progress)
    {
        return false;
    }

    char url[128];
    build_url(url, sizeof(url), endpoint);

    bool success = false;

    if (xSemaphoreTake(http_mutex, pdMS_TO_TICKS(5000)) == pdTRUE)
    {
        esp_http_client_config_t config = {
            .url = url,
            .method = HTTP_METHOD_POST,
            .timeout_ms = 5000,
        };

        esp_http_client_handle_t client = esp_http_client_init(&config);
        if (client)
        {
            esp_http_client_set_header(client, "Content-Type", "application/json");
            esp_http_client_set_header(client, "X-Gateway-MAC", gateway_mac_str);
            esp_http_client_set_header(client, "X-Gateway-Version", FIRMWARE_VERSION);
            esp_http_client_set_post_field(client, json, len);

            esp_err_t err = esp_http_client_perform(client);
            if (err == ESP_OK)
            {
                int status = esp_http_client_get_status_code(client);
                if (status == 200 || status == 201)
                {
                    success = true;
                }
                else
                {
                    metrics.http_errors++;
                }
            }
            else
            {
                metrics.http_errors++;
            }
            esp_http_client_cleanup(client);
        }
        xSemaphoreGive(http_mutex);
    }

    return success;
}

// ============================================================================
// HTTP POST TASK
// ============================================================================

static void http_post_task(void *pvParameters)
{
    espnow_packet_t packet;

    while (1)
    {
        if (xQueueReceive(espnow_queue, &packet, pdMS_TO_TICKS(1000)))
        {
            char src_mac_str[18];
            mac_to_string(packet.src_addr, src_mac_str);

            ESP_LOGI(TAG, "");
            ESP_LOGI(TAG, "╔════════════════════════════════════════════════════╗");
            ESP_LOGI(TAG, "║ ✓ ESP-NOW de: %s (%d bytes)", src_mac_str, packet.len);
            ESP_LOGI(TAG, "╠════════════════════════════════════════════════════╣");
            ESP_LOGI(TAG, "║ %s", packet.payload);
            ESP_LOGI(TAG, "╚════════════════════════════════════════════════════╝");

#if GATEWAY_MODE == GATEWAY_MODE_REPEATER
            // Modo repetidor: retransmitir para gateway principal
            repeat_packet(&packet);
#else
            // Modo primário: enviar para backend
            if (!wifi_connected)
            {
                ESP_LOGW(TAG, "⚠ WiFi desconectado");
                continue;
            }

            bool success = false;
            for (int attempt = 0; attempt < MAX_RETRY_ATTEMPTS && !success; attempt++)
            {
                if (attempt > 0)
                {
                    int delay_ms = RETRY_BACKOFF_BASE_MS * (1 << (attempt - 1));
                    ESP_LOGW(TAG, "Retry %d/%d em %dms...", attempt + 1, MAX_RETRY_ATTEMPTS, delay_ms);
                    vTaskDelay(pdMS_TO_TICKS(delay_ms));
                }

                success = http_post_json(TELEMETRY_ENDPOINT, packet.payload, packet.len);

                if (success)
                {
                    metrics.packets_sent++;
                    metrics.last_success_time = esp_timer_get_time();
                    ESP_LOGI(TAG, "→ HTTP OK (tentativa %d)", attempt + 1);
                }
            }

            if (!success)
            {
                metrics.packets_failed++;
                ESP_LOGE(TAG, "✗ Falha após %d tentativas", MAX_RETRY_ATTEMPTS);
            }
#endif
        }
    }
}

// ============================================================================
// OTA UPDATE
// ============================================================================

static void check_ota_update(void)
{
    if (ota_in_progress || !wifi_connected)
    {
        return;
    }

    metrics.ota_checks++;
    ESP_LOGI(TAG, "Verificando atualização OTA...");

    // Primeiro, verificar se há nova versão disponível
    char url[128];
    build_url(url, sizeof(url), OTA_ENDPOINT);

    char check_url[192];
    snprintf(check_url, sizeof(check_url), "%s/check?mac=%s&version=%s&type=%s",
             url, gateway_mac_str, FIRMWARE_VERSION, GATEWAY_TYPE);

    esp_http_client_config_t config = {
        .url = check_url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client)
    {
        ESP_LOGE(TAG, "Falha ao criar cliente HTTP");
        return;
    }

    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Erro ao verificar OTA: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return;
    }

    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (status == 200)
    {
        // Nova versão disponível - iniciar OTA
        ESP_LOGI(TAG, "Nova versão disponível! Iniciando OTA...");
        ota_in_progress = true;

        // LED rápido durante OTA
        for (int i = 0; i < 10; i++)
        {
            gpio_set_level(LED_BUILTIN, i % 2);
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        char bin_url[192];
        snprintf(bin_url, sizeof(bin_url), "%s/download?type=%s", url, GATEWAY_TYPE);

        esp_http_client_config_t ota_config = {
            .url = bin_url,
            .timeout_ms = 60000, // 60 segundos para download
        };

        esp_https_ota_config_t ota_params = {
            .http_config = &ota_config,
        };

        err = esp_https_ota(&ota_params);

        if (err == ESP_OK)
        {
            metrics.ota_updates++;
            ESP_LOGI(TAG, "✓ OTA completo! Reiniciando...");
            vTaskDelay(pdMS_TO_TICKS(1000));
            esp_restart();
        }
        else
        {
            ESP_LOGE(TAG, "✗ OTA falhou: %s", esp_err_to_name(err));
            ota_in_progress = false;
        }
    }
    else if (status == 204)
    {
        ESP_LOGI(TAG, "Firmware atualizado (v%s)", FIRMWARE_VERSION);
    }
    else
    {
        ESP_LOGW(TAG, "OTA check retornou status %d", status);
    }
}

static void ota_task(void *pvParameters)
{
    // Aguardar conexão WiFi estável
    vTaskDelay(pdMS_TO_TICKS(30000));

    while (1)
    {
        if (wifi_connected && !ota_in_progress)
        {
            check_ota_update();
        }
        vTaskDelay(pdMS_TO_TICKS(OTA_CHECK_INTERVAL_MS));
    }
}

// ============================================================================
// METRICS TASK
// ============================================================================

static void metrics_task(void *pvParameters)
{
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(METRICS_INTERVAL_MS));

        if (!wifi_connected || ota_in_progress)
        {
            continue;
        }

        UBaseType_t queue_messages = uxQueueMessagesWaiting(espnow_queue);
        int queue_usage_percent = (queue_messages * 100) / QUEUE_SIZE;
        int64_t uptime_seconds = (esp_timer_get_time() - metrics.boot_time) / 1000000;

        char json[768];
        snprintf(json, sizeof(json),
                 "{"
                 "\"mac\":\"%s\","
                 "\"type\":\"%s\","
                 "\"version\":\"%s\","
                 "\"mode\":\"%s\","
                 "\"metrics\":{"
                 "\"packets_received\":%lu,"
                 "\"packets_sent\":%lu,"
                 "\"packets_failed\":%lu,"
                 "\"packets_dropped\":%lu,"
                 "\"packets_repeated\":%lu,"
                 "\"http_errors\":%lu,"
                 "\"queue_usage_percent\":%d,"
                 "\"peer_count\":%d,"
                 "\"ota_checks\":%lu,"
                 "\"ota_updates\":%lu,"
                 "\"uptime_seconds\":%lld"
                 "}"
                 "}",
                 gateway_mac_str,
                 GATEWAY_TYPE,
                 FIRMWARE_VERSION,
                 GATEWAY_MODE == GATEWAY_MODE_PRIMARY ? "primary" : "repeater",
                 metrics.packets_received,
                 metrics.packets_sent,
                 metrics.packets_failed,
                 metrics.packets_dropped,
                 metrics.packets_repeated,
                 metrics.http_errors,
                 queue_usage_percent,
                 peer_count,
                 metrics.ota_checks,
                 metrics.ota_updates,
                 uptime_seconds);

        if (http_post_json(METRICS_ENDPOINT, json, strlen(json)))
        {
            ESP_LOGI(TAG, "✓ Métricas enviadas");
        }
    }
}

// ============================================================================
// WIFI EVENT HANDLER
// ============================================================================

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "WiFi iniciando...");
        wifi_connected = false;
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGW(TAG, "WiFi desconectado, reconectando...");
        wifi_connected = false;
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "✓ WiFi conectado! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connected = true;

        // Reenviar buffer
        if (fallback_buffer_count > 0)
        {
            ESP_LOGI(TAG, "Reenviando %d pacotes do buffer...", fallback_buffer_count);
            for (int i = 0; i < fallback_buffer_count; i++)
            {
                if (xQueueSend(espnow_queue, &fallback_buffer[i], pdMS_TO_TICKS(100)) == pdTRUE)
                {
                    fallback_buffer_count--;
                    for (int j = i; j < fallback_buffer_count; j++)
                    {
                        fallback_buffer[j] = fallback_buffer[j + 1];
                    }
                    i--;
                }
            }
        }
    }
}

// ============================================================================
// INIT FUNCTIONS
// ============================================================================

static void wifi_init_sta(void)
{
    ESP_LOGI(TAG, "Inicializando WiFi...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MIN_MODEM));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));

    ESP_LOGI(TAG, "✓ WiFi inicializado (SSID: %s, Canal: %d)", WIFI_SSID, ESPNOW_CHANNEL);
}

static void espnow_init(void)
{
    ESP_LOGI(TAG, "Inicializando ESP-NOW...");

    esp_wifi_get_mac(WIFI_IF_STA, gateway_mac);
    mac_to_string(gateway_mac, gateway_mac_str);
    ESP_LOGI(TAG, "Gateway MAC: %s", gateway_mac_str);

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));

    // Broadcast peer
    esp_now_peer_info_t peer = {0};
    peer.channel = ESPNOW_CHANNEL;
    peer.encrypt = false;
    memset(peer.peer_addr, 0xFF, 6);
    ESP_ERROR_CHECK(esp_now_add_peer(&peer));

#if GATEWAY_MODE == GATEWAY_MODE_REPEATER
    // Adicionar gateway primário como peer
    esp_now_peer_info_t primary = {0};
    primary.channel = ESPNOW_CHANNEL;
    primary.encrypt = false;
    memcpy(primary.peer_addr, primary_gateway_mac, 6);
    ESP_ERROR_CHECK(esp_now_add_peer(&primary));

    char mac_str[18];
    mac_to_string(primary_gateway_mac, mac_str);
    ESP_LOGI(TAG, "✓ Gateway primário configurado: %s", mac_str);
#endif

    ESP_LOGI(TAG, "✓ ESP-NOW inicializado (canal %d)", ESPNOW_CHANNEL);
}

static void gpio_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_BUILTIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(LED_BUILTIN, 0);

    // Boot blink
    for (int i = 0; i < 5; i++)
    {
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

static void heartbeat_task(void *pvParameters)
{
    while (1)
    {
        if (!ota_in_progress)
        {
            led_state = !led_state;
            gpio_set_level(LED_BUILTIN, led_state ? 1 : 0);
        }
        vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_INTERVAL_MS));
    }
}

// ============================================================================
// APP MAIN
// ============================================================================

void app_main(void)
{
    metrics.boot_time = esp_timer_get_time();

    // Print firmware info
    const esp_app_desc_t *app_desc = esp_app_get_description();

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔═══════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║       AGUADA Gateway OTA v%s                          ║", FIRMWARE_VERSION);
    ESP_LOGI(TAG, "║       ESP32 DevKit v1 - Gateway/Repetidor               ║");
    ESP_LOGI(TAG, "║       Modo: %s                                    ║",
             GATEWAY_MODE == GATEWAY_MODE_PRIMARY ? "PRIMÁRIO  " : "REPETIDOR ");
    ESP_LOGI(TAG, "╠═══════════════════════════════════════════════════════════╣");
    ESP_LOGI(TAG, "║  IDF: %s                                          ║", app_desc->idf_ver);
    ESP_LOGI(TAG, "║  Compilado: %s %s                       ║", app_desc->date, app_desc->time);
    ESP_LOGI(TAG, "╚═══════════════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");

    // Create resources
    espnow_queue = xQueueCreate(QUEUE_SIZE, sizeof(espnow_packet_t));
    if (!espnow_queue)
    {
        ESP_LOGE(TAG, "Falha ao criar queue");
        return;
    }

    http_mutex = xSemaphoreCreateMutex();
    if (!http_mutex)
    {
        ESP_LOGE(TAG, "Falha ao criar mutex");
        return;
    }

    ESP_LOGI(TAG, "✓ Queue criada (%d slots)", QUEUE_SIZE);

    // Initialize
    gpio_init();
    wifi_init_sta();

    vTaskDelay(pdMS_TO_TICKS(3000));

    espnow_init();

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "✓ Gateway inicializado!");
    ESP_LOGI(TAG, "  - Versão: %s", FIRMWARE_VERSION);
    ESP_LOGI(TAG, "  - Canal ESP-NOW: %d", ESPNOW_CHANNEL);
    ESP_LOGI(TAG, "  - OTA: Ativo (check a cada %d min)", OTA_CHECK_INTERVAL_MS / 60000);
    ESP_LOGI(TAG, "  - Aguardando dados...");
    ESP_LOGI(TAG, "");

    // Create tasks
    xTaskCreate(heartbeat_task, "heartbeat", 2048, NULL, 5, NULL);
    xTaskCreate(http_post_task, "http_post", 8192, NULL, 5, NULL);
    xTaskCreate(metrics_task, "metrics", 4096, NULL, 3, NULL);
    xTaskCreate(ota_task, "ota", 8192, NULL, 2, NULL);

    // Main loop
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
