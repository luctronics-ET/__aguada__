#include "node_registry.h"
#include "config_pins.h"

NodeRegistry::NodeRegistry() {}

void NodeRegistry::registerNode(const String& mac, const String& name, const String& sensorId, const String& elementoId) {
  NodeInfo info;
  info.nodeMac = mac;
  info.nodeName = name;
  info.sensorId = sensorId;
  info.elementoId = elementoId;
  info.lastSeen = millis();
  info.messageCount = 0;
  info.lastRSSI = 0;
  info.online = true;
  
  nodes[mac] = info;
  
  Serial.printf("[REGISTRY] Node registrado: %s (%s)\n", name.c_str(), mac.c_str());
}

void NodeRegistry::updateLastSeen(const String& mac, int rssi) {
  auto it = nodes.find(mac);
  if (it != nodes.end()) {
    it->second.lastSeen = millis();
    it->second.lastRSSI = rssi;
    it->second.messageCount++;
    it->second.online = true;
  } else {
    // Auto-registrar node desconhecido
    Serial.printf("[REGISTRY] Node desconhecido: %s. Auto-registrando...\n", mac.c_str());
    registerNode(mac, "unknown", "unknown", "unknown");
  }
}

bool NodeRegistry::isNodeOnline(const String& mac) {
  auto it = nodes.find(mac);
  if (it != nodes.end()) {
    uint32_t elapsed = (millis() - it->second.lastSeen) / 1000;
    return elapsed < NODE_TIMEOUT_SEC;
  }
  return false;
}

NodeInfo* NodeRegistry::getNode(const String& mac) {
  auto it = nodes.find(mac);
  if (it != nodes.end()) {
    return &it->second;
  }
  return nullptr;
}

void NodeRegistry::checkOfflineNodes() {
  for (auto& pair : nodes) {
    uint32_t elapsed = (millis() - pair.second.lastSeen) / 1000;
    
    if (elapsed > NODE_TIMEOUT_SEC && pair.second.online) {
      pair.second.online = false;
      Serial.printf("[REGISTRY] ⚠️ Node OFFLINE: %s (última msg há %d s)\n", 
                    pair.second.nodeName.c_str(), elapsed);
    }
  }
}

void NodeRegistry::printRegistry() {
  Serial.println("\n[REGISTRY] ==================== NODES REGISTRADOS ====================");
  Serial.printf("Total: %d | Online: %d\n", nodes.size(), getOnlineCount());
  Serial.println("MAC Address       | Name     | Sensor      | Msgs  | RSSI | Status");
  Serial.println("----------------------------------------------------------------");
  
  for (const auto& pair : nodes) {
    const NodeInfo& info = pair.second;
    uint32_t elapsed = (millis() - info.lastSeen) / 1000;
    
    Serial.printf("%-17s | %-8s | %-11s | %-5d | %-4d | %s (%ds)\n",
                  info.nodeMac.c_str(),
                  info.nodeName.c_str(),
                  info.sensorId.c_str(),
                  info.messageCount,
                  info.lastRSSI,
                  info.online ? "ONLINE" : "OFFLINE",
                  elapsed);
  }
  Serial.println("================================================================\n");
}

size_t NodeRegistry::getOnlineCount() {
  size_t count = 0;
  for (const auto& pair : nodes) {
    if (pair.second.online) {
      count++;
    }
  }
  return count;
}
