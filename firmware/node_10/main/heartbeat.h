#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include <Arduino.h>

class Heartbeat {
private:
  uint8_t ledPin;
  uint32_t lastBlink;
  bool ledState;
  
public:
  Heartbeat(uint8_t pin);
  
  void begin();
  void update();
  void blink(uint16_t count = 1, uint16_t delayMs = 100);
};

#endif // HEARTBEAT_H
