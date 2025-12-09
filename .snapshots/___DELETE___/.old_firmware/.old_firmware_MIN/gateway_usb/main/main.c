/**
 * @file main.c
 * @brief AGUADA Gateway USB - Bridge ESP-NOW para Serial
 *
 * Firmware mínimo para gateway:
 * - Recebe pacotes ESP-NOW de todos os nodes
 * - Envia JSON via USB Serial
 * - LED indica recepção
 * - NÃO precisa de WiFi/rede!
 *
 * @author AGUADA Project
 * @date 2025-12-06
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

// GPIO (ESP32 DevKit v1)
#define GPIO_LED GPIO_NUM_2 // LED builtin no ESP32 DevKit

// ESP-NOW
#define ESPNOW_CHANNEL 11   // Canal WiFi/ESP-NOW
#define MAX_PACKET_SIZE 250 // Tamanho máximo do pacote

// Queue
#define QUEUE_SIZE 20 // Tamanho da fila de pacotes

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

// ============================================================================
// FUNÇÕES ESP-NOW
// ============================================================================

/**
 * @brief Callback de recepção ESP-NOW
 */
static void espnow_recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    if (len <= 0 || len > MAX_PACKET_SIZE)
    {
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
        ESP_LOGW(TAG, "Queue cheia, pacote descartado");
    }
}

/**
 * @brief Inicializa ESP-NOW
 */
static esp_err_t init_espnow(void)
{
    ESP_LOGI(TAG, "Iniciando netif...");

    // Cria netif e event loop (requerido no ESP-IDF 6.x)
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_LOGI(TAG, "Iniciando WiFi...");
    vTaskDelay(pdMS_TO_TICKS(200));

    // Inicializa WiFi em modo STA (necessário para ESP-NOW)
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    vTaskDelay(pdMS_TO_TICKS(200));

    ESP_LOGI(TAG, "Configurando modo STA...");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // Reduz potência TX para evitar brownout durante inicialização
    esp_wifi_set_max_tx_power(20); // 20 = 5dBm (mínimo para ESP-NOW local)

    ESP_ERROR_CHECK(esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N));

    vTaskDelay(pdMS_TO_TICKS(200));

    ESP_LOGI(TAG, "Iniciando WiFi...");
    ESP_ERROR_CHECK(esp_wifi_start());

    vTaskDelay(pdMS_TO_TICKS(500));

    ESP_LOGI(TAG, "Configurando canal %d...", ESPNOW_CHANNEL);
    // Configura canal fixo
    ESP_ERROR_CHECK(esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));

    vTaskDelay(pdMS_TO_TICKS(200));

    ESP_LOGI(TAG, "Iniciando ESP-NOW...");
    // Inicializa ESP-NOW
    ESP_ERROR_CHECK(esp_now_init());

    ESP_LOGI(TAG, "Registrando callback...");
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));

    ESP_LOGI(TAG, "ESP-NOW RX iniciado (canal %d)", ESPNOW_CHANNEL);
    return ESP_OK;
}

// ============================================================================
// TASK DE PROCESSAMENTO
// ============================================================================

/**
 * @brief Task que processa pacotes e envia via Serial
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

            // Verifica se é JSON válido (começa com {)
            if (pkt.data[0] == '{')
            {
                // Remove o } final se existir para adicionar rssi
                char *end_brace = strrchr((char *)pkt.data, '}');
                if (end_brace)
                {
                    *end_brace = '\0';
                }

                // Envia JSON enriquecido com RSSI via Serial
                // Formato: JSON original + rssi + }
                printf("%s,\"rssi\":%d}\n", (char *)pkt.data, pkt.rssi);
            }
            else
            {
                // Dados não-JSON: encapsula
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
 * @brief Task de status periódico
 */
static void status_task(void *pvParameters)
{
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(60000)); // A cada 1 minuto

        int64_t uptime_s = esp_timer_get_time() / 1000000;

        // Envia status do gateway
        printf("{\"mac\":\"%s\",\"type\":\"gateway_status\","
               "\"rx\":%lu,\"proc\":%lu,\"uptime\":%lld}\n",
               gateway_mac_str,
               (unsigned long)packets_received,
               (unsigned long)packets_processed,
               (long long)uptime_s);
        fflush(stdout);
    }
}

// ============================================================================
// FUNÇÕES DE INICIALIZAÇÃO
// ============================================================================

/**
 * @brief Inicializa GPIO
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

    ESP_LOGI(TAG, "GPIO: LED=%d", GPIO_LED);
}

/**
 * @brief Obtém MAC do gateway
 */
static void get_gateway_mac(void)
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(gateway_mac_str, sizeof(gateway_mac_str),
             "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    ESP_LOGI(TAG, "Gateway MAC: %s", gateway_mac_str);
}

// ============================================================================
// MAIN
// ============================================================================

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  AGUADA Gateway USB v1.0");
    ESP_LOGI(TAG, "========================================");

    // Inicializa NVS
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

    // LED pisca 5x no boot (diferente do node)
    for (int i = 0; i < 5; i++)
    {
        gpio_set_level(GPIO_LED, 1);
        vTaskDelay(pdMS_TO_TICKS(50));
        gpio_set_level(GPIO_LED, 0);
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    // Cria fila de pacotes
    packet_queue = xQueueCreate(QUEUE_SIZE, sizeof(espnow_packet_t));
    if (packet_queue == NULL)
    {
        ESP_LOGE(TAG, "Erro ao criar queue");
        return;
    }

    // Inicializa ESP-NOW
    ESP_ERROR_CHECK(init_espnow());

    // Cria tasks
    xTaskCreate(serial_task, "serial_task", 8192, NULL, 5, NULL);
    xTaskCreate(status_task, "status_task", 4096, NULL, 3, NULL);

    // Mensagem inicial via Serial
    printf("{\"mac\":\"%s\",\"type\":\"gateway_boot\",\"channel\":%d}\n",
           gateway_mac_str, ESPNOW_CHANNEL);
    fflush(stdout);

    ESP_LOGI(TAG, "Gateway USB pronto! Aguardando pacotes ESP-NOW...");
}
