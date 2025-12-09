/**
 * AGUADA v1.1 - Firmware Universal para Sensor Nodes
 * 
 * Protocolo AGUADA-1:
 * - Pacote JSON padronizado com distance_mm, vcc_bat_mv, rssi
 * - Envio apenas de deltas (mudanças significativas)
 * - Heartbeat a cada 30 segundos
 * - Mediana de 11 amostras para filtragem
 * 
 * Hardware: ESP32-C3 SuperMini + AJ-SR04M
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

static const char *TAG = "AGUADA_NODE";

// ============================================================================
// TIPOS E ESTRUTURAS
// ============================================================================

/**
 * Pacote de telemetria AGUADA-1
 * Formato JSON: {"mac":"XX:XX:XX:XX:XX:XX","distance_mm":2450,"vcc_bat_mv":4900,"rssi":-50}
 */
typedef struct {
    int32_t distance_mm;    // Distância em mm (negativo = erro)
    int32_t vcc_bat_mv;     // Tensão em mV
    int32_t rssi;           // RSSI em dBm
    int64_t timestamp;      // Timestamp em microsegundos
} telemetry_data_t;

/**
 * Estado do sensor
 */
typedef struct {
    telemetry_data_t last_sent;     // Último valor enviado
    telemetry_data_t current;       // Valor atual
    int64_t last_send_time;         // Timestamp do último envio
    bool first_reading;             // Primeira leitura após boot
} sensor_state_t;

/**
 * Métricas de transmissão
 */
typedef struct {
    uint32_t packets_sent;          // Pacotes enviados com sucesso
    uint32_t packets_failed;        // Pacotes com falha
    uint32_t readings_total;        // Total de leituras do sensor
    uint32_t readings_valid;        // Leituras válidas
    uint32_t deltas_detected;       // Mudanças detectadas
    uint32_t heartbeats_sent;       // Heartbeats enviados
} metrics_t;

/**
 * RLE (Run-Length Encoding) - conta leituras estáveis consecutivas
 */
typedef struct {
    uint8_t stable_count;           // Contador de leituras estáveis (0-255)
    int32_t stable_value;           // Valor considerado estável
} rle_state_t;

/**
 * Agregação temporal - estatísticas em janela de tempo
 */
typedef struct {
    int32_t min_mm;                 // Mínimo na janela
    int32_t max_mm;                 // Máximo na janela
    int64_t sum_mm;                 // Soma para calcular média
    uint16_t count;                 // Número de amostras na janela
    bool valid;                     // Se há dados válidos
} aggregation_t;

/**
 * Payload binário compacto (16 bytes)
 * Estrutura: [MAGIC:2][MAC:6][DIST:2][VCC:2][RSSI:1][FLAGS:1][CRC:2]
 */
typedef struct __attribute__((packed)) {
    uint16_t magic;                 // 0xAD01 = AGUADA-1
    uint8_t mac[6];                 // MAC address
    int16_t distance_mm;            // Distância (±32767mm)
    uint16_t vcc_mv;                // Tensão (0-65535mV)
    int8_t rssi;                    // RSSI (-128 a +127)
    uint8_t flags;                  // Flags de status
    uint16_t crc16;                 // CRC-16 para validação
} binary_payload_t;

// ============================================================================
// VARIÁVEIS GLOBAIS
// ============================================================================

static uint8_t node_mac[6];
static char node_mac_str[18];
static sensor_state_t sensor_state = {0};
static metrics_t metrics = {0};

// ADC handles
static adc_oneshot_unit_handle_t adc_handle = NULL;
static adc_cali_handle_t adc_cali_handle = NULL;
static bool adc_calibrated = false;

// EMA state
static float ema_distance_mm = 0;
static bool ema_initialized = false;

// RLE state
#if USE_RLE
static rle_state_t rle_state = {0};
#endif

// Aggregation state
#if USE_AGGREGATION
static aggregation_t agg_state = {0};
#endif

// ============================================================================
// FUNÇÕES AUXILIARES
// ============================================================================

/**
 * Calcular CRC-16 (CCITT)
 */
static uint16_t crc16_ccitt(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/**
 * Formatar MAC address como string
 */
static void mac_to_string(const uint8_t *mac, char *str) {
    snprintf(str, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

/**
 * Obter RSSI
 * 
 * Nota: Sem WiFi conectado, não é possível ler RSSI diretamente.
 * O RSSI real é medido pelo gateway no momento da recepção.
 * Aqui retornamos um placeholder que pode ser substituído
 * por uma métrica de qualidade de link baseada em ACKs.
 */
static int get_rssi(void) {
    // Calcular "pseudo-RSSI" baseado na taxa de sucesso
    if (metrics.packets_sent + metrics.packets_failed > 0) {
        float success_rate = (float)metrics.packets_sent / 
                            (float)(metrics.packets_sent + metrics.packets_failed);
        // Mapear: 100% sucesso = -30dBm, 0% = -90dBm
        return (int)(-90 + success_rate * 60);
    }
    return -50;  // Valor inicial padrão
}

/**
 * Obter tensão de alimentação via ADC (mV)
 * 
 * Usa ADC1_CH4 (GPIO4) com divisor resistivo 2:1
 * 5V → 2.5V no pino → lido pelo ADC → multiplicado por 2
 */
static int get_vcc_mv(void) {
    if (adc_handle == NULL) {
        return VCC_USB_MV;  // Fallback se ADC não inicializado
    }
    
    int adc_raw = 0;
    int voltage_mv = 0;
    int sum_mv = 0;
    
    // Média de várias leituras
    for (int i = 0; i < VCC_ADC_SAMPLES; i++) {
        if (adc_oneshot_read(adc_handle, ADC_CHANNEL, &adc_raw) == ESP_OK) {
            if (adc_calibrated && adc_cali_handle != NULL) {
                adc_cali_raw_to_voltage(adc_cali_handle, adc_raw, &voltage_mv);
            } else {
                // Conversão aproximada sem calibração (12-bit ADC, 2.5V ref)
                voltage_mv = (adc_raw * 2500) / 4095;
            }
            sum_mv += voltage_mv;
        }
    }
    
    // Média e aplicação do divisor
    int avg_mv = sum_mv / VCC_ADC_SAMPLES;
    int vcc_mv = (int)(avg_mv * VCC_DIVIDER_RATIO);
    
    // Validar range
    if (vcc_mv < VCC_MIN_MV || vcc_mv > VCC_MAX_MV) {
        ESP_LOGW(TAG, "VCC fora do range: %d mV", vcc_mv);
    }
    
    return vcc_mv;
}

/**
 * Comparar ints para qsort
 */
static int compare_int(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

// ============================================================================
// INICIALIZAÇÃO GPIO
// ============================================================================

static void init_gpio(void) {
    // Sensor ultrassônico
    gpio_reset_pin(PIN_TRIG);
    gpio_set_direction(PIN_TRIG, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_TRIG, 0);
    
    gpio_reset_pin(PIN_ECHO);
    gpio_set_direction(PIN_ECHO, GPIO_MODE_INPUT);
    
    // LED Status
    gpio_reset_pin(PIN_LED_STATUS);
    gpio_set_direction(PIN_LED_STATUS, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_LED_STATUS, 0);
    
    ESP_LOGI(TAG, "✓ GPIO: TRIG=%d, ECHO=%d, LED=%d", 
             PIN_TRIG, PIN_ECHO, PIN_LED_STATUS);
}

// ============================================================================
// INICIALIZAÇÃO ADC
// ============================================================================

/**
 * Inicializar ADC para leitura de VCC
 */
static void init_adc(void) {
    // Configurar ADC unit
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    
    esp_err_t ret = adc_oneshot_new_unit(&unit_cfg, &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao inicializar ADC unit: %s", esp_err_to_name(ret));
        return;
    }
    
    // Configurar canal
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_12,
    };
    
    ret = adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao configurar canal ADC: %s", esp_err_to_name(ret));
        return;
    }
    
    // Tentar calibração (curve fitting para ESP32-C3)
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
        ESP_LOGI(TAG, "✓ ADC calibrado (curve fitting)");
    } else {
        ESP_LOGW(TAG, "ADC sem calibração: %s", esp_err_to_name(ret));
    }
#else
    ESP_LOGW(TAG, "Calibração ADC não suportada neste chip");
#endif
    
    ESP_LOGI(TAG, "✓ ADC: GPIO%d, CH%d, divisor %.1f:1", 
             PIN_VCC_ADC, ADC_CHANNEL, VCC_DIVIDER_RATIO);
}

// ============================================================================
// FILTRAGEM EMA
// ============================================================================

/**
 * Aplicar filtro EMA (Exponential Moving Average)
 * 
 * Suaviza leituras, reduz ruído de alta frequência
 */
static int apply_ema_filter(int new_value) {
#if USE_EMA_FILTER
    if (!ema_initialized) {
        ema_distance_mm = (float)new_value;
        ema_initialized = true;
        return new_value;
    }
    
    // EMA = alpha * novo + (1 - alpha) * anterior
    ema_distance_mm = EMA_ALPHA * (float)new_value + (1.0f - EMA_ALPHA) * ema_distance_mm;
    
    return (int)(ema_distance_mm + 0.5f);  // Arredondar
#else
    return new_value;
#endif
}

// ============================================================================
// SENSOR ULTRASSÔNICO
// ============================================================================

/**
 * Leitura única do sensor ultrassônico
 * 
 * @return Distância em mm, ou:
 *         -1: Timeout (sensor não respondeu)
 *         -2: Fora de range
 */
static int read_ultrasonic_single(void) {
    // Pulso TRIG (10μs)
    gpio_set_level(PIN_TRIG, 0);
    esp_rom_delay_us(2);
    gpio_set_level(PIN_TRIG, 1);
    esp_rom_delay_us(10);
    gpio_set_level(PIN_TRIG, 0);
    
    // Aguardar ECHO subir
    int64_t timeout_start = esp_timer_get_time();
    while (gpio_get_level(PIN_ECHO) == 0) {
        if ((esp_timer_get_time() - timeout_start) > SENSOR_TIMEOUT_US) {
            return -1;  // Timeout
        }
    }
    
    // Medir duração do pulso ECHO
    int64_t pulse_start = esp_timer_get_time();
    while (gpio_get_level(PIN_ECHO) == 1) {
        if ((esp_timer_get_time() - pulse_start) > SENSOR_TIMEOUT_US) {
            return -1;  // Timeout
        }
    }
    int64_t duration_us = esp_timer_get_time() - pulse_start;
    
    // Calcular distância em mm
    // Velocidade do som: 343 m/s = 0.343 mm/μs
    // Distância = (duração × 0.343) / 2 = duração × 0.1715
    // Para evitar float: distance_mm = (duration_us * 343) / 2000
    int32_t distance_mm = (int32_t)((duration_us * 343) / 2000);
    
    // Validar range
    if (distance_mm < SENSOR_MIN_MM || distance_mm > SENSOR_MAX_MM) {
        return -2;  // Fora de range
    }
    
    return distance_mm;
}

/**
 * Leitura filtrada com mediana
 * 
 * Coleta SAMPLES_PER_READ amostras e retorna a mediana
 */
static int read_ultrasonic_filtered(void) {
    int samples[SAMPLES_PER_READ];
    int valid_count = 0;
    
    metrics.readings_total++;
    
    // Coletar amostras
    for (int i = 0; i < SAMPLES_PER_READ; i++) {
        int dist = read_ultrasonic_single();
        if (dist > 0) {
            samples[valid_count++] = dist;
        }
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
    }
    
    // Verificar se temos amostras suficientes
    if (valid_count < (SAMPLES_PER_READ / 2)) {
        ESP_LOGW(TAG, "Poucas amostras válidas: %d/%d", valid_count, SAMPLES_PER_READ);
        return -1;
    }
    
    // Ordenar para obter mediana
    qsort(samples, valid_count, sizeof(int), compare_int);
    
    int median = samples[valid_count / 2];
    
    // Aplicar filtro EMA para suavização adicional
    int filtered = apply_ema_filter(median);
    
    metrics.readings_valid++;
    
    ESP_LOGD(TAG, "Distância: median=%d ema=%d mm (%d amostras)", median, filtered, valid_count);
    
    return filtered;
}

// ============================================================================
// ESP-NOW
// ============================================================================

/**
 * Callback de envio ESP-NOW
 */
static void espnow_send_cb(const esp_now_send_info_t *info, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        metrics.packets_sent++;
        gpio_set_level(PIN_LED_STATUS, 1);
        vTaskDelay(pdMS_TO_TICKS(50));
        gpio_set_level(PIN_LED_STATUS, 0);
    } else {
        metrics.packets_failed++;
    }
}

/**
 * Inicializar ESP-NOW (sem WiFi STA conectado)
 * 
 * ESP-NOW funciona com WiFi em modo STA, mas sem conectar a nenhum AP.
 * Isso economiza energia e simplifica o código.
 * O canal deve ser o mesmo do gateway.
 */
static void init_espnow(void) {
    // NVS Flash (necessário para WiFi/ESP-NOW)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Inicializar WiFi mínimo para ESP-NOW (sem netif, sem event loop completo)
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    // Desabilitar power save para melhor responsividade ESP-NOW
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    
    // Iniciar WiFi (necessário para ESP-NOW, mas não conecta a nenhum AP)
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Configurar canal ANTES de obter MAC (importante!)
    ESP_ERROR_CHECK(esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));
    ESP_LOGI(TAG, "✓ Canal ESP-NOW: %d", ESPNOW_CHANNEL);
    
    // Obter MAC address do dispositivo
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, node_mac));
    mac_to_string(node_mac, node_mac_str);
    ESP_LOGI(TAG, "✓ Node MAC: %s", node_mac_str);
    
    // Inicializar ESP-NOW
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));
    
    // Adicionar gateway como peer
    esp_now_peer_info_t peer_info = {0};
    memcpy(peer_info.peer_addr, GATEWAY_MAC, 6);
    peer_info.channel = ESPNOW_CHANNEL;
    peer_info.encrypt = false;
    ESP_ERROR_CHECK(esp_now_add_peer(&peer_info));
    
    ESP_LOGI(TAG, "✓ Gateway: %02X:%02X:%02X:%02X:%02X:%02X (canal %d)",
             GATEWAY_MAC[0], GATEWAY_MAC[1], GATEWAY_MAC[2],
             GATEWAY_MAC[3], GATEWAY_MAC[4], GATEWAY_MAC[5], ESPNOW_CHANNEL);
}

// ============================================================================
// TELEMETRIA
// ============================================================================

#if USE_RLE
/**
 * Atualizar estado RLE
 * Retorna true se valor é estável (dentro do deadband)
 */
static bool rle_update(int32_t distance_mm) {
    if (rle_state.stable_count == 0) {
        // Primeira leitura
        rle_state.stable_value = distance_mm;
        rle_state.stable_count = 1;
        return true;
    }
    
    int32_t delta = abs(distance_mm - rle_state.stable_value);
    
    if (delta < DELTA_DISTANCE_MM) {
        // Valor estável
        if (rle_state.stable_count < RLE_MAX_COUNT) {
            rle_state.stable_count++;
        }
        return true;
    } else {
        // Mudança detectada - reset RLE
        rle_state.stable_value = distance_mm;
        rle_state.stable_count = 1;
        return false;
    }
}
#endif

#if USE_AGGREGATION
/**
 * Atualizar agregação temporal
 */
static void agg_update(int32_t distance_mm) {
    if (!agg_state.valid) {
        agg_state.min_mm = distance_mm;
        agg_state.max_mm = distance_mm;
        agg_state.sum_mm = distance_mm;
        agg_state.count = 1;
        agg_state.valid = true;
    } else {
        if (distance_mm < agg_state.min_mm) agg_state.min_mm = distance_mm;
        if (distance_mm > agg_state.max_mm) agg_state.max_mm = distance_mm;
        agg_state.sum_mm += distance_mm;
        agg_state.count++;
        
        // Reset janela se atingiu tamanho máximo
        if (agg_state.count >= AGG_WINDOW_SIZE) {
            ESP_LOGD(TAG, "Agregação: min=%ld max=%ld avg=%ld (n=%d)",
                     (long)agg_state.min_mm, (long)agg_state.max_mm,
                     (long)(agg_state.sum_mm / agg_state.count), agg_state.count);
        }
    }
}

/**
 * Obter e resetar agregação
 */
static void agg_get_and_reset(int32_t *min, int32_t *max, int32_t *avg) {
    if (agg_state.valid && agg_state.count > 0) {
        *min = agg_state.min_mm;
        *max = agg_state.max_mm;
        *avg = (int32_t)(agg_state.sum_mm / agg_state.count);
    } else {
        *min = *max = *avg = 0;
    }
    // Reset
    agg_state.valid = false;
    agg_state.count = 0;
    agg_state.sum_mm = 0;
}
#endif

#if USE_BINARY_PAYLOAD
/**
 * Enviar pacote binário compacto (16 bytes)
 */
static bool send_telemetry_binary(const telemetry_data_t *data, uint8_t flags) {
    binary_payload_t pkt = {0};
    
    pkt.magic = BINARY_MAGIC;
    memcpy(pkt.mac, node_mac, 6);
    pkt.distance_mm = (int16_t)data->distance_mm;
    pkt.vcc_mv = (uint16_t)data->vcc_bat_mv;
    pkt.rssi = (int8_t)data->rssi;
    pkt.flags = flags;
    
    // CRC sobre os primeiros 14 bytes
    pkt.crc16 = crc16_ccitt((uint8_t*)&pkt, sizeof(pkt) - 2);
    
    ESP_LOGI(TAG, "→ BIN[%d]: dist=%d vcc=%d rssi=%d flags=0x%02X",
             BINARY_PAYLOAD_SIZE, pkt.distance_mm, pkt.vcc_mv, pkt.rssi, pkt.flags);
    
    for (int retry = 0; retry < ESPNOW_MAX_RETRIES; retry++) {
        esp_err_t result = esp_now_send(GATEWAY_MAC, (uint8_t*)&pkt, sizeof(pkt));
        
        if (result == ESP_OK) {
            sensor_state.last_send_time = esp_timer_get_time();
            return true;
        }
        
        ESP_LOGW(TAG, "Retry %d/%d", retry + 1, ESPNOW_MAX_RETRIES);
        vTaskDelay(pdMS_TO_TICKS(ESPNOW_RETRY_MS));
    }
    
    return false;
}
#endif

/**
 * Enviar pacote de telemetria (JSON ou binário)
 * 
 * Formato AGUADA-1 JSON:
 * {"mac":"XX:XX:XX:XX:XX:XX","distance_mm":2450,"vcc_bat_mv":4900,"rssi":-50}
 * 
 * Com RLE:
 * {"mac":"...","distance_mm":2450,"vcc_bat_mv":4900,"rssi":-50,"rle":15}
 * 
 * Com Agregação (no heartbeat):
 * {"mac":"...","distance_mm":2450,"vcc_bat_mv":4900,"rssi":-50,"min_mm":2400,"max_mm":2500,"avg_mm":2450}
 */
static bool send_telemetry(const telemetry_data_t *data, bool is_heartbeat) {
#if USE_BINARY_PAYLOAD
    uint8_t flags = 0;
    if (is_heartbeat) flags |= FLAG_HEARTBEAT;
    else flags |= FLAG_DELTA;
    if (data->distance_mm < 0) flags |= FLAG_ERROR;
    if (data->vcc_bat_mv < VCC_MIN_MV) flags |= FLAG_LOW_BATTERY;
    
    return send_telemetry_binary(data, flags);
#else
    char payload[MAX_PAYLOAD_SIZE];
    int len = 0;
    
    // Base do JSON
    len = snprintf(payload, sizeof(payload),
             "{\"mac\":\"%s\",\"distance_mm\":%ld,\"vcc_bat_mv\":%ld,\"rssi\":%ld",
             node_mac_str, 
             (long)data->distance_mm, 
             (long)data->vcc_bat_mv, 
             (long)data->rssi);

#if USE_RLE
    // Adicionar contador RLE
    len += snprintf(payload + len, sizeof(payload) - len,
                    ",\"rle\":%d", rle_state.stable_count);
#endif

#if USE_AGGREGATION
    // Adicionar agregação no heartbeat
    if (is_heartbeat && agg_state.valid) {
        int32_t min_mm, max_mm, avg_mm;
        agg_get_and_reset(&min_mm, &max_mm, &avg_mm);
        len += snprintf(payload + len, sizeof(payload) - len,
                        ",\"min_mm\":%ld,\"max_mm\":%ld,\"avg_mm\":%ld",
                        (long)min_mm, (long)max_mm, (long)avg_mm);
    }
#endif

    // Fechar JSON
    snprintf(payload + len, sizeof(payload) - len, "}");
    
    ESP_LOGI(TAG, "→ %s", payload);
    
    // Enviar com retries
    for (int retry = 0; retry < ESPNOW_MAX_RETRIES; retry++) {
        esp_err_t result = esp_now_send(GATEWAY_MAC, (uint8_t*)payload, strlen(payload));
        
        if (result == ESP_OK) {
            sensor_state.last_send_time = esp_timer_get_time();
            return true;
        }
        
        ESP_LOGW(TAG, "Retry %d/%d", retry + 1, ESPNOW_MAX_RETRIES);
        vTaskDelay(pdMS_TO_TICKS(ESPNOW_RETRY_MS));
    }
    
    ESP_LOGE(TAG, "Falha ao enviar após %d tentativas", ESPNOW_MAX_RETRIES);
    return false;
#endif
}

/**
 * Verificar se deve enviar (delta com histerese ou heartbeat)
 * 
 * Histerese previne oscilação quando valor fica no limiar do delta
 * 
 * @param is_heartbeat Output: true se envio é por heartbeat
 * @return true se deve enviar
 */
static bool should_send(const telemetry_data_t *current, const telemetry_data_t *last, bool *is_heartbeat) {
    static int32_t trend_direction = 0;  // -1 = descendo, 0 = estável, +1 = subindo
    
    *is_heartbeat = false;
    
    // Primeira leitura - sempre enviar
    if (sensor_state.first_reading) {
        sensor_state.first_reading = false;
        return true;
    }
    
    // Heartbeat (tempo desde último envio)
    int64_t now = esp_timer_get_time();
    int64_t elapsed_ms = (now - sensor_state.last_send_time) / 1000;
    
    if (elapsed_ms >= HEARTBEAT_MS) {
        metrics.heartbeats_sent++;
        trend_direction = 0;  // Reset trend
        *is_heartbeat = true;
        ESP_LOGD(TAG, "Heartbeat (elapsed: %lld ms)", elapsed_ms);
        return true;
    }
    
    // Delta na distância com histerese
    int32_t delta_mm = current->distance_mm - last->distance_mm;
    int32_t abs_delta = abs(delta_mm);
    
    // Determinar threshold baseado na direção da tendência
    int32_t threshold = DELTA_DISTANCE_MM;
    
    if (delta_mm > 0 && trend_direction <= 0) {
        // Mudando de direção (descendo→subindo) - usar threshold + histerese
        threshold = DELTA_DISTANCE_MM + HYSTERESIS_MM;
    } else if (delta_mm < 0 && trend_direction >= 0) {
        // Mudando de direção (subindo→descendo) - usar threshold + histerese
        threshold = DELTA_DISTANCE_MM + HYSTERESIS_MM;
    }
    
    if (abs_delta >= threshold) {
        metrics.deltas_detected++;
        trend_direction = (delta_mm > 0) ? 1 : -1;
        ESP_LOGD(TAG, "Delta distância: %ld mm (threshold: %ld)", (long)delta_mm, (long)threshold);
        return true;
    }
    
    // Delta na tensão
    int32_t delta_vcc = abs(current->vcc_bat_mv - last->vcc_bat_mv);
    if (delta_vcc >= DELTA_VCC_MV) {
        metrics.deltas_detected++;
        ESP_LOGD(TAG, "Delta VCC: %ld mV", (long)delta_vcc);
        return true;
    }
    
    return false;
}

// ============================================================================
// TASK PRINCIPAL
// ============================================================================

/**
 * Task de telemetria
 * 
 * Ciclo:
 * 1. Ler sensor (mediana de 11 amostras, ~1.1s)
 * 2. Atualizar RLE e agregação
 * 3. Verificar se deve enviar (delta ou heartbeat)
 * 4. Enviar se necessário
 * 5. Aguardar próximo ciclo
 */
static void telemetry_task(void *pvParameters) {
    ESP_LOGI(TAG, "Iniciando telemetria (intervalo: %d ms, heartbeat: %d ms)",
             READ_INTERVAL_MS, HEARTBEAT_MS);
    
    sensor_state.first_reading = true;
    sensor_state.last_send_time = esp_timer_get_time();
    
    while (1) {
        // Coletar dados atuais
        telemetry_data_t current = {
            .distance_mm = read_ultrasonic_filtered(),
            .vcc_bat_mv = get_vcc_mv(),
            .rssi = get_rssi(),
            .timestamp = esp_timer_get_time()
        };
        
        // Tratar erros do sensor
        if (current.distance_mm < 0) {
            // Sensor com erro - enviar código de erro
            current.distance_mm = (current.distance_mm == -1) ? 0 : 1;
        }
        
#if USE_RLE
        // Atualizar RLE (conta leituras estáveis)
        rle_update(current.distance_mm);
#endif

#if USE_AGGREGATION
        // Atualizar agregação temporal
        if (current.distance_mm > 0) {
            agg_update(current.distance_mm);
        }
#endif
        
        // Verificar se deve enviar
        bool is_heartbeat = false;
        if (should_send(&current, &sensor_state.last_sent, &is_heartbeat)) {
            if (send_telemetry(&current, is_heartbeat)) {
                sensor_state.last_sent = current;
#if USE_RLE
                // Reset RLE após envio por delta
                if (!is_heartbeat) {
                    rle_state.stable_count = 1;
                }
#endif
            }
        } else {
            ESP_LOGD(TAG, "Sem mudança significativa");
        }
        
        // Log de estatísticas periodicamente
        if (metrics.packets_sent > 0 && metrics.packets_sent % STATS_INTERVAL == 0) {
            ESP_LOGI(TAG, "📊 Stats: TX=%lu OK=%lu FAIL=%lu Delta=%lu HB=%lu",
                     metrics.readings_total, metrics.packets_sent, 
                     metrics.packets_failed, metrics.deltas_detected,
                     metrics.heartbeats_sent);
        }
        
        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }
}

/**
 * Task de heartbeat LED
 * 
 * 3 piscadas rápidas a cada 3 segundos
 */
static void heartbeat_led_task(void *pvParameters) {
    while (1) {
        // 3 piscadas rápidas
        for (int i = 0; i < 3; i++) {
            gpio_set_level(PIN_LED_STATUS, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_set_level(PIN_LED_STATUS, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        
        vTaskDelay(pdMS_TO_TICKS(2400));  // Total = 3s
    }
}

// ============================================================================
// MAIN
// ============================================================================

void app_main(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║           AGUADA - Universal Sensor Node             ║");
    ESP_LOGI(TAG, "╠══════════════════════════════════════════════════════╣");
    ESP_LOGI(TAG, "║  Firmware:  %-40s ║", FIRMWARE_VERSION);
    ESP_LOGI(TAG, "║  Protocolo: %-40s ║", PROTOCOL_VERSION);
    ESP_LOGI(TAG, "║  Hardware:  ESP32-C3 + AJ-SR04M                      ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    
    // Inicializações
    init_gpio();
    init_adc();
    
    // Animação de boot (3 piscadas)
    for (int i = 0; i < 3; i++) {
        gpio_set_level(PIN_LED_STATUS, 1);
        vTaskDelay(pdMS_TO_TICKS(150));
        gpio_set_level(PIN_LED_STATUS, 0);
        vTaskDelay(pdMS_TO_TICKS(150));
    }
    
    init_espnow();
    
    // Criar tasks
    xTaskCreate(telemetry_task, "telemetry", 4096, NULL, 5, NULL);
    xTaskCreate(heartbeat_led_task, "heartbeat", 2048, NULL, 3, NULL);
    
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "✓ Sistema pronto!");
    ESP_LOGI(TAG, "  - Leitura a cada %d ms", READ_INTERVAL_MS);
    ESP_LOGI(TAG, "  - Heartbeat a cada %d ms", HEARTBEAT_MS);
    ESP_LOGI(TAG, "  - Delta mínimo: %d mm", DELTA_DISTANCE_MM);
    ESP_LOGI(TAG, "");
}
