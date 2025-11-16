#include "packet.h"
#include "config_pins.h"
#include <WiFi.h>

TelemetryPacket::TelemetryPacket() 
  : batteryVoltage(0.0f), rssi(0), uptime(0) {}

void TelemetryPacket::setNodeMac(const String& mac) {
  nodeMac = mac;
}

void TelemetryPacket::setBattery(float voltage) {
  batteryVoltage = voltage;
}

void TelemetryPacket::setRSSI(int rssiValue) {
  rssi = rssiValue;
}

void TelemetryPacket::updateUptime() {
  uptime = millis() / 1000;
}

String TelemetryPacket::buildJSON(float nivelCm, float volumeM3, float percentual) {
  // Criar documento JSON
  StaticJsonDocument<512> doc;
  
  // Node info
  doc["node_mac"] = nodeMac;
  doc["datetime"] = ""; // Será preenchido pelo backend baseado no timestamp de chegada
  
  // Data array
  JsonArray dataArray = doc.createNestedArray("data");
  
  JsonObject reading = dataArray.createNestedObject();
  reading["label"] = "nivel_cm";
  reading["value"] = nivelCm;
  reading["unit"] = "cm";
  
  // Meta information
  JsonObject meta = doc.createNestedObject("meta");
  meta["battery"] = batteryVoltage;
  meta["rssi"] = rssi;
  meta["uptime"] = uptime;
  meta["firmware_version"] = "1.0.0";
  meta["sensor_id"] = SENSOR_ID;
  meta["elemento_id"] = ELEMENTO_ID;
  meta["volume_m3"] = volumeM3;
  meta["percentual"] = percentual;
  
  // Serializar para string
  String jsonString;
  serializeJson(doc, jsonString);
  
  return jsonString;
}
