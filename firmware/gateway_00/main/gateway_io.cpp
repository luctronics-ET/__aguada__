#include "gateway_io.h"
#include "config_pins.h"

GatewayIO::GatewayIO(const String& broker, uint16_t port, const String& user, 
                     const String& pass, const String& topicBase, const String& topicStatus,
                     const String& httpSrv, const String& httpEp)
  : mqttBroker(broker), mqttPort(port), mqttUser(user), mqttPass(pass),
    mqttTopicBase(topicBase), mqttTopicStatus(topicStatus),
    httpServer(httpSrv), httpEndpoint(httpEp),
    mqttConnected(false), lastMqttAttempt(0), lastStatusSent(0),
    mqttClient(nullptr), queueManager(nullptr) {}

void GatewayIO::begin(Client* client, QueueManager* queue) {
  mqttClient = new PubSubClient(*client);
  mqttClient->setServer(mqttBroker.c_str(), mqttPort);
  mqttClient->setBufferSize(1024);
  queueManager = queue;
  
  Serial.println("[GATEWAY_IO] Serviço de I/O inicializado");
}

bool GatewayIO::connectMQTT() {
  if (!mqttClient) return false;
  
  Serial.println("[MQTT] Conectando gateway...");
  
  String clientId = "aguada_gateway_" + String(random(0xffff), HEX);
  
  if (mqttClient->connect(clientId.c_str(), mqttUser.c_str(), mqttPass.c_str())) {
    Serial.println("[MQTT] ✓ Gateway conectado");
    mqttConnected = true;
    
    // Enviar status inicial
    sendStatus("{\"status\":\"online\",\"gateway\":\"gateway_00\"}");
    
    return true;
  } else {
    Serial.printf("[MQTT] ✗ Falha (state: %d)\n", mqttClient->state());
    mqttConnected = false;
    return false;
  }
}

void GatewayIO::maintain() {
  if (mqttClient && mqttConnected) {
    mqttClient->loop();
    
    if (!mqttClient->connected()) {
      mqttConnected = false;
      Serial.println("[MQTT] Conexão perdida");
    }
  } else {
    // Tentar reconectar a cada 10 segundos
    uint32_t now = millis();
    if (now - lastMqttAttempt > 10000) {
      lastMqttAttempt = now;
      connectMQTT();
    }
  }
  
  // Enviar status periódico (a cada 60 segundos)
  uint32_t now = millis();
  if (now - lastStatusSent > 60000) {
    lastStatusSent = now;
    
    String status = "{";
    status += "\"gateway\":\"gateway_00\",";
    status += "\"status\":\"online\",";
    status += "\"uptime\":" + String(millis() / 1000) + ",";
    status += "\"queue_size\":" + String(queueManager ? queueManager->size() : 0) + ",";
    status += "\"msgs_sent\":" + String(queueManager ? queueManager->getMessagesSent() : 0) + ",";
    status += "\"msgs_dropped\":" + String(queueManager ? queueManager->getMessagesDropped() : 0);
    status += "}";
    
    sendStatus(status);
  }
}

bool GatewayIO::sendTelemetry(const String& payload) {
  // Tentar MQTT primeiro
  if (mqttConnected && mqttClient && mqttClient->connected()) {
    String topic = mqttTopicBase + "/gateway";
    
    if (mqttClient->publish(topic.c_str(), payload.c_str())) {
      Serial.println("[MQTT] ✓ Telemetria enviada");
      if (queueManager) queueManager->incrementSent();
      return true;
    } else {
      Serial.println("[MQTT] ✗ Falha ao publicar");
    }
  }
  
  // Fallback para HTTP
  Serial.println("[HTTP] Tentando fallback...");
  return sendViaHTTP(payload);
}

bool GatewayIO::sendViaHTTP(const String& payload) {
  HTTPClient http;
  
  String url = httpServer + httpEndpoint;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000);
  
  int httpCode = http.POST(payload);
  
  if (httpCode == 200 || httpCode == 201) {
    Serial.printf("[HTTP] ✓ Enviado (code: %d)\n", httpCode);
    if (queueManager) queueManager->incrementSent();
    http.end();
    return true;
  } else {
    Serial.printf("[HTTP] ✗ Erro (code: %d)\n", httpCode);
    http.end();
    return false;
  }
}

bool GatewayIO::sendStatus(const String& statusJson) {
  if (mqttConnected && mqttClient && mqttClient->connected()) {
    if (mqttClient->publish(mqttTopicStatus.c_str(), statusJson.c_str())) {
      Serial.println("[MQTT] ✓ Status enviado");
      return true;
    }
  }
  return false;
}

void GatewayIO::processQueue() {
  if (!queueManager || queueManager->isEmpty()) {
    return;
  }
  
  TelemetryMessage msg;
  if (queueManager->dequeue(msg)) {
    Serial.printf("[GATEWAY_IO] Processando mensagem de %s (tentativa %d)\n", 
                  msg.nodeMac.c_str(), msg.retryCount + 1);
    
    if (!sendTelemetry(msg.payload)) {
      // Falhou - reenfileirar com retry
      queueManager->incrementRetry(msg);
    }
  }
}
