#ifndef NODE_REGISTRY_H
#define NODE_REGISTRY_H

#include <Arduino.h>
#include <map>

struct NodeInfo {
  String nodeMac;
  String nodeName;
  String sensorId;
  String elementoId;
  uint32_t lastSeen;
  uint32_t messageCount;
  int lastRSSI;
  bool online;
};

class NodeRegistry {
private:
  std::map<String, NodeInfo> nodes;
  
public:
  NodeRegistry();
  
  void registerNode(const String& mac, const String& name, const String& sensorId, const String& elementoId);
  void updateLastSeen(const String& mac, int rssi);
  bool isNodeOnline(const String& mac);
  NodeInfo* getNode(const String& mac);
  
  void checkOfflineNodes();
  void printRegistry();
  
  size_t getOnlineCount();
  size_t getTotalCount() { return nodes.size(); }
};

#endif // NODE_REGISTRY_H
