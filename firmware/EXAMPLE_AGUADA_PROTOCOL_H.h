/**
 * @file aguada_protocol.h
 * @brief AGUADA Protocol Definitions (v1 and v2)
 * 
 * This header defines the AGUADA telemetry protocol format.
 * Supports backward compatibility with AGUADA-1 and new AGUADA-2.
 * 
 * @author AGUADA Project
 * @version 2.0.0
 */

#ifndef AGUADA_PROTOCOL_H
#define AGUADA_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// PROTOCOL CONSTANTS
// ============================================================================

#define AGUADA_PROTOCOL_V1      1
#define AGUADA_PROTOCOL_V2      2

#define AGUADA_MAGIC_V1         0xAD01  // AGUADA-1
#define AGUADA_MAGIC_V2         0xAD02  // AGUADA-2

#define AGUADA_MAX_MAC_STR      18      // "XX:XX:XX:XX:XX:XX\0"
#define AGUADA_MAX_JSON_SIZE    512     // Max JSON payload

// Error codes for distance readings
#define AGUADA_DISTANCE_TIMEOUT     0   // Sensor timeout
#define AGUADA_DISTANCE_OUT_RANGE   1   // Out of valid range

// ============================================================================
// ERROR CODES
// ============================================================================

typedef enum {
    AGUADA_OK = 0,
    AGUADA_ERR_INVALID_ARG,
    AGUADA_ERR_NO_MEM,
    AGUADA_ERR_TIMEOUT,
    AGUADA_ERR_COMM_FAIL,
    AGUADA_ERR_SENSOR_FAIL,
    AGUADA_ERR_NOT_INITIALIZED,
    AGUADA_ERR_CONFIG_FAIL
} aguada_error_t;

// ============================================================================
// AGUADA-1 PROTOCOL (Legacy - JSON only)
// ============================================================================

/**
 * @brief AGUADA-1 JSON format
 * 
 * Example:
 * {
 *   "mac": "80:F1:B2:50:31:34",
 *   "distance_mm": 2450,
 *   "vcc_bat_mv": 5000,
 *   "rssi": -50
 * }
 */
typedef struct {
    char mac[AGUADA_MAX_MAC_STR];
    int32_t distance_mm;
    int32_t vcc_bat_mv;
    int32_t rssi;
} aguada1_packet_t;

// ============================================================================
// AGUADA-2 PROTOCOL (Enhanced)
// ============================================================================

/**
 * @brief Reboot reasons
 */
typedef enum {
    REBOOT_REASON_UNKNOWN = 0,
    REBOOT_REASON_POWER_ON,
    REBOOT_REASON_SW_RESET,
    REBOOT_REASON_WATCHDOG,
    REBOOT_REASON_PANIC,
    REBOOT_REASON_OTA
} aguada_reboot_reason_t;

/**
 * @brief Health metrics structure
 */
typedef struct {
    uint32_t uptime_s;          ///< Uptime in seconds
    uint32_t free_heap;         ///< Free heap memory (bytes)
    uint32_t min_heap_ever;     ///< Minimum heap ever (bytes)
    int8_t cpu_temp;            ///< CPU temperature (°C)
    aguada_reboot_reason_t reboot_reason;
    char fw_version[16];        ///< Firmware version string
    uint32_t tx_ok;             ///< Packets sent successfully
    uint32_t tx_fail;           ///< Packets failed
    uint16_t sensor_errors;     ///< Sensor read errors
} aguada_health_t;

/**
 * @brief AGUADA-2 JSON format (with health)
 * 
 * Example:
 * {
 *   "v": 2,
 *   "mac": "80:F1:B2:50:31:34",
 *   "ts": 1702234567,
 *   "data": {
 *     "distance_mm": 2450,
 *     "vcc_bat_mv": 4200,
 *     "rssi": -50
 *   },
 *   "health": {
 *     "uptime_s": 86400,
 *     "free_heap": 180000,
 *     "min_heap": 150000,
 *     "cpu_temp": 45,
 *     "reboot_reason": 1,
 *     "fw_version": "2.0.1",
 *     "tx_ok": 2880,
 *     "tx_fail": 5,
 *     "sensor_errors": 2
 *   }
 * }
 */
typedef struct {
    uint8_t version;            ///< Protocol version (2)
    char mac[AGUADA_MAX_MAC_STR];
    uint32_t timestamp;         ///< Unix timestamp
    
    // Sensor data
    int32_t distance_mm;
    int32_t vcc_bat_mv;
    int32_t rssi;
    
    // Health metrics
    aguada_health_t health;
} aguada2_packet_t;

/**
 * @brief AGUADA-2 Binary format (compact, 64 bytes)
 * 
 * For low-power nodes with deep sleep.
 * More efficient than JSON for transmission.
 */
typedef struct __attribute__((packed)) {
    uint16_t magic;             ///< 0xAD02
    uint8_t version;            ///< Protocol version
    uint8_t mac[6];             ///< MAC address
    uint32_t timestamp;         ///< Unix timestamp
    
    // Sensor data (12 bytes)
    int16_t distance_mm;        ///< -32k to +32k
    uint16_t vcc_mv;            ///< 0-65535mV
    int8_t rssi;                ///< -128 to +127
    uint8_t flags;              ///< Bit flags (error, low_bat, etc)
    
    // Health metrics (16 bytes)
    uint32_t uptime_s;
    uint32_t free_heap;
    int8_t cpu_temp;
    uint8_t reboot_reason;
    uint16_t tx_ok;
    uint16_t tx_fail;
    uint16_t sensor_errors;
    
    // Integrity (2 bytes)
    uint16_t crc16;             ///< CRC-16/CCITT
} aguada2_binary_t;

// ============================================================================
// FLAGS (for binary format)
// ============================================================================

#define AGUADA_FLAG_LOW_BATTERY     BIT0
#define AGUADA_FLAG_SENSOR_ERROR    BIT1
#define AGUADA_FLAG_COMM_ERROR      BIT2
#define AGUADA_FLAG_OTA_PENDING     BIT3
#define AGUADA_FLAG_CONFIG_MODE     BIT4

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

/**
 * @brief Build AGUADA-1 JSON packet
 * 
 * @param[out] json_str Output JSON string buffer
 * @param[in] json_size Size of json_str buffer
 * @param[in] mac MAC address string
 * @param[in] distance_mm Distance in millimeters
 * @param[in] vcc_mv Battery voltage in millivolts
 * @param[in] rssi RSSI in dBm
 * @return ESP_OK on success
 */
esp_err_t aguada_build_json_v1(
    char *json_str,
    size_t json_size,
    const char *mac,
    int32_t distance_mm,
    int32_t vcc_mv,
    int32_t rssi
);

/**
 * @brief Build AGUADA-2 JSON packet (with health)
 * 
 * @param[out] json_str Output JSON string buffer
 * @param[in] json_size Size of json_str buffer
 * @param[in] packet AGUADA-2 packet structure
 * @return ESP_OK on success
 */
esp_err_t aguada_build_json_v2(
    char *json_str,
    size_t json_size,
    const aguada2_packet_t *packet
);

/**
 * @brief Build AGUADA-2 binary packet
 * 
 * @param[out] binary Output binary packet
 * @param[in] packet AGUADA-2 packet structure
 * @return ESP_OK on success
 */
esp_err_t aguada_build_binary_v2(
    aguada2_binary_t *binary,
    const aguada2_packet_t *packet
);

/**
 * @brief Parse AGUADA-1 JSON packet
 * 
 * @param[in] json_str Input JSON string
 * @param[out] packet Parsed packet structure
 * @return ESP_OK on success
 */
esp_err_t aguada_parse_json_v1(
    const char *json_str,
    aguada1_packet_t *packet
);

/**
 * @brief Parse AGUADA-2 JSON packet
 * 
 * @param[in] json_str Input JSON string
 * @param[out] packet Parsed packet structure
 * @return ESP_OK on success
 */
esp_err_t aguada_parse_json_v2(
    const char *json_str,
    aguada2_packet_t *packet
);

/**
 * @brief Calculate CRC-16/CCITT
 * 
 * @param[in] data Data buffer
 * @param[in] length Length of data
 * @return CRC-16 value
 */
uint16_t aguada_crc16(const uint8_t *data, size_t length);

/**
 * @brief Verify AGUADA-2 binary packet integrity
 * 
 * @param[in] binary Binary packet
 * @return true if CRC is valid
 */
bool aguada_verify_binary(const aguada2_binary_t *binary);

/**
 * @brief Get reset reason as string
 * 
 * @param[in] reason Reset reason code
 * @return Human-readable string
 */
const char* aguada_get_reset_reason_str(esp_reset_reason_t reason);

/**
 * @brief Convert MAC bytes to string
 * 
 * @param[in] mac MAC address bytes (6 bytes)
 * @param[out] mac_str Output string buffer (18 bytes min)
 */
void aguada_mac_to_str(const uint8_t *mac, char *mac_str);

/**
 * @brief Convert MAC string to bytes
 * 
 * @param[in] mac_str MAC address string "XX:XX:XX:XX:XX:XX"
 * @param[out] mac Output MAC bytes (6 bytes)
 * @return ESP_OK on success
 */
esp_err_t aguada_str_to_mac(const char *mac_str, uint8_t *mac);

#ifdef __cplusplus
}
#endif

#endif // AGUADA_PROTOCOL_H
