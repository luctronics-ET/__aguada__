#ifndef IOS_H
#define IOS_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <HTTPClient.h>

class IOService {
private:
  PubSubClient* mqttClient;
  String mqttBroker;
  uint16_t mqttPort;
  String mqttUser;
  String mqttPass;
  String mqttTopicBase;
  
  String httpServer;
  String httpEndpoint;
  
  bool mqttConnected;
  uint32_t lastMqttAttempt;
  
public:
  IOService(const String& broker, uint16_t port, const String& user, const String& pass, 
            const String& topic, const String& httpSrv, const String& httpEp);
  
  void begin(Client* client);
  bool connectMQTT();
  void maintain();
  
  bool sendTelemetry(const String& payload);
  bool sendViaHTTP(const String& payload);
  
  bool isMQTTConnected() { return mqttConnected; }
};

#endif // IOS_H
