#ifndef PACKET_H
#define PACKET_H

#include <Arduino.h>
#include <ArduinoJson.h>

class TelemetryPacket {
private:
  String nodeMac;
  float batteryVoltage;
  int rssi;
  uint32_t uptime;
  
public:
  TelemetryPacket();
  
  void setNodeMac(const String& mac);
  void setBattery(float voltage);
  void setRSSI(int rssi);
  void updateUptime();
  
  String buildJSON(float nivelCm, float volumeM3, float percentual);
  String getNodeMac() { return nodeMac; }
};

#endif // PACKET_H
