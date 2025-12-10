/**
 * @file aguada_comm.c
 * @brief AGUADA Communication - Implementation
 */

#include "aguada_comm.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "esp_crc.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "AGUADA_COMM";

static aguada_comm_config_t s_config;
static aguada_comm_stats_t s_stats = {0};
static bool s_initialized = false;
static uint8_t s_node_mac[6];
static QueueHandle_t s_send_queue = NULL;
static SemaphoreHandle_t s_send_mutex = NULL;

/* ESP-NOW send callback */
static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS)
    {
        s_stats.packets_sent++;
        s_stats.packets_acked++;
        ESP_LOGD(TAG, "Packet sent successfully");

        if (s_config.send_callback)
        {
            s_config.send_callback(true);
        }
    }
    else
    {
        s_stats.packets_failed++;
        ESP_LOGW(TAG, "Packet send failed");

        if (s_config.send_callback)
        {
            s_config.send_callback(false);
        }
    }
}

aguada_comm_config_t aguada_comm_get_default_config(void)
{
    uint8_t default_gateway[] = COMM_GATEWAY_MAC;
    aguada_comm_config_t config = {
        .channel = COMM_ESPNOW_CHANNEL,
        .max_retry = COMM_ESPNOW_MAX_RETRY,
        .retry_delay_ms = COMM_ESPNOW_RETRY_DELAY_MS,
        .send_callback = NULL};
    memcpy(config.gateway_mac, default_gateway, 6);
    return config;
}

esp_err_t aguada_comm_init(const aguada_comm_config_t *config)
{
    if (s_initialized)
    {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    if (config)
    {
        memcpy(&s_config, config, sizeof(aguada_comm_config_t));
    }
    else
    {
        s_config = aguada_comm_get_default_config();
    }

    ESP_LOGI(TAG, "Initializing communication...");

    // Initialize NVS (required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize WiFi in STA mode (required for ESP-NOW)
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(s_config.channel, WIFI_SECOND_CHAN_NONE));

    // Get node MAC address
    ESP_ERROR_CHECK(esp_read_mac(s_node_mac, ESP_MAC_WIFI_STA));

    char mac_str[18];
    aguada_mac_to_string(s_node_mac, mac_str);
    ESP_LOGI(TAG, "Node MAC: %s", mac_str);

    // Initialize ESP-NOW
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));

    // Add gateway as peer
    esp_now_peer_info_t peer_info = {0};
    memcpy(peer_info.peer_addr, s_config.gateway_mac, 6);
    peer_info.channel = s_config.channel;
    peer_info.ifidx = WIFI_IF_STA;
    peer_info.encrypt = false; // No encryption for now

    ret = esp_now_add_peer(&peer_info);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add gateway peer: %s", esp_err_to_name(ret));
        return ret;
    }

    char gw_mac_str[18];
    aguada_mac_to_string(s_config.gateway_mac, gw_mac_str);
    ESP_LOGI(TAG, "Gateway added: %s (channel %d)", gw_mac_str, s_config.channel);

    // Create send queue
    s_send_queue = xQueueCreate(COMM_QUEUE_SIZE, sizeof(void *));
    if (!s_send_queue)
    {
        ESP_LOGE(TAG, "Failed to create send queue");
        return ESP_ERR_NO_MEM;
    }

    // Create send mutex
    s_send_mutex = xSemaphoreCreateMutex();
    if (!s_send_mutex)
    {
        ESP_LOGE(TAG, "Failed to create send mutex");
        vQueueDelete(s_send_queue);
        return ESP_ERR_NO_MEM;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Communication initialized");

    return ESP_OK;
}

esp_err_t aguada_comm_deinit(void)
{
    if (!s_initialized)
    {
        return ESP_OK;
    }

    esp_now_deinit();
    esp_wifi_stop();
    esp_wifi_deinit();

    if (s_send_queue)
    {
        vQueueDelete(s_send_queue);
        s_send_queue = NULL;
    }

    if (s_send_mutex)
    {
        vSemaphoreDelete(s_send_mutex);
        s_send_mutex = NULL;
    }

    s_initialized = false;
    ESP_LOGI(TAG, "Communication deinitialized");

    return ESP_OK;
}

esp_err_t aguada_comm_send_v1(const aguada_packet_v1_t *packet)
{
    if (!s_initialized)
    {
        ESP_LOGE(TAG, "Not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (!packet)
    {
        return ESP_ERR_INVALID_ARG;
    }

    char json_buf[256];
    esp_err_t ret = aguada_build_json_v1(packet, json_buf, sizeof(json_buf));
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to build JSON v1");
        return ret;
    }

    ESP_LOGI(TAG, "→ %s", json_buf);

    // Send with retry logic
    if (xSemaphoreTake(s_send_mutex, pdMS_TO_TICKS(1000)) != pdTRUE)
    {
        ESP_LOGW(TAG, "Failed to acquire send mutex");
        return ESP_ERR_TIMEOUT;
    }

    for (uint8_t retry = 0; retry < s_config.max_retry; retry++)
    {
        ret = esp_now_send(s_config.gateway_mac, (uint8_t *)json_buf, strlen(json_buf));

        if (ret == ESP_OK)
        {
            xSemaphoreGive(s_send_mutex);
            return ESP_OK;
        }

        ESP_LOGW(TAG, "Send failed (retry %d/%d): %s",
                 retry + 1, s_config.max_retry, esp_err_to_name(ret));

        if (retry < s_config.max_retry - 1)
        {
            vTaskDelay(pdMS_TO_TICKS(s_config.retry_delay_ms));
        }
    }

    s_stats.packets_failed++;
    xSemaphoreGive(s_send_mutex);

    return ESP_FAIL;
}

esp_err_t aguada_comm_send_v2(const aguada_packet_v2_t *packet)
{
    if (!s_initialized)
    {
        ESP_LOGE(TAG, "Not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (!packet)
    {
        return ESP_ERR_INVALID_ARG;
    }

    char json_buf[512]; // Larger buffer for v2 with health metrics
    esp_err_t ret = aguada_build_json_v2(packet, json_buf, sizeof(json_buf));
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to build JSON v2");
        return ret;
    }

    ESP_LOGI(TAG, "→ %s", json_buf);

    // Send with retry logic
    if (xSemaphoreTake(s_send_mutex, pdMS_TO_TICKS(1000)) != pdTRUE)
    {
        ESP_LOGW(TAG, "Failed to acquire send mutex");
        return ESP_ERR_TIMEOUT;
    }

    for (uint8_t retry = 0; retry < s_config.max_retry; retry++)
    {
        ret = esp_now_send(s_config.gateway_mac, (uint8_t *)json_buf, strlen(json_buf));

        if (ret == ESP_OK)
        {
            xSemaphoreGive(s_send_mutex);
            return ESP_OK;
        }

        ESP_LOGW(TAG, "Send failed (retry %d/%d): %s",
                 retry + 1, s_config.max_retry, esp_err_to_name(ret));

        if (retry < s_config.max_retry - 1)
        {
            vTaskDelay(pdMS_TO_TICKS(s_config.retry_delay_ms));
        }
    }

    s_stats.packets_failed++;
    xSemaphoreGive(s_send_mutex);

    return ESP_FAIL;
}

esp_err_t aguada_comm_send_binary(const aguada_binary_v2_t *binary)
{
    if (!s_initialized)
    {
        ESP_LOGE(TAG, "Not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (!binary)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Verify CRC before sending
    if (!aguada_verify_binary(binary))
    {
        ESP_LOGE(TAG, "Binary packet CRC mismatch");
        return ESP_ERR_INVALID_CRC;
    }

    ESP_LOGI(TAG, "→ Binary packet (32 bytes)");

    // Send with retry logic
    if (xSemaphoreTake(s_send_mutex, pdMS_TO_TICKS(1000)) != pdTRUE)
    {
        ESP_LOGW(TAG, "Failed to acquire send mutex");
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t ret;
    for (uint8_t retry = 0; retry < s_config.max_retry; retry++)
    {
        ret = esp_now_send(s_config.gateway_mac, (uint8_t *)binary, sizeof(aguada_binary_v2_t));

        if (ret == ESP_OK)
        {
            xSemaphoreGive(s_send_mutex);
            return ESP_OK;
        }

        ESP_LOGW(TAG, "Send failed (retry %d/%d): %s",
                 retry + 1, s_config.max_retry, esp_err_to_name(ret));

        if (retry < s_config.max_retry - 1)
        {
            vTaskDelay(pdMS_TO_TICKS(s_config.retry_delay_ms));
        }
    }

    s_stats.packets_failed++;
    xSemaphoreGive(s_send_mutex);

    return ESP_FAIL;
}

esp_err_t aguada_comm_get_stats(aguada_comm_stats_t *stats)
{
    if (!stats)
    {
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(stats, &s_stats, sizeof(aguada_comm_stats_t));
    return ESP_OK;
}

esp_err_t aguada_comm_get_mac(uint8_t mac[6])
{
    if (!mac)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    memcpy(mac, s_node_mac, 6);
    return ESP_OK;
}

bool aguada_comm_is_ready(void)
{
    return s_initialized;
}
