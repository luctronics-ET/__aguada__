#ifndef ULTRA_H
#define ULTRA_H

#include <Arduino.h>

class UltrasonicSensor {
private:
  uint8_t trigPin;
  uint8_t echoPin;
  float minDistance;
  float maxDistance;
  uint32_t timeoutUs;
  
  float lastValidReading;
  uint32_t lastReadingTime;
  uint16_t errorCount;
  
public:
  UltrasonicSensor(uint8_t trig, uint8_t echo, float minDist, float maxDist, uint32_t timeout);
  
  void begin();
  float readDistanceCm();
  bool isValid(float distance);
  
  // Diagnostics
  uint16_t getErrorCount() { return errorCount; }
  uint32_t getTimeSinceLastReading() { return millis() - lastReadingTime; }
  float getLastValidReading() { return lastValidReading; }
};

#endif // ULTRA_H
