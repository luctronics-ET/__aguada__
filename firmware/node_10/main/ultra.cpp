#include "ultra.h"
#include "config_pins.h"

UltrasonicSensor::UltrasonicSensor(uint8_t trig, uint8_t echo, float minDist, float maxDist, uint32_t timeout)
  : trigPin(trig), echoPin(echo), minDistance(minDist), maxDistance(maxDist), timeoutUs(timeout),
    lastValidReading(0.0f), lastReadingTime(0), errorCount(0) {}

void UltrasonicSensor::begin() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(trigPin, LOW);
  delay(50);
  Serial.println("[ULTRA] Sensor inicializado");
}

float UltrasonicSensor::readDistanceCm() {
  // Trigger pulse (10µs)
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Aguardar echo
  uint32_t duration = pulseIn(echoPin, HIGH, timeoutUs);
  
  if (duration == 0) {
    errorCount++;
    Serial.println("[ULTRA] Timeout - sem resposta");
    return -1.0f;
  }
  
  // Calcular distância: distance = (duration * sound_speed) / 2
  float distance = (duration * ULTRA_SOUND_SPEED_CM_US) / 2.0f;
  
  if (isValid(distance)) {
    lastValidReading = distance;
    lastReadingTime = millis();
    errorCount = 0;
    return distance;
  } else {
    errorCount++;
    Serial.printf("[ULTRA] Leitura inválida: %.2f cm (fora do range %.0f-%.0f)\n", 
                  distance, minDistance, maxDistance);
    return -1.0f;
  }
}

bool UltrasonicSensor::isValid(float distance) {
  return (distance >= minDistance && distance <= maxDistance);
}
