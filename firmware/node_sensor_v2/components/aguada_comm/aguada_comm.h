/**
 * @file aguada_comm.h
 * @brief AGUADA Communication - ESP-NOW interface
 */

#ifndef AGUADA_COMM_H
#define AGUADA_COMM_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_now.h"
#include "aguada_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ESP-NOW configuration */
#define COMM_ESPNOW_CHANNEL     1
#define COMM_ESPNOW_MAX_RETRY   3
#define COMM_ESPNOW_RETRY_DELAY_MS 1000
#define COMM_QUEUE_SIZE         16

/* Gateway MAC address (hardcoded for TYPE_SINGLE_ULTRA) */
#define COMM_GATEWAY_MAC        {0x80, 0xF1, 0xB2, 0x50, 0x2E, 0xC4}

/**
 * @brief Communication statistics
 */
typedef struct {
    uint32_t packets_sent;
    uint32_t packets_failed;
    uint32_t packets_acked;
    int8_t last_rssi;
    uint32_t uptime_sec;
} aguada_comm_stats_t;

/**
 * @brief Send callback type
 */
typedef void (*aguada_comm_send_cb_t)(bool success);

/**
 * @brief Communication configuration
 */
typedef struct {
    uint8_t gateway_mac[6];
    uint8_t channel;
    uint8_t max_retry;
    uint16_t retry_delay_ms;
    aguada_comm_send_cb_t send_callback;
} aguada_comm_config_t;

/**
 * @brief Initialize communication subsystem
 * 
 * Initializes WiFi in STA mode and ESP-NOW
 * 
 * @param config Communication configuration (NULL for defaults)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t aguada_comm_init(const aguada_comm_config_t *config);

/**
 * @brief Deinitialize communication subsystem
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t aguada_comm_deinit(void);

/**
 * @brief Send telemetry packet (v1 format - JSON)
 * 
 * @param packet Packet to send
 * @return esp_err_t ESP_OK on success
 */
esp_err_t aguada_comm_send_v1(const aguada_packet_v1_t *packet);

/**
 * @brief Send telemetry packet (v2 format - JSON with health)
 * 
 * @param packet Packet to send
 * @return esp_err_t ESP_OK on success
 */
esp_err_t aguada_comm_send_v2(const aguada_packet_v2_t *packet);

/**
 * @brief Send binary packet (v2 format - optimized)
 * 
 * @param binary Binary packet to send
 * @return esp_err_t ESP_OK on success
 */
esp_err_t aguada_comm_send_binary(const aguada_binary_v2_t *binary);

/**
 * @brief Get communication statistics
 * 
 * @param stats Output statistics structure
 * @return esp_err_t ESP_OK on success
 */
esp_err_t aguada_comm_get_stats(aguada_comm_stats_t *stats);

/**
 * @brief Get node MAC address
 * 
 * @param mac Output MAC address buffer (6 bytes)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t aguada_comm_get_mac(uint8_t mac[6]);

/**
 * @brief Get default communication configuration
 * 
 * @return aguada_comm_config_t Default configuration
 */
aguada_comm_config_t aguada_comm_get_default_config(void);

/**
 * @brief Check if communication is ready
 * 
 * @return true if ESP-NOW is initialized and ready
 */
bool aguada_comm_is_ready(void);

#ifdef __cplusplus
}
#endif

#endif // AGUADA_COMM_H
