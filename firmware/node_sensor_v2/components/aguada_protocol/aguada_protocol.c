/**
 * @file aguada_protocol.c
 * @brief AGUADA Protocol v2.0 - Implementation
 */

#include "aguada_protocol.h"
#include <string.h>
#include <stdio.h>
#include "cJSON.h"
#include "esp_log.h"

static const char *TAG = "AGUADA_PROTOCOL";

esp_err_t aguada_build_json_v1(const aguada_packet_v1_t *packet,
                               char *json_buf, size_t buf_size)
{
    if (!packet || !json_buf)
    {
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *root = cJSON_CreateObject();
    if (!root)
    {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddStringToObject(root, "mac", packet->mac);
    cJSON_AddStringToObject(root, "type", packet->type);
    cJSON_AddNumberToObject(root, "value", packet->value);
    cJSON_AddNumberToObject(root, "battery", packet->battery);
    cJSON_AddNumberToObject(root, "uptime", packet->uptime);
    cJSON_AddNumberToObject(root, "rssi", packet->rssi);

    char *json_str = cJSON_PrintUnformatted(root);
    if (!json_str)
    {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }

    if (strlen(json_str) >= buf_size)
    {
        free(json_str);
        cJSON_Delete(root);
        return ESP_ERR_INVALID_SIZE;
    }

    strcpy(json_buf, json_str);
    free(json_str);
    cJSON_Delete(root);

    return ESP_OK;
}

esp_err_t aguada_build_json_v2(const aguada_packet_v2_t *packet,
                               char *json_buf, size_t buf_size)
{
    if (!packet || !json_buf)
    {
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *root = cJSON_CreateObject();
    if (!root)
    {
        return ESP_ERR_NO_MEM;
    }

    // Base fields (v1 compatibility)
    cJSON_AddStringToObject(root, "mac", packet->base.mac);
    cJSON_AddStringToObject(root, "type", packet->base.type);
    cJSON_AddNumberToObject(root, "value", packet->base.value);
    cJSON_AddNumberToObject(root, "battery", packet->base.battery);
    cJSON_AddNumberToObject(root, "uptime", packet->base.uptime);
    cJSON_AddNumberToObject(root, "rssi", packet->base.rssi);

    // Health metrics (v2 extension)
    cJSON *health = cJSON_CreateObject();
    cJSON_AddNumberToObject(health, "free_heap", packet->health.free_heap);
    cJSON_AddNumberToObject(health, "temperature", packet->health.temperature);
    cJSON_AddNumberToObject(health, "reboot_reason", packet->health.reboot_reason);
    cJSON_AddNumberToObject(health, "packets_sent", packet->health.packets_sent);
    cJSON_AddNumberToObject(health, "packets_failed", packet->health.packets_failed);
    cJSON_AddItemToObject(root, "health", health);

    char *json_str = cJSON_PrintUnformatted(root);
    if (!json_str)
    {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }

    if (strlen(json_str) >= buf_size)
    {
        free(json_str);
        cJSON_Delete(root);
        return ESP_ERR_INVALID_SIZE;
    }

    strcpy(json_buf, json_str);
    free(json_str);
    cJSON_Delete(root);

    return ESP_OK;
}

esp_err_t aguada_parse_json_v1(const char *json_str, aguada_packet_v1_t *packet)
{
    if (!json_str || !packet)
    {
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *root = cJSON_Parse(json_str);
    if (!root)
    {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *item;

    item = cJSON_GetObjectItem(root, "mac");
    if (item && cJSON_IsString(item))
    {
        strncpy(packet->mac, item->valuestring, sizeof(packet->mac) - 1);
    }

    item = cJSON_GetObjectItem(root, "type");
    if (item && cJSON_IsString(item))
    {
        strncpy(packet->type, item->valuestring, sizeof(packet->type) - 1);
    }

    item = cJSON_GetObjectItem(root, "value");
    if (item && cJSON_IsNumber(item))
    {
        packet->value = item->valueint;
    }

    item = cJSON_GetObjectItem(root, "battery");
    if (item && cJSON_IsNumber(item))
    {
        packet->battery = item->valueint;
    }

    item = cJSON_GetObjectItem(root, "uptime");
    if (item && cJSON_IsNumber(item))
    {
        packet->uptime = item->valueint;
    }

    item = cJSON_GetObjectItem(root, "rssi");
    if (item && cJSON_IsNumber(item))
    {
        packet->rssi = item->valueint;
    }

    cJSON_Delete(root);
    return ESP_OK;
}

esp_err_t aguada_build_binary_v2(const aguada_packet_v2_t *packet,
                                 aguada_binary_v2_t *binary)
{
    if (!packet || !binary)
    {
        return ESP_ERR_INVALID_ARG;
    }

    memset(binary, 0, sizeof(aguada_binary_v2_t));

    binary->magic = 0xAA;
    binary->version = AGUADA_PROTOCOL_VERSION;
    binary->type = AGUADA_PKT_TELEMETRY;

    // Convert type string to enum
    if (strcmp(packet->base.type, "distance_cm") == 0)
    {
        binary->variable = AGUADA_VAR_DISTANCE_CM;
    }
    else if (strcmp(packet->base.type, "valve_in") == 0)
    {
        binary->variable = AGUADA_VAR_VALVE_IN;
    }
    else if (strcmp(packet->base.type, "valve_out") == 0)
    {
        binary->variable = AGUADA_VAR_VALVE_OUT;
    }
    else if (strcmp(packet->base.type, "sound_in") == 0)
    {
        binary->variable = AGUADA_VAR_SOUND_IN;
    }

    // Convert MAC string to binary
    aguada_string_to_mac(packet->base.mac, binary->mac);

    binary->value = packet->base.value;
    binary->battery = packet->base.battery;
    binary->uptime = packet->base.uptime;
    binary->rssi = packet->base.rssi;
    binary->free_heap = packet->health.free_heap;
    binary->temperature = packet->health.temperature;

    // Calculate CRC (exclude CRC field itself)
    binary->crc16 = aguada_crc16((uint8_t *)binary,
                                 sizeof(aguada_binary_v2_t) - sizeof(uint16_t));

    return ESP_OK;
}

bool aguada_verify_binary(const aguada_binary_v2_t *binary)
{
    if (!binary || binary->magic != 0xAA)
    {
        return false;
    }

    uint16_t calculated_crc = aguada_crc16((uint8_t *)binary,
                                           sizeof(aguada_binary_v2_t) - sizeof(uint16_t));
    return (calculated_crc == binary->crc16);
}

uint16_t aguada_crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < length; i++)
    {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
            {
                crc = (crc >> 1) ^ 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}

void aguada_mac_to_string(const uint8_t mac_bin[6], char *mac_str)
{
    sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac_bin[0], mac_bin[1], mac_bin[2],
            mac_bin[3], mac_bin[4], mac_bin[5]);
}

void aguada_string_to_mac(const char *mac_str, uint8_t mac_bin[6])
{
    sscanf(mac_str, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
           &mac_bin[0], &mac_bin[1], &mac_bin[2],
           &mac_bin[3], &mac_bin[4], &mac_bin[5]);
}
