/**
 * AGUADA - Firmware Universal para Sensor Nodes
 * 
 * Hardware: ESP32-C3 SuperMini + AJ-SR04M
 * Protocolo: ESP-NOW → Gateway → MQTT
 * 
 * Recursos por Node:
 * - 1 sensor ultrassônico (distance_cm)
 * HOLD - 2 válvulas digitais (valve_in, valve_out)
 * HOLD- 1 detector de som (sound_in - água entrando)
 * - RSSI, Battery, Uptime
 * 
 * Transmissão Individual:
 * Cada variável é enviada separadamente quando muda
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_now.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "config.h"

static const char *TAG = "AGUADA_NODE";

// ========== VARIÁVEIS GLOBAIS ==========
static uint8_t node_mac[6];
static char node_mac_str[18];
static uint8_t espnow_channel = 0;  // Canal ESP-NOW (0 = não configurado)

// Últimos valores conhecidos
static int last_distance_cm = -1;  // Multiplicado por 100
static uint8_t last_valve_in = 255;  // 255 = desconhecido
static uint8_t last_valve_out = 255;
static uint8_t last_sound_in = 255;

// Estatísticas
static uint32_t packets_sent = 0;
static uint32_t packets_failed = 0;

// ========== PROTÓTIPOS ==========
void init_gpio(void);
void init_espnow(void);
int read_ultrasonic_distance(void);
int read_distance_filtered(void);
void send_telemetry(const char *type, int value);
void check_and_send_changes(void);
uint8_t read_valve_in(void);
uint8_t read_valve_out(void);
uint8_t read_sound_in(void);
void telemetry_task(void *pvParameters);
void heartbeat_task(void *pvParameters);
void espnow_send_cb(const esp_now_send_info_t *info, esp_now_send_status_t status);
// ========== INICIALIZAÇÃO GPIO ==========
void init_gpio(void) {
    // Sensor ultrassônico
    gpio_reset_pin(TRIG_PIN);
    gpio_set_direction(TRIG_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(TRIG_PIN, 0);
    
    gpio_reset_pin(ECHO_PIN);
    gpio_set_direction(ECHO_PIN, GPIO_MODE_INPUT);
    
    // Válvulas (INPUT - apenas leitura de estado)
    gpio_reset_pin(VALVE_IN_PIN);
    gpio_set_direction(VALVE_IN_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(VALVE_IN_PIN, GPIO_PULLDOWN_ONLY);
    
    gpio_reset_pin(VALVE_OUT_PIN);
    gpio_set_direction(VALVE_OUT_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(VALVE_OUT_PIN, GPIO_PULLDOWN_ONLY);
    
    // Detector de som (INPUT)
    gpio_reset_pin(SOUND_IN_PIN);
    gpio_set_direction(SOUND_IN_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SOUND_IN_PIN, GPIO_PULLDOWN_ONLY);
    
    // LED Status
    gpio_reset_pin(LED_STATUS);
    gpio_set_direction(LED_STATUS, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_STATUS, 0);
    
    ESP_LOGI(TAG, "GPIO inicializado (TRIG=%d, ECHO=%d, VALVE_IN=%d, VALVE_OUT=%d, SOUND=%d)", 
             TRIG_PIN, ECHO_PIN, VALVE_IN_PIN, VALVE_OUT_PIN, SOUND_IN_PIN);
}

// ========== NVS - CARREGAR CANAL ==========
// ========== NVS - SALVAR CANAL ==========
// ========== ESP-NOW RECEIVE CALLBACK (para ACK) ==========
// ========== ENVIAR DISCOVERY PACKET ==========
// ========== AUTO-DISCOVERY DE CANAL ==========
// ========== LEITURA SENSOR ULTRASSÔNICO ==========
// Retorna:
//  > 0: distância válida (cm * 100)
//  -1: timeout (sensor não respondeu - retorna 0)
//  -2: fora de range (sensor respondeu mas valor inválido - retorna 1)
int read_ultrasonic_distance(void) {
    // Enviar pulso TRIG (10us)
    gpio_set_level(TRIG_PIN, 0);
    esp_rom_delay_us(2);
    gpio_set_level(TRIG_PIN, 1);
    esp_rom_delay_us(10);
    gpio_set_level(TRIG_PIN, 0);
    
    // Aguardar ECHO subir
    int64_t timeout = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 0) {
        if ((esp_timer_get_time() - timeout) > TIMEOUT_US) {
            return -1;  // Timeout - sensor não respondeu
        }
    }
    
    // Medir duração do pulso ECHO
    int64_t start = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 1) {
        if ((esp_timer_get_time() - start) > TIMEOUT_US) {
            return -1;  // Timeout - pulso muito longo
        }
    }
    int64_t duration = esp_timer_get_time() - start;
    
    // Calcular distância em cm
    // Velocidade do som: 343 m/s = 0.0343 cm/μs
    // Distância = (duração * 0.0343) / 2
    // Para evitar float: distance_cm = (duration * 343) / (2 * 10000) = (duration * 343) / 20000
    // Multiplicado por 100: distance_cm_x100 = (duration * 343) / (2 * 100) = (duration * 343) / 200
    int distance_cm_x100 = (int)((duration * 343) / 200);
    
    ESP_LOGD(TAG, "Duration: %lld μs, Distance: %d.%02d cm", 
             duration, distance_cm_x100 / 100, distance_cm_x100 % 100);
    
    // Validar range
    if (distance_cm_x100 < (MIN_DISTANCE_CM * 100) || distance_cm_x100 > (MAX_DISTANCE_CM * 100)) {
        ESP_LOGW(TAG, "Fora de range: %d.%02d cm (válido: %d-%d cm)", 
                 distance_cm_x100 / 100, distance_cm_x100 % 100, 
                 MIN_DISTANCE_CM, MAX_DISTANCE_CM);
        return -2;  // Fora de range - sensor leu mas valor inválido
    }
    
    return distance_cm_x100;
}

// ========== FILTRO DE MEDIANA ==========
int read_distance_filtered(void) {
    int samples[SAMPLES_FOR_MEDIAN];
    int valid_samples = 0;
    
    // Coletar N amostras
    for (int i = 0; i < SAMPLES_FOR_MEDIAN; i++) {
        int dist = read_ultrasonic_distance();
        if (dist > 0) {
            samples[valid_samples++] = dist;
        }
        vTaskDelay(pdMS_TO_TICKS(200));  // 200ms entre leituras
    }
    
    if (valid_samples == 0) {
        ESP_LOGE(TAG, "Nenhuma amostra válida");
        return -1;
    }
    
    // Ordenar (bubble sort)
    for (int i = 0; i < valid_samples - 1; i++) {
        for (int j = 0; j < valid_samples - i - 1; j++) {
            if (samples[j] > samples[j + 1]) {
                int temp = samples[j];
                samples[j] = samples[j + 1];
                samples[j + 1] = temp;
            }
        }
    }
    
    // Retornar mediana
    int median = samples[valid_samples / 2];
    ESP_LOGI(TAG, "Distância: %d.%02d cm (%d amostras)", 
             median / 100, median % 100, valid_samples);
    
    return median;
}

// ========== LEITURA ESTADOS DIGITAIS ==========
uint8_t read_valve_in(void) {
    return gpio_get_level(VALVE_IN_PIN);
}

uint8_t read_valve_out(void) {
    return gpio_get_level(VALVE_OUT_PIN);
}

uint8_t read_sound_in(void) {
    return gpio_get_level(SOUND_IN_PIN);
}

// ========== ESP-NOW CALLBACK ==========
void espnow_send_cb(const esp_now_send_info_t *info, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        packets_sent++;
    } else {
        packets_failed++;
    }
}

// ========== INICIALIZAÇÃO ESP-NOW ==========
void init_espnow(void) {
    // Inicializar NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Inicializar WiFi em modo STA
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Obter MAC address
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, node_mac));
    snprintf(node_mac_str, sizeof(node_mac_str), 
             "%02X:%02X:%02X:%02X:%02X:%02X",
             node_mac[0], node_mac[1], node_mac[2],
             node_mac[3], node_mac[4], node_mac[5]);
    
    ESP_LOGI(TAG, "Node MAC: %s", node_mac_str);
    
    // ========== CONFIGURAR CANAL FIXO ==========
    espnow_channel = ESPNOW_CHANNEL;
    ESP_ERROR_CHECK(esp_wifi_set_channel(espnow_channel, WIFI_SECOND_CHAN_NONE));
    ESP_LOGI(TAG, "✓ Canal ESP-NOW: %d (fixo)", espnow_channel);
    
    // Inicializar ESP-NOW
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));
    
    // Adicionar gateway como peer
    esp_now_peer_info_t peer_info = {};
    memcpy(peer_info.peer_addr, gateway_mac, 6);
    peer_info.channel = espnow_channel;
    peer_info.encrypt = false;
    
    ESP_ERROR_CHECK(esp_now_add_peer(&peer_info));
    
    ESP_LOGI(TAG, "ESP-NOW OK - Gateway: %02X:%02X:%02X:%02X:%02X:%02X (CH%d)",
             gateway_mac[0], gateway_mac[1], gateway_mac[2],
             gateway_mac[3], gateway_mac[4], gateway_mac[5], espnow_channel);
}

// ========== ENVIO INDIVIDUAL DE TELEMETRIA ==========
void send_telemetry(const char *type, int value) {
    // Ler RSSI real do WiFi
    int rssi = 0;
    esp_wifi_sta_get_rssi(&rssi);
    if (rssi == 0) {
        rssi = -50;  // Fallback se não conseguir ler
    }
    
    int battery = 5000;
    uint32_t uptime = esp_timer_get_time() / 1000000;
    
    char payload[200];
    snprintf(payload, sizeof(payload),
             "{\"mac\":\"%s\",\"type\":\"%s\",\"value\":%d,\"battery\":%d,\"uptime\":%lu,\"rssi\":%d}",
             node_mac_str, type, value, battery, uptime, rssi);
    
    ESP_LOGI(TAG, "→ %s", payload);
    
    // Enviar via ESP-NOW com retries
    for (int retry = 0; retry < MAX_RETRIES; retry++) {
        esp_err_t result = esp_now_send(gateway_mac, (uint8_t *)payload, strlen(payload));
        
        if (result == ESP_OK) {
            break;
        }
        
        vTaskDelay(pdMS_TO_TICKS(RETRY_DELAY_MS));
    }
}

// ========== VERIFICAR E ENVIAR MUDANÇAS ==========
void check_and_send_changes(void) {
    // Ler distância
    int distance_cm = read_distance_filtered();
    
    // Ler estados digitais (uma única vez por ciclo)
    uint8_t valve_in = read_valve_in();
    uint8_t valve_out = read_valve_out();
    uint8_t sound_in = read_sound_in();
    
    // Processar distance_cm
    if (distance_cm > 0) {
        // Primeira leitura ou mudança além do deadband
        if (last_distance_cm < 0 || 
            abs(distance_cm - last_distance_cm) >= (DEADBAND_CM * 100)) {
            send_telemetry("distance_cm", distance_cm);
            last_distance_cm = distance_cm;
        } else {
            ESP_LOGD(TAG, "Distância estável (deadband=%dcm)", DEADBAND_CM);
        }
    } else if (distance_cm == -1) {
        // Timeout (sensor não respondeu)
        if (last_distance_cm != -1) {
            send_telemetry("distance_cm", 0);  // Enviar erro code 0
            last_distance_cm = -1;
        }
    } else if (distance_cm == -2) {
        // Fora de range
        if (last_distance_cm != -2) {
            send_telemetry("distance_cm", 1);  // Enviar erro code 1
            last_distance_cm = -2;
        }
    }
    
    // Processar valve_in (enviar em primeira leitura ou mudança)
    if (last_valve_in == 255 || valve_in != last_valve_in) {
        send_telemetry("valve_in", valve_in);
        last_valve_in = valve_in;
    }
    
    // Processar valve_out (enviar em primeira leitura ou mudança)
    if (last_valve_out == 255 || valve_out != last_valve_out) {
        send_telemetry("valve_out", valve_out);
        last_valve_out = valve_out;
    }
    
    // Processar sound_in (enviar em primeira leitura ou mudança)
    if (last_sound_in == 255 || sound_in != last_sound_in) {
        send_telemetry("sound_in", sound_in);
        last_sound_in = sound_in;
    }
}

// ========== TASK DE TELEMETRIA ==========
void telemetry_task(void *pvParameters) {
    ESP_LOGI(TAG, "Telemetria: intervalo %d ms", HEARTBEAT_INTERVAL_MS);
    
    uint32_t cycle_count = 0;

    while (1) {
        // A cada ciclo, verifica mudanças e envia
        // Apenas envia variáveis quando mudam ou na primeira leitura.
        // Não há envio periódico de heartbeat dos últimos valores conhecidos.
        check_and_send_changes();

        cycle_count++;
        
        // Log de estatísticas a cada 10 pacotes enviados
        if (packets_sent > 0 && packets_sent % 10 == 0) {
            ESP_LOGI(TAG, "Stats - OK:%lu FAIL:%lu (ciclo %lu)", 
                     packets_sent, packets_failed, cycle_count);
        }

        vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_INTERVAL_MS));
    }
}

// ========== TASK DE HEARTBEAT (LED) ==========
void heartbeat_task(void *pvParameters) {
    while (1) {
        gpio_set_level(LED_STATUS, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(LED_STATUS, 0);
        vTaskDelay(pdMS_TO_TICKS(2900));
    }
}

// ========== MAIN ==========
void app_main(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║     AGUADA - Universal Sensor Node          ║");
    ESP_LOGI(TAG, "╠══════════════════════════════════════════════╣");
    ESP_LOGI(TAG, "║ Firmware:    %s                        ║", FIRMWARE_VERSION);
    ESP_LOGI(TAG, "║ Hardware:    ESP32-C3 + AJ-SR04M             ║");
    ESP_LOGI(TAG, "║ Protocol:    ESP-NOW → Gateway → MQTT        ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    
    init_gpio();
    
    // Animação de boot
    for (int i = 0; i < 3; i++) {
        gpio_set_level(LED_STATUS, 1);
        vTaskDelay(pdMS_TO_TICKS(200));
        gpio_set_level(LED_STATUS, 0);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    
    init_espnow();
    
    xTaskCreate(telemetry_task, "telemetry", 4096, NULL, 5, NULL);
    xTaskCreate(heartbeat_task, "heartbeat", 2048, NULL, 3, NULL);
    
    ESP_LOGI(TAG, "✓ Sistema pronto!");
    ESP_LOGI(TAG, "");
    
    vTaskDelay(pdMS_TO_TICKS(2000));
    check_and_send_changes();
}
