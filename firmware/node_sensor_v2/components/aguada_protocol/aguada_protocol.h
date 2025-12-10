/**
 * @file aguada_protocol.h
 * @brief AGUADA Protocol v2.0 - Data structures and serialization
 * 
 * Protocol evolution:
 * - v1: JSON only (legacy compatibility)
 * - v2: JSON + binary + health metrics
 */

#ifndef AGUADA_PROTOCOL_H
#define AGUADA_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Protocol version */
#define AGUADA_PROTOCOL_VERSION 2

/* Packet types */
typedef enum {
    AGUADA_PKT_TELEMETRY = 0x01,
    AGUADA_PKT_HEALTH = 0x02,
    AGUADA_PKT_CONFIG = 0x03,
    AGUADA_PKT_ACK = 0x04
} aguada_packet_type_t;

/* Variable types (telemetry) */
typedef enum {
    AGUADA_VAR_DISTANCE_CM = 0x10,
    AGUADA_VAR_VALVE_IN = 0x20,
    AGUADA_VAR_VALVE_OUT = 0x21,
    AGUADA_VAR_SOUND_IN = 0x30
} aguada_variable_t;

/**
 * @brief AGUADA v1 Packet (JSON format - legacy compatibility)
 * 
 * Example JSON:
 * {"mac":"20:6E:F1:6B:77:58","type":"distance_cm","value":24480,"battery":5000,"uptime":3,"rssi":-50}
 */
typedef struct {
    char mac[18];               // MAC address string "XX:XX:XX:XX:XX:XX"
    char type[32];              // Variable type: "distance_cm", "valve_in", etc.
    int32_t value;              // Measurement value (cm×100 for distance)
    uint16_t battery;           // Battery voltage in mV
    uint32_t uptime;            // Uptime in seconds
    int8_t rssi;                // Signal strength in dBm
} aguada_packet_v1_t;

/**
 * @brief Health metrics
 */
typedef struct {
    uint32_t uptime_sec;        // Uptime in seconds
    uint32_t free_heap;         // Free heap in bytes
    int8_t temperature;         // Internal temperature in °C
    uint8_t reboot_reason;      // Last reboot reason
    uint32_t packets_sent;      // Total packets sent
    uint32_t packets_failed;    // Failed transmissions
} aguada_health_t;

/**
 * @brief AGUADA v2 Packet (JSON with extended metrics)
 * 
 * Adds health monitoring for diagnostics
 */
typedef struct {
    aguada_packet_v1_t base;    // Base v1 packet (backwards compatible)
    aguada_health_t health;     // Additional health metrics
} aguada_packet_v2_t;

/**
 * @brief AGUADA v2 Binary format (optimized for bandwidth)
 * 
 * Total size: 32 bytes
 * Use when bandwidth is critical (future ESP-NOW optimization)
 */
typedef struct __attribute__((packed)) {
    uint8_t magic;              // 0xAA (packet identification)
    uint8_t version;            // Protocol version (2)
    uint8_t type;               // aguada_packet_type_t
    uint8_t variable;           // aguada_variable_t
    uint8_t mac[6];             // MAC address (binary)
    int32_t value;              // Measurement value
    uint16_t battery;           // Battery mV
    uint32_t uptime;            // Uptime seconds
    int8_t rssi;                // Signal strength dBm
    uint32_t free_heap;         // Free heap bytes
    int8_t temperature;         // Temperature °C
    uint8_t reserved[6];        // Reserved for future use
    uint16_t crc16;             // CRC16 checksum
} aguada_binary_v2_t;

/**
 * @brief Build JSON string from v1 packet
 * 
 * @param packet Packet data
 * @param json_buf Output buffer for JSON string
 * @param buf_size Buffer size
 * @return esp_err_t ESP_OK on success
 */
esp_err_t aguada_build_json_v1(const aguada_packet_v1_t *packet, 
                                char *json_buf, size_t buf_size);

/**
 * @brief Build JSON string from v2 packet (with health metrics)
 * 
 * @param packet Packet data
 * @param json_buf Output buffer for JSON string
 * @param buf_size Buffer size
 * @return esp_err_t ESP_OK on success
 */
esp_err_t aguada_build_json_v2(const aguada_packet_v2_t *packet, 
                                char *json_buf, size_t buf_size);

/**
 * @brief Parse JSON string to v1 packet
 * 
 * @param json_str JSON string
 * @param packet Output packet
 * @return esp_err_t ESP_OK on success
 */
esp_err_t aguada_parse_json_v1(const char *json_str, aguada_packet_v1_t *packet);

/**
 * @brief Build binary packet from v2 data
 * 
 * @param packet Packet data
 * @param binary Output binary packet
 * @return esp_err_t ESP_OK on success
 */
esp_err_t aguada_build_binary_v2(const aguada_packet_v2_t *packet, 
                                  aguada_binary_v2_t *binary);

/**
 * @brief Verify binary packet CRC
 * 
 * @param binary Binary packet
 * @return true if CRC is valid
 */
bool aguada_verify_binary(const aguada_binary_v2_t *binary);

/**
 * @brief Calculate CRC16 for data
 * 
 * @param data Data buffer
 * @param length Data length
 * @return uint16_t CRC16 value
 */
uint16_t aguada_crc16(const uint8_t *data, size_t length);

/**
 * @brief Convert MAC address from binary to string
 * 
 * @param mac_bin Binary MAC (6 bytes)
 * @param mac_str Output string buffer (min 18 bytes)
 */
void aguada_mac_to_string(const uint8_t mac_bin[6], char *mac_str);

/**
 * @brief Convert MAC address from string to binary
 * 
 * @param mac_str MAC string "XX:XX:XX:XX:XX:XX"
 * @param mac_bin Output binary buffer (6 bytes)
 */
void aguada_string_to_mac(const char *mac_str, uint8_t mac_bin[6]);

#ifdef __cplusplus
}
#endif

#endif // AGUADA_PROTOCOL_H
