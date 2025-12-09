/*
 * AGUADA NODE SENSOR 20 - DUAL ULTRASONIC (IE01 + IE02)
 * Version: v1.2.0-fixed-channel
 * 
 * Firmware para monitoramento de 2 reservatórios com um único ESP32-C3:
 * - IE01 (Cisterna Ilha do Engenho 01)
 * - IE02 (Cisterna Ilha do Engenho 02)
 * 
 * Hardware:
 * - 2x Sensor Ultrassônico AJ-SR04M
 * - 2x Detector de som (GPIO)
 * - 4x Válvulas (2 por reservatório)
 * 
 * Protocolo ESP-NOW:
 * - Canal fixo 11 (sem auto-discovery)
 * - Transmissão individual por variável
 * - Format: {"mac":"XX:XX","type":"IE01_distance_cm","value":12345,...}
 * 
 * ESP-IDF 6.x Compatible
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_mac.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "config.h"

static const char *TAG = "AGUADA_NODE20";

// MAC address do node (auto-detectado)
static char node_mac_str[18];

// Últimas leituras (para deadband)
static int last_ie01_distance = -1;
static int last_ie02_distance = -1;
static uint8_t last_ie01_valve_in = 255;
static uint8_t last_ie01_valve_out = 255;
static uint8_t last_ie02_valve_in = 255;
static uint8_t last_ie02_valve_out = 255;
static uint8_t last_ie01_sound = 255;
static uint8_t last_ie02_sound = 255;

// Estatísticas ESP-NOW
static uint32_t packets_sent = 0;
static uint32_t packets_failed = 0;

// ============================================================================
// Callback ESP-NOW (IDF 6.x)
// ============================================================================
void espnow_send_cb(const esp_now_send_info_t *info, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        packets_sent++;
    } else {
        packets_failed++;
        ESP_LOGW(TAG, "ESP-NOW send failed (total: %lu)", packets_failed);
    }
}

// ============================================================================
// Inicialização WiFi/ESP-NOW
// ============================================================================
void init_espnow(void) {
    // Inicializar NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Inicializar WiFi
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));

    // Obter MAC address
    uint8_t node_mac[6];
    esp_efuse_mac_get_default(node_mac);
    snprintf(node_mac_str, sizeof(node_mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
             node_mac[0], node_mac[1], node_mac[2], node_mac[3], node_mac[4], node_mac[5]);
    
    ESP_LOGI(TAG, "Node MAC: %s", node_mac_str);
    ESP_LOGI(TAG, "✓ Canal ESP-NOW: %d (fixo)", ESPNOW_CHANNEL);

    // Inicializar ESP-NOW
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));

    // Adicionar gateway como peer
    esp_now_peer_info_t peer_info = {};
    memcpy(peer_info.peer_addr, gateway_mac, 6);
    peer_info.channel = ESPNOW_CHANNEL;
    peer_info.encrypt = false;
    ESP_ERROR_CHECK(esp_now_add_peer(&peer_info));

    ESP_LOGI(TAG, "ESP-NOW OK - Gateway: %02X:%02X:%02X:%02X:%02X:%02X",
             gateway_mac[0], gateway_mac[1], gateway_mac[2], gateway_mac[3], gateway_mac[4], gateway_mac[5]);
}

// ============================================================================
// Inicialização GPIO
// ============================================================================
void init_gpio(void) {
    // IE01 - Ultrasonic
    gpio_set_direction(IE01_TRIG_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(IE01_ECHO_PIN, GPIO_MODE_INPUT);
    gpio_set_level(IE01_TRIG_PIN, 0);

    // IE02 - Ultrasonic
    gpio_set_direction(IE02_TRIG_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(IE02_ECHO_PIN, GPIO_MODE_INPUT);
    gpio_set_level(IE02_TRIG_PIN, 0);

    // IE01 - Válvulas e Som
    gpio_set_direction(IE01_VALVE_IN_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(IE01_VALVE_OUT_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(IE01_SOUND_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(IE01_VALVE_IN_PIN, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(IE01_VALVE_OUT_PIN, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(IE01_SOUND_PIN, GPIO_PULLDOWN_ONLY);

    // IE02 - Válvulas e Som
    gpio_set_direction(IE02_VALVE_IN_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(IE02_VALVE_OUT_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(IE02_SOUND_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(IE02_VALVE_IN_PIN, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(IE02_VALVE_OUT_PIN, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(IE02_SOUND_PIN, GPIO_PULLDOWN_ONLY);

    // LED de status
    gpio_set_direction(LED_STATUS, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_STATUS, 0);

    ESP_LOGI(TAG, "GPIO inicializado (IE01: trig=%d echo=%d | IE02: trig=%d echo=%d)",
             IE01_TRIG_PIN, IE01_ECHO_PIN, IE02_TRIG_PIN, IE02_ECHO_PIN);
}

// ============================================================================
// Leitura do sensor ultrassônico
// ============================================================================
int read_ultrasonic_distance(gpio_num_t trig_pin, gpio_num_t echo_pin) {
    // Enviar pulso trigger (10μs)
    gpio_set_level(trig_pin, 0);
    esp_rom_delay_us(2);
    gpio_set_level(trig_pin, 1);
    esp_rom_delay_us(10);
    gpio_set_level(trig_pin, 0);

    // Aguardar echo subir (timeout 30ms)
    int64_t start_time = esp_timer_get_time();
    while (gpio_get_level(echo_pin) == 0) {
        if ((esp_timer_get_time() - start_time) > TIMEOUT_US) {
            return -1; // Timeout
        }
    }

    // Medir duração do pulso echo
    int64_t pulse_start = esp_timer_get_time();
    while (gpio_get_level(echo_pin) == 1) {
        if ((esp_timer_get_time() - pulse_start) > TIMEOUT_US) {
            return -1; // Timeout
        }
    }
    int64_t pulse_end = esp_timer_get_time();

    uint32_t duration = (uint32_t)(pulse_end - pulse_start);

    // Calcular distância (cm × 100)
    // Velocidade do som: 343 m/s = 0.0343 cm/μs
    // Distância = (duração × 0.034) / 2
    // Simplificado: (duração × 343) / 20000
    int distance_cm_x100 = (int)((duration * 343) / 20000);

    // Validar range (20-450 cm)
    if (distance_cm_x100 < (MIN_DISTANCE_CM * 100) || 
        distance_cm_x100 > (MAX_DISTANCE_CM * 100)) {
        return -2; // Fora do range
    }

    return distance_cm_x100;
}

// ============================================================================
// Filtro de mediana
// ============================================================================
int compare(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

int read_distance_filtered(gpio_num_t trig_pin, gpio_num_t echo_pin, const char *sensor_id) {
    int samples[SAMPLES_FOR_MEDIAN];
    int valid_count = 0;

    for (int i = 0; i < SAMPLES_FOR_MEDIAN; i++) {
        int dist = read_ultrasonic_distance(trig_pin, echo_pin);
        if (dist > 0) {
            samples[valid_count++] = dist;
        }
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
    }

    if (valid_count < 5) {
        ESP_LOGW(TAG, "%s: Poucas amostras válidas (%d/11)", sensor_id, valid_count);
        return -1;
    }

    // Ordenar e retornar mediana
    qsort(samples, valid_count, sizeof(int), compare);
    int median = samples[valid_count / 2];

    ESP_LOGI(TAG, "%s: %d.%02d cm (%d amostras)", 
             sensor_id, median/100, median%100, valid_count);

    return median;
}

// ============================================================================
// Envio de telemetria ESP-NOW
// ============================================================================
void send_telemetry(const char *type, int value) {
    char payload[200];
    uint32_t uptime = esp_timer_get_time() / 1000000; // μs → segundos
    int rssi = -50; // TODO: Ler RSSI real
    int battery = 5000; // 5V DC

    snprintf(payload, sizeof(payload),
             "{\"mac\":\"%s\",\"type\":\"%s\",\"value\":%d,"
             "\"battery\":%d,\"uptime\":%lu,\"rssi\":%d}",
             node_mac_str, type, value, battery, uptime, rssi);

    ESP_LOGI(TAG, "→ %s", payload);

    esp_err_t result = esp_now_send(gateway_mac, (uint8_t *)payload, strlen(payload));
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "ESP-NOW send error: %d", result);
    }
}

// ============================================================================
// Verificar mudanças e enviar (deadband logic)
// ============================================================================
void check_and_send_changes(void) {
    // ========== IE01 - Distância ==========
    int ie01_distance = read_distance_filtered(IE01_TRIG_PIN, IE01_ECHO_PIN, SENSOR_IE01_ID);
    
    if (ie01_distance > 0) {
        if (last_ie01_distance < 0 || 
            abs(ie01_distance - last_ie01_distance) >= (DEADBAND_CM * VALUE_MULTIPLIER)) {
            send_telemetry("IE01_distance_cm", ie01_distance);
            last_ie01_distance = ie01_distance;
        }
    } else if (ie01_distance == -1) {
        send_telemetry("IE01_distance_cm", 0); // Timeout
        last_ie01_distance = -1;
    } else if (ie01_distance == -2) {
        send_telemetry("IE01_distance_cm", 1); // Fora do range
        last_ie01_distance = -2;
    }

    // ========== IE02 - Distância ==========
    int ie02_distance = read_distance_filtered(IE02_TRIG_PIN, IE02_ECHO_PIN, SENSOR_IE02_ID);
    
    if (ie02_distance > 0) {
        if (last_ie02_distance < 0 || 
            abs(ie02_distance - last_ie02_distance) >= (DEADBAND_CM * VALUE_MULTIPLIER)) {
            send_telemetry("IE02_distance_cm", ie02_distance);
            last_ie02_distance = ie02_distance;
        }
    } else if (ie02_distance == -1) {
        send_telemetry("IE02_distance_cm", 0); // Timeout
        last_ie02_distance = -1;
    } else if (ie02_distance == -2) {
        send_telemetry("IE02_distance_cm", 1); // Fora do range
        last_ie02_distance = -2;
    }

    // ========== IE01 - Válvulas ==========
    uint8_t ie01_valve_in = gpio_get_level(IE01_VALVE_IN_PIN);
    if (last_ie01_valve_in == 255 || ie01_valve_in != last_ie01_valve_in) {
        send_telemetry("IE01_valve_in", ie01_valve_in);
        last_ie01_valve_in = ie01_valve_in;
    }

    uint8_t ie01_valve_out = gpio_get_level(IE01_VALVE_OUT_PIN);
    if (last_ie01_valve_out == 255 || ie01_valve_out != last_ie01_valve_out) {
        send_telemetry("IE01_valve_out", ie01_valve_out);
        last_ie01_valve_out = ie01_valve_out;
    }

    // ========== IE02 - Válvulas ==========
    uint8_t ie02_valve_in = gpio_get_level(IE02_VALVE_IN_PIN);
    if (last_ie02_valve_in == 255 || ie02_valve_in != last_ie02_valve_in) {
        send_telemetry("IE02_valve_in", ie02_valve_in);
        last_ie02_valve_in = ie02_valve_in;
    }

    uint8_t ie02_valve_out = gpio_get_level(IE02_VALVE_OUT_PIN);
    if (last_ie02_valve_out == 255 || ie02_valve_out != last_ie02_valve_out) {
        send_telemetry("IE02_valve_out", ie02_valve_out);
        last_ie02_valve_out = ie02_valve_out;
    }

    // ========== IE01 - Som ==========
    uint8_t ie01_sound = gpio_get_level(IE01_SOUND_PIN);
    if (last_ie01_sound == 255 || ie01_sound != last_ie01_sound) {
        send_telemetry("IE01_sound_in", ie01_sound);
        last_ie01_sound = ie01_sound;
    }

    // ========== IE02 - Som ==========
    uint8_t ie02_sound = gpio_get_level(IE02_SOUND_PIN);
    if (last_ie02_sound == 255 || ie02_sound != last_ie02_sound) {
        send_telemetry("IE02_sound_in", ie02_sound);
        last_ie02_sound = ie02_sound;
    }
}

// ============================================================================
// Task de telemetria (heartbeat 30s)
// ============================================================================
void telemetry_task(void *pvParameters) {
    ESP_LOGI(TAG, "Telemetry task iniciada");
    
    while (1) {
        check_and_send_changes();
        vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_INTERVAL_MS));
    }
}

// ============================================================================
// Task de blink do LED (heartbeat visual)
// ============================================================================
void led_blink_task(void *pvParameters) {
    while (1) {
        gpio_set_level(LED_STATUS, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(LED_STATUS, 0);
        vTaskDelay(pdMS_TO_TICKS(BLINK_INTERVAL_MS - 100));
    }
}

// ============================================================================
// MAIN
// ============================================================================
void app_main(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  AGUADA - Dual Ultrasonic Node (IE01+IE02)  ║");
    ESP_LOGI(TAG, "╠══════════════════════════════════════════════╣");
    ESP_LOGI(TAG, "║ Firmware:    %-32s║", FIRMWARE_VERSION);
    ESP_LOGI(TAG, "║ Hardware:    ESP32-C3 + 2x AJ-SR04M          ║");
    ESP_LOGI(TAG, "║ Protocol:    ESP-NOW → Gateway → MQTT        ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");

    // Inicializar componentes
    init_gpio();
    init_espnow();

    // Criar tasks
    xTaskCreate(telemetry_task, "telemetry", 4096, NULL, 5, NULL);
    xTaskCreate(led_blink_task, "led_blink", 2048, NULL, 1, NULL);

    ESP_LOGI(TAG, "✓ Sistema pronto!");
    ESP_LOGI(TAG, "");
}
