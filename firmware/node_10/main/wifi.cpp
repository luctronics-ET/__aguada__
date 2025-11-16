#include "wifi.h"
#include <WiFi.h>

WiFiManager::WiFiManager(const String& ssid, const String& password)
  : ssid(ssid), password(password), reconnectAttempts(0), lastReconnectAttempt(0) {}

bool WiFiManager::connect() {
  Serial.println("[WiFi] Conectando...");
  Serial.printf("[WiFi] SSID: %s\n", ssid.c_str());
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  
  uint8_t attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] ✓ Conectado!");
    Serial.printf("[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("[WiFi] MAC: %s\n", WiFi.macAddress().c_str());
    Serial.printf("[WiFi] RSSI: %d dBm\n", WiFi.RSSI());
    reconnectAttempts = 0;
    return true;
  } else {
    Serial.println("\n[WiFi] ✗ Falha ao conectar");
    return false;
  }
}

bool WiFiManager::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::maintain() {
  if (!isConnected()) {
    uint32_t now = millis();
    
    // Tentar reconectar a cada 30 segundos
    if (now - lastReconnectAttempt > 30000) {
      Serial.println("[WiFi] Conexão perdida. Tentando reconectar...");
      lastReconnectAttempt = now;
      reconnectAttempts++;
      
      if (reconnectAttempts > 10) {
        Serial.println("[WiFi] Muitas tentativas falhas. Reiniciando ESP...");
        ESP.restart();
      }
      
      connect();
    }
  }
}

String WiFiManager::getMacAddress() {
  return WiFi.macAddress();
}

int WiFiManager::getRSSI() {
  return WiFi.RSSI();
}
