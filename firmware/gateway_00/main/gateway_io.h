#ifndef GATEWAY_IO_H
#define GATEWAY_IO_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include "queue_manager.h"

class GatewayIO {
private:
  PubSubClient* mqttClient;
  String mqttBroker;
  uint16_t mqttPort;
  String mqttUser;
  String mqttPass;
  String mqttTopicBase;
  String mqttTopicStatus;
  
  String httpServer;
  String httpEndpoint;
  
  bool mqttConnected;
  uint32_t lastMqttAttempt;
  uint32_t lastStatusSent;
  
  QueueManager* queueManager;
  
public:
  GatewayIO(const String& broker, uint16_t port, const String& user, const String& pass, 
            const String& topicBase, const String& topicStatus,
            const String& httpSrv, const String& httpEp);
  
  void begin(Client* client, QueueManager* queue);
  bool connectMQTT();
  void maintain();
  
  bool sendTelemetry(const String& payload);
  bool sendViaHTTP(const String& payload);
  bool sendStatus(const String& statusJson);
  
  void processQueue();
  
  bool isMQTTConnected() { return mqttConnected; }
};

#endif // GATEWAY_IO_H
