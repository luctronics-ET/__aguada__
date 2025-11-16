#ifndef QUEUE_MANAGER_H
#define QUEUE_MANAGER_H

#include <Arduino.h>
#include <vector>

struct TelemetryMessage {
  String nodeMac;
  String payload;
  uint32_t timestamp;
  uint8_t retryCount;
  bool priority;
};

class QueueManager {
private:
  std::vector<TelemetryMessage> queue;
  size_t maxSize;
  uint32_t messagesReceived;
  uint32_t messagesSent;
  uint32_t messagesDropped;
  
public:
  QueueManager(size_t size = 50);
  
  bool enqueue(const String& nodeMac, const String& payload, bool priority = false);
  bool dequeue(TelemetryMessage& msg);
  bool isEmpty();
  bool isFull();
  size_t size();
  
  void incrementRetry(TelemetryMessage& msg);
  void clear();
  
  // Statistics
  uint32_t getMessagesReceived() { return messagesReceived; }
  uint32_t getMessagesSent() { return messagesSent; }
  uint32_t getMessagesDropped() { return messagesDropped; }
  void incrementSent() { messagesSent++; }
};

#endif // QUEUE_MANAGER_H
