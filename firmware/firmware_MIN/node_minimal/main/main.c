/**
 * @file main.c
 * @brief AGUADA Node Minimal - Sensor ultrassônico com ESP-NOW
 *
 * Firmware mínimo para ESP32-C3 SuperMini:
 * - 1x Sensor ultrassônico AJ-SR04M
 * - 1x LED builtin (status)
 * - ESP-NOW TX (envio para gateway)
 * - Transmissão inteligente: delta > 2cm OU timeout 5min
 *
 * @author AGUADA Project
 * @date 2025-12-06
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

// ============================================================================
// CONFIGURAÇÃO - Ajustar conforme hardware
// ============================================================================

// GPIO do ESP32-C3 SuperMini
#define GPIO_TRIG GPIO_NUM_1 // Trigger do ultrassônico
#define GPIO_ECHO GPIO_NUM_0 // Echo do ultrassônico
#define GPIO_LED GPIO_NUM_8  // LED builtin (azul)

// Parâmetros de transmissão
#define DEADBAND_CM 2                // Variação mínima para enviar (cm)
#define HEARTBEAT_MS (5 * 60 * 1000) // Heartbeat a cada 5 minutos
#define SAMPLE_INTERVAL_MS 1000      // Intervalo entre amostras (1s)

// Ultrassônico
#define SOUND_SPEED_CM_US 0.0343f // Velocidade do som em cm/μs
#define TIMEOUT_US 30000          // Timeout de 30ms (~5m max)
#define VALUE_MULTIPLIER 100      // Multiplicador (cm → int)

// ESP-NOW
#define ESPNOW_CHANNEL 11 // Canal WiFi/ESP-NOW

// ============================================================================
// VARIÁVEIS GLOBAIS
// ============================================================================

static const char *TAG = "NODE_MIN";

// MAC do gateway (será descoberto ou configurado)
static uint8_t gateway_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Broadcast inicial

// Estado
static int32_t last_distance_cm = -1; // Última distância enviada (x100)
static int64_t last_send_time = 0;    // Timestamp do último envio
static char node_mac_str[18];         // MAC do node como string
static uint8_t node_mac[6];           // MAC do node

// Estatísticas
static uint32_t packets_sent = 0;
static uint32_t packets_failed = 0;

// ============================================================================
// FUNÇÕES DO ULTRASSÔNICO
// ============================================================================

/**
 * @brief Lê distância do sensor ultrassônico
 * @return Distância em cm x 100, ou -1 em caso de erro
 */
static int32_t read_ultrasonic(void)
{
    // Trigger: pulso de 10μs
    gpio_set_level(GPIO_TRIG, 0);
    esp_rom_delay_us(2);
    gpio_set_level(GPIO_TRIG, 1);
    esp_rom_delay_us(10);
    gpio_set_level(GPIO_TRIG, 0);

    // Aguarda início do echo (HIGH)
    int64_t start_wait = esp_timer_get_time();
    while (gpio_get_level(GPIO_ECHO) == 0)
    {
        if ((esp_timer_get_time() - start_wait) > TIMEOUT_US)
        {
            return -1; // Timeout esperando echo
        }
    }

    // Mede duração do pulso echo
    int64_t pulse_start = esp_timer_get_time();
    while (gpio_get_level(GPIO_ECHO) == 1)
    {
        if ((esp_timer_get_time() - pulse_start) > TIMEOUT_US)
        {
            return -1; // Timeout durante echo
        }
    }
    int64_t pulse_end = esp_timer_get_time();

    // Calcula distância: (tempo * velocidade) / 2
    uint32_t duration_us = (uint32_t)(pulse_end - pulse_start);
    float distance_cm = (duration_us * SOUND_SPEED_CM_US) / 2.0f;

    // Converte para inteiro (x100) e valida range
    int32_t distance_x100 = (int32_t)(distance_cm * VALUE_MULTIPLIER);

    // Valida range (2cm - 400cm)
    if (distance_x100 < 200 || distance_x100 > 40000)
    {
        return -1;
    }

    return distance_x100;
}

/**
 * @brief Lê distância com filtro mediana (5 amostras)
 */
static int32_t read_ultrasonic_filtered(void)
{
    int32_t samples[5];
    int valid = 0;

    for (int i = 0; i < 5; i++)
    {
        int32_t d = read_ultrasonic();
        if (d > 0)
        {
            samples[valid++] = d;
        }
        vTaskDelay(pdMS_TO_TICKS(50)); // 50ms entre amostras
    }

    if (valid < 3)
    {
        return -1; // Menos de 3 amostras válidas
    }

    // Ordena para mediana
    for (int i = 0; i < valid - 1; i++)
    {
        for (int j = i + 1; j < valid; j++)
        {
            if (samples[i] > samples[j])
            {
                int32_t tmp = samples[i];
                samples[i] = samples[j];
                samples[j] = tmp;
            }
        }
    }

    return samples[valid / 2]; // Retorna mediana
}

// ============================================================================
// FUNÇÕES ESP-NOW
// ============================================================================

/**
 * @brief Callback de envio ESP-NOW (ESP-IDF 6.x signature)
 */
static void espnow_send_cb(const esp_now_send_info_t *send_info, esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS)
    {
        packets_sent++;
        gpio_set_level(GPIO_LED, 1); // LED ON = sucesso
    }
    else
    {
        packets_failed++;
        // Pisca rápido em erro
        for (int i = 0; i < 3; i++)
        {
            gpio_set_level(GPIO_LED, 1);
            vTaskDelay(pdMS_TO_TICKS(50));
            gpio_set_level(GPIO_LED, 0);
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

/**
 * @brief Envia telemetria via ESP-NOW
 */
static esp_err_t send_telemetry(const char *type, int32_t value)
{
    char payload[200];
    int64_t uptime_s = esp_timer_get_time() / 1000000;

    snprintf(payload, sizeof(payload),
             "{\"mac\":\"%s\",\"type\":\"%s\",\"value\":%ld,\"uptime\":%lld}",
             node_mac_str, type, (long)value, (long long)uptime_s);

    ESP_LOGI(TAG, "TX: %s", payload);

    esp_err_t ret = esp_now_send(gateway_mac, (uint8_t *)payload, strlen(payload));

    if (ret == ESP_OK)
    {
        last_send_time = esp_timer_get_time();
    }

    return ret;
}

/**
 * @brief Inicializa ESP-NOW
 */
static esp_err_t init_espnow(void)
{
    // Inicializa WiFi em modo NULL (apenas para ESP-NOW)
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Configura canal
    ESP_ERROR_CHECK(esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));

    // Inicializa ESP-NOW
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));

    // Adiciona peer broadcast
    esp_now_peer_info_t peer = {
        .channel = ESPNOW_CHANNEL,
        .ifidx = WIFI_IF_STA,
        .encrypt = false,
    };
    memcpy(peer.peer_addr, gateway_mac, 6);

    esp_err_t ret = esp_now_add_peer(&peer);
    if (ret != ESP_OK && ret != ESP_ERR_ESPNOW_EXIST)
    {
        ESP_LOGE(TAG, "Erro ao adicionar peer: %d", ret);
        return ret;
    }

    ESP_LOGI(TAG, "ESP-NOW iniciado (canal %d)", ESPNOW_CHANNEL);
    return ESP_OK;
}

// ============================================================================
// FUNÇÕES DE INICIALIZAÇÃO
// ============================================================================

/**
 * @brief Inicializa GPIOs
 */
static void init_gpio(void)
{
    // LED como saída
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << GPIO_LED),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);
    gpio_set_level(GPIO_LED, 0);

    // Trigger como saída
    gpio_config_t trig_conf = {
        .pin_bit_mask = (1ULL << GPIO_TRIG),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&trig_conf);
    gpio_set_level(GPIO_TRIG, 0);

    // Echo como entrada
    gpio_config_t echo_conf = {
        .pin_bit_mask = (1ULL << GPIO_ECHO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&echo_conf);

    ESP_LOGI(TAG, "GPIO: TRIG=%d, ECHO=%d, LED=%d", GPIO_TRIG, GPIO_ECHO, GPIO_LED);
}

/**
 * @brief Obtém MAC address do node
 */
static void get_node_mac(void)
{
    esp_read_mac(node_mac, ESP_MAC_WIFI_STA);
    snprintf(node_mac_str, sizeof(node_mac_str),
             "%02X:%02X:%02X:%02X:%02X:%02X",
             node_mac[0], node_mac[1], node_mac[2],
             node_mac[3], node_mac[4], node_mac[5]);
    ESP_LOGI(TAG, "Node MAC: %s", node_mac_str);
}

// ============================================================================
// TASK PRINCIPAL
// ============================================================================

/**
 * @brief Task de leitura e envio de telemetria
 */
static void sensor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Sensor task iniciada");

    // Primeira leitura
    vTaskDelay(pdMS_TO_TICKS(1000));

    while (1)
    {
        // LED OFF durante leitura
        gpio_set_level(GPIO_LED, 0);

        // Lê distância
        int32_t distance = read_ultrasonic_filtered();

        if (distance > 0)
        {
            int64_t now = esp_timer_get_time();
            int64_t elapsed_ms = (now - last_send_time) / 1000;

            // Calcula delta
            int32_t delta = 0;
            if (last_distance_cm > 0)
            {
                delta = abs(distance - last_distance_cm);
            }

            // Decide se envia
            bool should_send = false;
            const char *reason = "";

            // Primeira leitura
            if (last_distance_cm < 0)
            {
                should_send = true;
                reason = "FIRST";
            }
            // Variação significativa (> DEADBAND_CM)
            else if (delta >= DEADBAND_CM * VALUE_MULTIPLIER)
            {
                should_send = true;
                reason = "DELTA";
            }
            // Heartbeat (timeout 5 min)
            else if (elapsed_ms >= HEARTBEAT_MS)
            {
                should_send = true;
                reason = "HEARTBEAT";
            }

            if (should_send)
            {
                ESP_LOGI(TAG, "Distância: %ld.%02ld cm [%s] (delta=%ld.%02ld)",
                         (long)(distance / 100), (long)(distance % 100),
                         reason,
                         (long)(delta / 100), (long)(delta % 100));

                send_telemetry("distance_cm", distance);
                last_distance_cm = distance;
            }
            else
            {
                ESP_LOGD(TAG, "Distância: %ld.%02ld cm (sem envio)",
                         (long)(distance / 100), (long)(distance % 100));
            }
        }
        else
        {
            ESP_LOGW(TAG, "Erro na leitura do ultrassônico");
        }

        // Aguarda próxima leitura
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
    }
}

// ============================================================================
// MAIN
// ============================================================================

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  AGUADA Node Minimal v1.0");
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
    get_node_mac();

    // LED pisca 3x no boot
    for (int i = 0; i < 3; i++)
    {
        gpio_set_level(GPIO_LED, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(GPIO_LED, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Inicializa ESP-NOW
    ESP_ERROR_CHECK(init_espnow());

    // Cria task de sensor
    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Sistema iniciado!");
}
