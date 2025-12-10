/**
 * @file main.c
 * @brief AGUADA Gateway USB v2.0 - Bridge ESP-NOW para Serial
 *
 * Firmware mínimo para gateway ESP32-C3 SuperMini:
 * - Recebe pacotes ESP-NOW de todos os nodes (broadcast)
 * - Envia JSON via USB Serial (printf)
 * - LED indica recepção
 * - NÃO precisa de WiFi/rede!
 *
 * Conexão: USB-C direto no computador
 * Serial: 115200 baud (padrão ESP-IDF)
 *
 * @author AGUADA Project
 * @date 2025-12-07
 * @version 2.0.0
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

// ============================================================================
// CONFIGURAÇÃO
// ============================================================================

// Versão do firmware
#define FIRMWARE_VERSION "v2.0.0"
#define FIRMWARE_NAME "AGUADA Gateway USB"

// GPIO para ESP32-C3 SuperMini
#define GPIO_LED GPIO_NUM_8 // LED interno do C3 SuperMini

// ESP-NOW
#define ESPNOW_CHANNEL 11   // Mesmo canal dos sensores!
#define MAX_PACKET_SIZE 250 // Tamanho máximo do pacote

// Queue
#define QUEUE_SIZE 30 // Tamanho da fila de pacotes

// ============================================================================
// ESTRUTURAS
// ============================================================================

typedef struct
{
    uint8_t mac[6];
    uint8_t data[MAX_PACKET_SIZE];
    int len;
    int rssi;
    int64_t timestamp;
} espnow_packet_t;

// ============================================================================
// VARIÁVEIS GLOBAIS
// ============================================================================

static const char *TAG = "GW_USB";
static QueueHandle_t packet_queue;
static char gateway_mac_str[18];

// Estatísticas
static uint32_t packets_received = 0;
static uint32_t packets_processed = 0;
static uint32_t packets_dropped = 0;

// ============================================================================
// FUNÇÕES ESP-NOW
// ============================================================================

/**
 * @brief Callback de recepção ESP-NOW (ESP-IDF 6.x signature)
 */
static void espnow_recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    if (len <= 0 || len > MAX_PACKET_SIZE)
    {
        ESP_LOGW(TAG, "Pacote inválido: len=%d", len);
        return;
    }

    packets_received++;

    // Pisca LED
    gpio_set_level(GPIO_LED, 1);

    // Cria pacote para a fila
    espnow_packet_t pkt;
    memcpy(pkt.mac, info->src_addr, 6);
    memcpy(pkt.data, data, len);
    pkt.len = len;
    pkt.rssi = info->rx_ctrl->rssi;
    pkt.timestamp = esp_timer_get_time();

    // Envia para a fila (não bloqueia)
    if (xQueueSend(packet_queue, &pkt, 0) != pdTRUE)
    {
        packets_dropped++;
        ESP_LOGW(TAG, "Queue cheia, pacote descartado (drops=%lu)", (unsigned long)packets_dropped);
    }
}

/**
 * @brief Inicializa ESP-NOW (modo receptor)
 */
static esp_err_t init_espnow(void)
{
    ESP_LOGI(TAG, "Iniciando WiFi para ESP-NOW...");

    // Cria netif e event loop (requerido no ESP-IDF 6.x)
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Inicializa WiFi em modo STA (necessário para ESP-NOW)
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    // Configura protocolo WiFi
    ESP_ERROR_CHECK(esp_wifi_set_protocol(WIFI_IF_STA,
                                          WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N));

    ESP_ERROR_CHECK(esp_wifi_start());

    // Espera WiFi iniciar
    vTaskDelay(pdMS_TO_TICKS(100));

    // Configura canal fixo (IMPORTANTE: mesmo canal dos sensores!)
    ESP_ERROR_CHECK(esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));
    ESP_LOGI(TAG, "Canal ESP-NOW configurado: %d", ESPNOW_CHANNEL);

    // Inicializa ESP-NOW
    ESP_ERROR_CHECK(esp_now_init());

    // Registra callback de recepção
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));

    ESP_LOGI(TAG, "ESP-NOW inicializado com sucesso");
    return ESP_OK;
}

// ============================================================================
// TASK DE PROCESSAMENTO SERIAL
// ============================================================================

/**
 * @brief Task que processa pacotes e envia via Serial USB
 *
 * Formato de saída:
 * - Se JSON válido: adiciona rssi e imprime
 * - Se não-JSON: encapsula em JSON
 */
static void serial_task(void *pvParameters)
{
    espnow_packet_t pkt;

    ESP_LOGI(TAG, "Serial bridge task iniciada");

    while (1)
    {
        // Aguarda pacote na fila
        if (xQueueReceive(packet_queue, &pkt, portMAX_DELAY) == pdTRUE)
        {
            // Formata MAC do sender
            char sender_mac[18];
            snprintf(sender_mac, sizeof(sender_mac),
                     "%02X:%02X:%02X:%02X:%02X:%02X",
                     pkt.mac[0], pkt.mac[1], pkt.mac[2],
                     pkt.mac[3], pkt.mac[4], pkt.mac[5]);

            // Garante null-termination
            pkt.data[pkt.len] = '\0';

            // Log de debug
            ESP_LOGD(TAG, "RX de %s: %s (rssi=%d)", sender_mac, (char *)pkt.data, pkt.rssi);

            // Verifica se é JSON válido (começa com {)
            if (pkt.data[0] == '{')
            {
                // JSON recebido - adiciona rssi se não existir
                char *existing_rssi = strstr((char *)pkt.data, "\"rssi\"");

                if (existing_rssi == NULL)
                {
                    // Remove o } final para adicionar rssi
                    char *end_brace = strrchr((char *)pkt.data, '}');
                    if (end_brace)
                    {
                        *end_brace = '\0';
                    }
                    // Envia JSON com RSSI adicionado
                    printf("%s,\"rssi\":%d}\n", (char *)pkt.data, pkt.rssi);
                }
                else
                {
                    // JSON já tem rssi, envia como está
                    printf("%s\n", (char *)pkt.data);
                }
            }
            else
            {
                // Dados não-JSON: encapsula em JSON
                printf("{\"mac\":\"%s\",\"raw\":\"%.*s\",\"rssi\":%d}\n",
                       sender_mac, pkt.len, pkt.data, pkt.rssi);
            }

            // Flush para garantir envio imediato
            fflush(stdout);

            packets_processed++;

            // LED OFF após processar
            gpio_set_level(GPIO_LED, 0);
        }
    }
}

/**
 * @brief Task de status periódico (a cada 60s)
 */
static void status_task(void *pvParameters)
{
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(60000)); // 1 minuto

        int64_t uptime_s = esp_timer_get_time() / 1000000;

        // Envia status do gateway via Serial
        printf("{\"mac\":\"%s\",\"type\":\"gateway_status\","
               "\"rx\":%lu,\"proc\":%lu,\"drops\":%lu,\"uptime\":%lld,"
               "\"channel\":%d,\"version\":\"%s\"}\n",
               gateway_mac_str,
               (unsigned long)packets_received,
               (unsigned long)packets_processed,
               (unsigned long)packets_dropped,
               (long long)uptime_s,
               ESPNOW_CHANNEL,
               FIRMWARE_VERSION);
        fflush(stdout);

        ESP_LOGI(TAG, "Status: rx=%lu proc=%lu drops=%lu uptime=%llds",
                 (unsigned long)packets_received,
                 (unsigned long)packets_processed,
                 (unsigned long)packets_dropped,
                 (long long)uptime_s);
    }
}

// ============================================================================
// FUNÇÕES DE INICIALIZAÇÃO
// ============================================================================

/**
 * @brief Inicializa GPIO do LED
 */
static void init_gpio(void)
{
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << GPIO_LED),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);
    gpio_set_level(GPIO_LED, 0);

    ESP_LOGI(TAG, "GPIO configurado: LED=%d", GPIO_LED);
}

/**
 * @brief Obtém e exibe MAC do gateway
 */
static void get_gateway_mac(void)
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(gateway_mac_str, sizeof(gateway_mac_str),
             "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  GATEWAY MAC: %s", gateway_mac_str);
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Configure este MAC nos sensores!");
}

// ============================================================================
// MAIN
// ============================================================================

void app_main(void)
{
    // Banner
    ESP_LOGI(TAG, "╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  %s %s  ║", FIRMWARE_NAME, FIRMWARE_VERSION);
    ESP_LOGI(TAG, "║  ESP32-C3 SuperMini                    ║");
    ESP_LOGI(TAG, "║  Canal ESP-NOW: %d                      ║", ESPNOW_CHANNEL);
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝");

    // Inicializa NVS (necessário para WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Inicializa componentes
    init_gpio();
    get_gateway_mac();

    // LED pisca 5x no boot (padrão gateway)
    for (int i = 0; i < 5; i++)
    {
        gpio_set_level(GPIO_LED, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(GPIO_LED, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Cria fila de pacotes
    packet_queue = xQueueCreate(QUEUE_SIZE, sizeof(espnow_packet_t));
    if (packet_queue == NULL)
    {
        ESP_LOGE(TAG, "Erro ao criar queue de pacotes!");
        return;
    }

    // Inicializa ESP-NOW
    ESP_ERROR_CHECK(init_espnow());

    // Cria tasks
    xTaskCreate(serial_task, "serial_task", 8192, NULL, 5, NULL);
    xTaskCreate(status_task, "status_task", 4096, NULL, 3, NULL);

    // Mensagem inicial via Serial (para backend detectar gateway)
    printf("{\"mac\":\"%s\",\"type\":\"gateway_boot\",\"channel\":%d,\"version\":\"%s\"}\n",
           gateway_mac_str, ESPNOW_CHANNEL, FIRMWARE_VERSION);
    fflush(stdout);

    ESP_LOGI(TAG, "Gateway USB pronto!");
    ESP_LOGI(TAG, "Aguardando pacotes ESP-NOW no canal %d...", ESPNOW_CHANNEL);
    ESP_LOGI(TAG, "Conecte USB ao computador e execute o backend");
}
