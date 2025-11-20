#include "packet.h"
#include "esp_timer.h"
#include <stdio.h>

TelemetryPacket::TelemetryPacket() 
    : batteryVoltage(0), rssi(0), uptime(0) {
    nodeMac[0] = '\0';
}

void TelemetryPacket::setNodeMac(const char* mac) {
    strncpy(nodeMac, mac, sizeof(nodeMac) - 1);
    nodeMac[sizeof(nodeMac) - 1] = '\0';
}

void TelemetryPacket::setBattery(float voltage) {
    batteryVoltage = voltage;
}

void TelemetryPacket::setRSSI(int rssiValue) {
    rssi = rssiValue;
}

void TelemetryPacket::updateUptime() {
    uptime = (uint32_t)(esp_timer_get_time() / 1000000ULL);
}

char* TelemetryPacket::buildJSON(float nivelCm, float volumeM3, float percentual) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "node_mac", nodeMac);
    cJSON_AddStringToObject(root, "datetime", "2025-11-16T00:00:00Z"); // TODO: RTC ou usa datetime da recepcao pelo server
    
    cJSON *data = cJSON_CreateArray();
    cJSON *nivel = cJSON_CreateObject();
    cJSON_AddStringToObject(nivel, "label", "nivel_cm");
    cJSON_AddNumberToObject(nivel, "value", nivelCm);
    cJSON_AddStringToObject(nivel, "unit", "cm");
    cJSON_AddItemToArray(data, nivel);
    
    cJSON_AddItemToObject(root, "data", data);
    
    cJSON *meta = cJSON_CreateObject();
    cJSON_AddNumberToObject(meta, "battery", batteryVoltage);
    cJSON_AddNumberToObject(meta, "rssi", rssi);
    cJSON_AddNumberToObject(meta, "uptime", uptime);
    cJSON_AddItemToObject(root, "meta", meta);
    
    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);
    
    return json_str;
}
