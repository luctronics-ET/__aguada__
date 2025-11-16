/**
 * AGUADA - Gateway WiFi (ESP-IDF Nativo)
 * ESP32-C3 SuperMini - Gateway Central
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_task_wdt.h"

// WiFi Configuration - CONFIGURE AQUI SUA REDE
#define WIFI_SSID      "luciano"
#define WIFI_PASSWORD  "Luciano19852012"

// GPIO Pins
#define LED_BUILTIN    GPIO_NUM_8
#define LED_WIFI       GPIO_NUM_10
#define LED_MQTT       GPIO_NUM_2

static const char *TAG = "GATEWAY";

static bool wifi_connected = false;

// Event handler para WiFi
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "WiFi desconectado, tentando reconectar...");
        wifi_connected = false;
        gpio_set_level(LED_WIFI, 0);
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "WiFi conectado! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connected = true;
        gpio_set_level(LED_WIFI, 1);
    }
}

// Inicializar WiFi
void wifi_init_sta(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    
    // Criar event loop apenas se não existir
    esp_err_t ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(ret);
    }
    
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi inicializado. Conectando a %s...", WIFI_SSID);
}

// Task heartbeat LED
void heartbeat_task(void *pvParameters)
{
    bool led_state = false;
    
    while (1) {
        led_state = !led_state;
        gpio_set_level(LED_BUILTIN, led_state);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Task de status
void status_task(void *pvParameters)
{
    uint32_t uptime = 0;
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(60000)); // 60 segundos
        uptime++;
        
        ESP_LOGI(TAG, "╔═══════════════════════════════════════════╗");
        ESP_LOGI(TAG, "║     AGUADA GATEWAY - STATUS               ║");
        ESP_LOGI(TAG, "╠═══════════════════════════════════════════╣");
        ESP_LOGI(TAG, "║ Uptime:  %lu minutos", uptime);
        ESP_LOGI(TAG, "║ WiFi:    %s", wifi_connected ? "CONECTADO" : "DESCONECTADO");
        ESP_LOGI(TAG, "║ Heap:    %lu bytes livres", esp_get_free_heap_size());
        ESP_LOGI(TAG, "╚═══════════════════════════════════════════╝");
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔═══════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║           AGUADA - Gateway WiFi                           ║");
    ESP_LOGI(TAG, "╠═══════════════════════════════════════════════════════════╣");
    ESP_LOGI(TAG, "║ Gateway:         gateway_00                               ║");
    ESP_LOGI(TAG, "║ Firmware:        v1.0.0                                   ║");
    ESP_LOGI(TAG, "║ ESP-IDF:         v%s                            ║", IDF_VER);
    ESP_LOGI(TAG, "╚═══════════════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");

    // Inicializar NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Configurar GPIOs
    gpio_reset_pin(LED_BUILTIN);
    gpio_set_direction(LED_BUILTIN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(LED_WIFI);
    gpio_set_direction(LED_WIFI, GPIO_MODE_OUTPUT);
    gpio_reset_pin(LED_MQTT);
    gpio_set_direction(LED_MQTT, GPIO_MODE_OUTPUT);

    // Blink de inicialização
    for (int i = 0; i < 3; i++) {
        gpio_set_level(LED_BUILTIN, 1);
        gpio_set_level(LED_WIFI, 1);
        gpio_set_level(LED_MQTT, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(LED_BUILTIN, 0);
        gpio_set_level(LED_WIFI, 0);
        gpio_set_level(LED_MQTT, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Inicializar WiFi
    wifi_init_sta();

    // Criar tasks
    xTaskCreate(heartbeat_task, "heartbeat", 2048, NULL, 5, NULL);
    xTaskCreate(status_task, "status", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "[SETUP] ✓ Gateway inicializado e pronto!");
    ESP_LOGI(TAG, "");

    // Loop principal - sem watchdog manual
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
