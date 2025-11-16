#include "ios.h"

IOService::IOService(const String& broker, uint16_t port, const String& user, 
                     const String& pass, const String& topic, 
                     const String& httpSrv, const String& httpEp)
  : mqttBroker(broker), mqttPort(port), mqttUser(user), mqttPass(pass),
    mqttTopicBase(topic), httpServer(httpSrv), httpEndpoint(httpEp),
    mqttConnected(false), lastMqttAttempt(0), mqttClient(nullptr) {}

void IOService::begin(Client* client) {
  mqttClient = new PubSubClient(*client);
  mqttClient->setServer(mqttBroker.c_str(), mqttPort);
  mqttClient->setBufferSize(512);
  
  Serial.println("[IO] Serviço de I/O inicializado");
}

bool IOService::connectMQTT() {
  if (!mqttClient) return false;
  
  Serial.println("[MQTT] Conectando...");
  
  String clientId = "aguada_node_" + String(random(0xffff), HEX);
  
  if (mqttClient->connect(clientId.c_str(), mqttUser.c_str(), mqttPass.c_str())) {
    Serial.println("[MQTT] ✓ Conectado");
    mqttConnected = true;
    return true;
  } else {
    Serial.printf("[MQTT] ✗ Falha (state: %d)\n", mqttClient->state());
    mqttConnected = false;
    return false;
  }
}

void IOService::maintain() {
  if (mqttClient && mqttConnected) {
    mqttClient->loop();
    
    if (!mqttClient->connected()) {
      mqttConnected = false;
      Serial.println("[MQTT] Conexão perdida");
    }
  } else {
    // Tentar reconectar a cada 30 segundos
    uint32_t now = millis();
    if (now - lastMqttAttempt > 30000) {
      lastMqttAttempt = now;
      connectMQTT();
    }
  }
}

bool IOService::sendTelemetry(const String& payload) {
  // Tentar MQTT primeiro
  if (mqttConnected && mqttClient && mqttClient->connected()) {
    String topic = mqttTopicBase + "/" + WiFi.macAddress();
    
    if (mqttClient->publish(topic.c_str(), payload.c_str())) {
      Serial.println("[MQTT] ✓ Telemetria enviada");
      return true;
    } else {
      Serial.println("[MQTT] ✗ Falha ao publicar");
    }
  }
  
  // Fallback para HTTP
  Serial.println("[HTTP] Tentando fallback...");
  return sendViaHTTP(payload);
}

bool IOService::sendViaHTTP(const String& payload) {
  HTTPClient http;
  
  String url = httpServer + httpEndpoint;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  int httpCode = http.POST(payload);
  
  if (httpCode == 200 || httpCode == 201) {
    Serial.printf("[HTTP] ✓ Enviado (code: %d)\n", httpCode);
    String response = http.getString();
    Serial.println(response);
    http.end();
    return true;
  } else {
    Serial.printf("[HTTP] ✗ Erro (code: %d)\n", httpCode);
    http.end();
    return false;
  }
}
