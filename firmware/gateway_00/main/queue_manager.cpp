#include "queue_manager.h"
#include "config_pins.h"

QueueManager::QueueManager(size_t size) 
  : maxSize(size), messagesReceived(0), messagesSent(0), messagesDropped(0) {
  queue.reserve(size);
}

bool QueueManager::enqueue(const String& nodeMac, const String& payload, bool priority) {
  if (isFull()) {
    Serial.println("[QUEUE] Fila cheia! Descartando mensagem mais antiga.");
    queue.erase(queue.begin());
    messagesDropped++;
  }
  
  TelemetryMessage msg;
  msg.nodeMac = nodeMac;
  msg.payload = payload;
  msg.timestamp = millis();
  msg.retryCount = 0;
  msg.priority = priority;
  
  if (priority) {
    // Inserir no início para mensagens prioritárias
    queue.insert(queue.begin(), msg);
  } else {
    queue.push_back(msg);
  }
  
  messagesReceived++;
  Serial.printf("[QUEUE] Mensagem enfileirada: %s (total: %d)\n", nodeMac.c_str(), queue.size());
  
  return true;
}

bool QueueManager::dequeue(TelemetryMessage& msg) {
  if (isEmpty()) {
    return false;
  }
  
  msg = queue.front();
  queue.erase(queue.begin());
  
  return true;
}

bool QueueManager::isEmpty() {
  return queue.empty();
}

bool QueueManager::isFull() {
  return queue.size() >= maxSize;
}

size_t QueueManager::size() {
  return queue.size();
}

void QueueManager::incrementRetry(TelemetryMessage& msg) {
  msg.retryCount++;
  msg.timestamp = millis();
  
  // Recolocar na fila se ainda tem tentativas
  if (msg.retryCount < RETRY_ATTEMPTS) {
    queue.push_back(msg);
    Serial.printf("[QUEUE] Reenfileirando (tentativa %d/%d)\n", msg.retryCount, RETRY_ATTEMPTS);
  } else {
    Serial.printf("[QUEUE] ✗ Descartando após %d tentativas\n", RETRY_ATTEMPTS);
    messagesDropped++;
  }
}

void QueueManager::clear() {
  queue.clear();
  Serial.println("[QUEUE] Fila limpa");
}
