#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

class WiFiManager {
private:
  String ssid;
  String password;
  uint8_t reconnectAttempts;
  uint32_t lastReconnectAttempt;
  
public:
  WiFiManager(const String& ssid, const String& password);
  
  bool connect();
  bool isConnected();
  void maintain();
  
  String getMacAddress();
  int getRSSI();
};

#endif // WIFI_MANAGER_H
