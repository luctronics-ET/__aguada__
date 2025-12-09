/**
 * AGUADA v1.2 - Node Sensor 21 (Dual Ultrasonic)
 * 
 * Firmware para 2 sensores ultrassônicos (IE01 + IE02)
 * Cada sensor envia pacotes como se fosse um node separado:
 * - IE01: Usa MAC real do ESP32-C3
 * - IE02: Usa MAC virtual (AA:BB:CC:DD:1E:02)
 * 
 * Hardware: ESP32-C3 SuperMini + 2x AJ-SR04M
 * Protocolo: ESP-NOW → Gateway → HTTP/MQTT → Backend
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_now.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "config.h"

static const char *TAG = "AGUADA_NODE21";

// ============================================================================
// TIPOS E ESTRUTURAS
// ============================================================================

typedef struct {
    int32_t distance_mm;
    int32_t vcc_bat_mv;
    int32_t rssi;
    int64_t timestamp;
} telemetry_data_t;

typedef struct {
    telemetry_data_t last_sent;
    telemetry_data_t current;
    int64_t last_send_time;
    bool first_reading;
    float ema_distance_mm;
    bool ema_initialized;
    uint8_t rle_stable_count;
    int32_t rle_stable_value;
} sensor_state_t;

typedef struct {
    uint32_t packets_sent;
    uint32_t packets_failed;
    uint32_t readings_total;
    uint32_t readings_valid;
    uint32_t deltas_detected;
    uint32_t heartbeats_sent;
} metrics_t;

// ============================================================================
// VARIÁVEIS GLOBAIS
// ============================================================================

// MAC addresses
static uint8_t node_mac_ie01[6];        // MAC real do ESP32-C3 (IE01)
static uint8_t node_mac_ie02[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0x1E, 0x02};  // MAC virtual (IE02)
static char mac_str_ie01[18];
static char mac_str_ie02[18];

// Estados dos sensores (separados)
static sensor_state_t sensor_ie01 = {0};
static sensor_state_t sensor_ie02 = {0};

// Métricas (compartilhadas)
static metrics_t metrics = {0};

// ADC handles
static adc_oneshot_unit_handle_t adc_handle = NULL;
static adc_cali_handle_t adc_cali_handle = NULL;
static bool adc_calibrated = false;

// ============================================================================
// FUNÇÕES AUXILIARES
// ============================================================================

static void mac_to_string(const uint8_t *mac, char *str) {
    snprintf(str, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static int get_rssi(void) {
    if (metrics.packets_sent + metrics.packets_failed > 0) {
        float success_rate = (float)metrics.packets_sent / 
                            (float)(metrics.packets_sent + metrics.packets_failed);
        return (int)(-90 + success_rate * 60);
    }
    return -50;
}

static int get_vcc_mv(void) {
    if (adc_handle == NULL) {
        return VCC_USB_MV;
    }
    
    int adc_raw = 0;
    int voltage_mv = 0;
    int sum_mv = 0;
    
    for (int i = 0; i < VCC_ADC_SAMPLES; i++) {
        if (adc_oneshot_read(adc_handle, ADC_CHANNEL, &adc_raw) == ESP_OK) {
            if (adc_calibrated && adc_cali_handle != NULL) {
                adc_cali_raw_to_voltage(adc_cali_handle, adc_raw, &voltage_mv);
            } else {
                voltage_mv = (adc_raw * 2500) / 4095;
            }
            sum_mv += voltage_mv;
        }
    }
    
    int avg_mv = sum_mv / VCC_ADC_SAMPLES;
    int vcc_mv = (int)(avg_mv * VCC_DIVIDER_RATIO);
    
    if (vcc_mv < VCC_MIN_MV || vcc_mv > VCC_MAX_MV) {
        ESP_LOGW(TAG, "VCC fora do range: %d mV", vcc_mv);
    }
    
    return vcc_mv;
}

static int compare_int(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

// ============================================================================
// INICIALIZAÇÃO GPIO
// ============================================================================

static void init_gpio(void) {
    // Sensor IE01
    gpio_reset_pin(PIN_TRIG_IE01);
    gpio_set_direction(PIN_TRIG_IE01, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_TRIG_IE01, 0);
    
    gpio_reset_pin(PIN_ECHO_IE01);
    gpio_set_direction(PIN_ECHO_IE01, GPIO_MODE_INPUT);
    
    // Sensor IE02
    gpio_reset_pin(PIN_TRIG_IE02);
    gpio_set_direction(PIN_TRIG_IE02, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_TRIG_IE02, 0);
    
    gpio_reset_pin(PIN_ECHO_IE02);
    gpio_set_direction(PIN_ECHO_IE02, GPIO_MODE_INPUT);
    
    // LED Status
    gpio_reset_pin(PIN_LED_STATUS);
    gpio_set_direction(PIN_LED_STATUS, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_LED_STATUS, 0);
    
    ESP_LOGI(TAG, "✓ GPIO IE01: TRIG=%d, ECHO=%d", PIN_TRIG_IE01, PIN_ECHO_IE01);
    ESP_LOGI(TAG, "✓ GPIO IE02: TRIG=%d, ECHO=%d", PIN_TRIG_IE02, PIN_ECHO_IE02);
    ESP_LOGI(TAG, "✓ LED=%d", PIN_LED_STATUS);
}

// ============================================================================
// INICIALIZAÇÃO ADC
// ============================================================================

static void init_adc(void) {
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    
    esp_err_t ret = adc_oneshot_new_unit(&unit_cfg, &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao inicializar ADC: %s", esp_err_to_name(ret));
        return;
    }
    
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_12,
    };
    
    ret = adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao configurar canal ADC: %s", esp_err_to_name(ret));
        return;
    }
    
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT,
        .chan = ADC_CHANNEL,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_12,
    };
    
    ret = adc_cali_create_scheme_curve_fitting(&cali_cfg, &adc_cali_handle);
    if (ret == ESP_OK) {
        adc_calibrated = true;
        ESP_LOGI(TAG, "✓ ADC calibrado");
    }
#endif
    
    ESP_LOGI(TAG, "✓ ADC: GPIO%d, divisor %.1f:1", PIN_VCC_ADC, VCC_DIVIDER_RATIO);
}

// ============================================================================
// SENSOR ULTRASSÔNICO (Genérico para qualquer pino)
// ============================================================================

static int read_ultrasonic_single(gpio_num_t trig_pin, gpio_num_t echo_pin) {
    gpio_set_level(trig_pin, 0);
    esp_rom_delay_us(2);
    gpio_set_level(trig_pin, 1);
    esp_rom_delay_us(10);
    gpio_set_level(trig_pin, 0);
    
    int64_t timeout_start = esp_timer_get_time();
    while (gpio_get_level(echo_pin) == 0) {
        if ((esp_timer_get_time() - timeout_start) > SENSOR_TIMEOUT_US) {
            return -1;
        }
    }
    
    int64_t pulse_start = esp_timer_get_time();
    while (gpio_get_level(echo_pin) == 1) {
        if ((esp_timer_get_time() - pulse_start) > SENSOR_TIMEOUT_US) {
            return -1;
        }
    }
    int64_t duration_us = esp_timer_get_time() - pulse_start;
    
    int32_t distance_mm = (int32_t)((duration_us * 343) / 2000);
    
    if (distance_mm < SENSOR_MIN_MM || distance_mm > SENSOR_MAX_MM) {
        return -2;
    }
    
    return distance_mm;
}

static int apply_ema_filter(sensor_state_t *state, int new_value) {
#if USE_EMA_FILTER
    if (!state->ema_initialized) {
        state->ema_distance_mm = (float)new_value;
        state->ema_initialized = true;
        return new_value;
    }
    
    state->ema_distance_mm = EMA_ALPHA * (float)new_value + (1.0f - EMA_ALPHA) * state->ema_distance_mm;
    return (int)(state->ema_distance_mm + 0.5f);
#else
    return new_value;
#endif
}

static int read_ultrasonic_filtered(gpio_num_t trig_pin, gpio_num_t echo_pin, sensor_state_t *state) {
    int samples[SAMPLES_PER_READ];
    int valid_count = 0;
    
    metrics.readings_total++;
    
    for (int i = 0; i < SAMPLES_PER_READ; i++) {
        int dist = read_ultrasonic_single(trig_pin, echo_pin);
        if (dist > 0) {
            samples[valid_count++] = dist;
        }
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
    }
    
    if (valid_count < (SAMPLES_PER_READ / 2)) {
        ESP_LOGW(TAG, "Poucas amostras válidas: %d/%d", valid_count, SAMPLES_PER_READ);
        return -1;
    }
    
    qsort(samples, valid_count, sizeof(int), compare_int);
    int median = samples[valid_count / 2];
    int filtered = apply_ema_filter(state, median);
    
    metrics.readings_valid++;
    
    return filtered;
}

// ============================================================================
// ESP-NOW
// ============================================================================

static void espnow_send_cb(const esp_now_send_info_t *info, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        metrics.packets_sent++;
        gpio_set_level(PIN_LED_STATUS, 1);
        vTaskDelay(pdMS_TO_TICKS(30));
        gpio_set_level(PIN_LED_STATUS, 0);
    } else {
        metrics.packets_failed++;
    }
}

static void init_espnow(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));
    
    // Obter MAC real (para IE01)
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, node_mac_ie01));
    mac_to_string(node_mac_ie01, mac_str_ie01);
    mac_to_string(node_mac_ie02, mac_str_ie02);
    
    ESP_LOGI(TAG, "✓ Canal ESP-NOW: %d", ESPNOW_CHANNEL);
    ESP_LOGI(TAG, "✓ IE01 MAC: %s (real)", mac_str_ie01);
    ESP_LOGI(TAG, "✓ IE02 MAC: %s (virtual)", mac_str_ie02);
    
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));
    
    esp_now_peer_info_t peer_info = {0};
    memcpy(peer_info.peer_addr, GATEWAY_MAC, 6);
    peer_info.channel = ESPNOW_CHANNEL;
    peer_info.encrypt = false;
    ESP_ERROR_CHECK(esp_now_add_peer(&peer_info));
    
    ESP_LOGI(TAG, "✓ Gateway: %02X:%02X:%02X:%02X:%02X:%02X",
             GATEWAY_MAC[0], GATEWAY_MAC[1], GATEWAY_MAC[2],
             GATEWAY_MAC[3], GATEWAY_MAC[4], GATEWAY_MAC[5]);
}

// ============================================================================
// TELEMETRIA
// ============================================================================

static bool rle_update(sensor_state_t *state, int32_t distance_mm) {
#if USE_RLE
    if (state->rle_stable_count == 0) {
        state->rle_stable_value = distance_mm;
        state->rle_stable_count = 1;
        return true;
    }
    
    int32_t delta = abs(distance_mm - state->rle_stable_value);
    
    if (delta < DELTA_DISTANCE_MM) {
        if (state->rle_stable_count < RLE_MAX_COUNT) {
            state->rle_stable_count++;
        }
        return true;
    } else {
        state->rle_stable_value = distance_mm;
        state->rle_stable_count = 1;
        return false;
    }
#else
    return true;
#endif
}

static bool send_telemetry(const char *mac_str, const uint8_t *mac, 
                          const telemetry_data_t *data, 
                          sensor_state_t *state,
                          const char *sensor_name) {
    char payload[MAX_PAYLOAD_SIZE];
    
    int len = snprintf(payload, sizeof(payload),
             "{\"mac\":\"%s\",\"distance_mm\":%ld,\"vcc_bat_mv\":%ld,\"rssi\":%ld,\"rle\":%d}",
             mac_str, 
             (long)data->distance_mm, 
             (long)data->vcc_bat_mv, 
             (long)data->rssi,
             state->rle_stable_count);
    
    ESP_LOGI(TAG, "[%s] → %s", sensor_name, payload);
    
    for (int retry = 0; retry < ESPNOW_MAX_RETRIES; retry++) {
        esp_err_t result = esp_now_send(GATEWAY_MAC, (uint8_t*)payload, strlen(payload));
        
        if (result == ESP_OK) {
            state->last_send_time = esp_timer_get_time();
            return true;
        }
        
        ESP_LOGW(TAG, "[%s] Retry %d/%d", sensor_name, retry + 1, ESPNOW_MAX_RETRIES);
        vTaskDelay(pdMS_TO_TICKS(ESPNOW_RETRY_MS));
    }
    
    ESP_LOGE(TAG, "[%s] Falha ao enviar", sensor_name);
    return false;
}

static bool should_send(const telemetry_data_t *current, sensor_state_t *state, bool *is_heartbeat) {
    *is_heartbeat = false;
    
    if (state->first_reading) {
        state->first_reading = false;
        return true;
    }
    
    int64_t now = esp_timer_get_time();
    int64_t elapsed_ms = (now - state->last_send_time) / 1000;
    
    if (elapsed_ms >= HEARTBEAT_MS) {
        metrics.heartbeats_sent++;
        *is_heartbeat = true;
        return true;
    }
    
    int32_t delta_mm = abs(current->distance_mm - state->last_sent.distance_mm);
    if (delta_mm >= DELTA_DISTANCE_MM) {
        metrics.deltas_detected++;
        return true;
    }
    
    int32_t delta_vcc = abs(current->vcc_bat_mv - state->last_sent.vcc_bat_mv);
    if (delta_vcc >= DELTA_VCC_MV) {
        metrics.deltas_detected++;
        return true;
    }
    
    return false;
}

// ============================================================================
// TASK PRINCIPAL
// ============================================================================

static void telemetry_task(void *pvParameters) {
    ESP_LOGI(TAG, "Iniciando telemetria dual (intervalo: %d ms)", READ_INTERVAL_MS);
    
    sensor_ie01.first_reading = true;
    sensor_ie01.last_send_time = esp_timer_get_time();
    
    sensor_ie02.first_reading = true;
    sensor_ie02.last_send_time = esp_timer_get_time();
    
    while (1) {
        int vcc_mv = get_vcc_mv();
        int rssi = get_rssi();
        
        // ====== SENSOR IE01 ======
        {
            telemetry_data_t current = {
                .distance_mm = read_ultrasonic_filtered(PIN_TRIG_IE01, PIN_ECHO_IE01, &sensor_ie01),
                .vcc_bat_mv = vcc_mv,
                .rssi = rssi,
                .timestamp = esp_timer_get_time()
            };
            
            if (current.distance_mm < 0) {
                current.distance_mm = (current.distance_mm == -1) ? 0 : 1;
            }
            
            rle_update(&sensor_ie01, current.distance_mm);
            
            bool is_heartbeat = false;
            if (should_send(&current, &sensor_ie01, &is_heartbeat)) {
                if (send_telemetry(mac_str_ie01, node_mac_ie01, &current, &sensor_ie01, "IE01")) {
                    sensor_ie01.last_sent = current;
                    if (!is_heartbeat) {
                        sensor_ie01.rle_stable_count = 1;
                    }
                }
            }
        }
        
        // Pequeno delay entre sensores para evitar interferência
        vTaskDelay(pdMS_TO_TICKS(100));
        
        // ====== SENSOR IE02 ======
        {
            telemetry_data_t current = {
                .distance_mm = read_ultrasonic_filtered(PIN_TRIG_IE02, PIN_ECHO_IE02, &sensor_ie02),
                .vcc_bat_mv = vcc_mv,
                .rssi = rssi,
                .timestamp = esp_timer_get_time()
            };
            
            if (current.distance_mm < 0) {
                current.distance_mm = (current.distance_mm == -1) ? 0 : 1;
            }
            
            rle_update(&sensor_ie02, current.distance_mm);
            
            bool is_heartbeat = false;
            if (should_send(&current, &sensor_ie02, &is_heartbeat)) {
                if (send_telemetry(mac_str_ie02, node_mac_ie02, &current, &sensor_ie02, "IE02")) {
                    sensor_ie02.last_sent = current;
                    if (!is_heartbeat) {
                        sensor_ie02.rle_stable_count = 1;
                    }
                }
            }
        }
        
        // Log de estatísticas
        if (metrics.packets_sent > 0 && metrics.packets_sent % STATS_INTERVAL == 0) {
            ESP_LOGI(TAG, "📊 Stats: TX=%lu OK=%lu FAIL=%lu Delta=%lu HB=%lu",
                     metrics.readings_total, metrics.packets_sent, 
                     metrics.packets_failed, metrics.deltas_detected,
                     metrics.heartbeats_sent);
        }
        
        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }
}

static void heartbeat_led_task(void *pvParameters) {
    while (1) {
        // 3 piscadas rápidas (dual = 2 grupos)
        for (int g = 0; g < 2; g++) {
            for (int i = 0; i < 3; i++) {
                gpio_set_level(PIN_LED_STATUS, 1);
                vTaskDelay(pdMS_TO_TICKS(80));
                gpio_set_level(PIN_LED_STATUS, 0);
                vTaskDelay(pdMS_TO_TICKS(80));
            }
            vTaskDelay(pdMS_TO_TICKS(300));
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// ============================================================================
// MAIN
// ============================================================================

void app_main(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║       AGUADA - Dual Sensor Node (IE01 + IE02)        ║");
    ESP_LOGI(TAG, "╠══════════════════════════════════════════════════════╣");
    ESP_LOGI(TAG, "║  Firmware:  %-40s ║", FIRMWARE_VERSION);
    ESP_LOGI(TAG, "║  Protocolo: %-40s ║", PROTOCOL_VERSION);
    ESP_LOGI(TAG, "║  Hardware:  ESP32-C3 + 2x AJ-SR04M                   ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    
    init_gpio();
    init_adc();
    
    // Animação de boot (6 piscadas = 2 sensores × 3)
    for (int i = 0; i < 6; i++) {
        gpio_set_level(PIN_LED_STATUS, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(PIN_LED_STATUS, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    init_espnow();
    
    xTaskCreate(telemetry_task, "telemetry", 4096, NULL, 5, NULL);
    xTaskCreate(heartbeat_led_task, "heartbeat", 2048, NULL, 3, NULL);
    
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "✓ Sistema pronto!");
    ESP_LOGI(TAG, "  - IE01: GPIO TRIG=%d ECHO=%d (MAC real)", PIN_TRIG_IE01, PIN_ECHO_IE01);
    ESP_LOGI(TAG, "  - IE02: GPIO TRIG=%d ECHO=%d (MAC virtual)", PIN_TRIG_IE02, PIN_ECHO_IE02);
    ESP_LOGI(TAG, "  - Leitura a cada %d ms", READ_INTERVAL_MS);
    ESP_LOGI(TAG, "  - Heartbeat a cada %d ms", HEARTBEAT_MS);
    ESP_LOGI(TAG, "");
}
