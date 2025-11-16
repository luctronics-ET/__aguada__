#include "heartbeat.h"

Heartbeat::Heartbeat(uint8_t pin) 
  : ledPin(pin), lastBlink(0), ledState(false) {}

void Heartbeat::begin() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  Serial.println("[HEARTBEAT] LED inicializado");
}

void Heartbeat::update() {
  uint32_t now = millis();
  
  // Piscar a cada 1 segundo
  if (now - lastBlink > 1000) {
    ledState = !ledState;
    digitalWrite(ledPin, ledState ? HIGH : LOW);
    lastBlink = now;
  }
}

void Heartbeat::blink(uint16_t count, uint16_t delayMs) {
  for (uint16_t i = 0; i < count; i++) {
    digitalWrite(ledPin, HIGH);
    delay(delayMs);
    digitalWrite(ledPin, LOW);
    delay(delayMs);
  }
}
