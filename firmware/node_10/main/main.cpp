/**
 * AGUADA - Sistema de Monitoramento Hidráulico
 * Firmware para ESP32-C3 SuperMini
 * 
 * Node: node_10
 * Sensor: SEN_CON_01 (AJ-SR04M)
 * Elemento: res_cons (Reservatório de Consumo)
 * 
 * Funcionalidades:
 * - Leitura de nível com sensor ultrassônico
 * - Filtro de mediana (11 amostras)
 * - Cálculo de volume e percentual
 * - Envio via MQTT (QoS 1) com fallback HTTP
 * - Watchdog timer
 * - Detecção de falhas do sensor
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_task_wdt.h>

#include "config_pins.h"
#include "ultra.h"
#include "packet.h"
#include "wifi.h"
#include "ios.h"
#include "heartbeat.h"

// =============================================================================
// OBJETOS GLOBAIS
// =============================================================================

UltrasonicSensor ultraSensor(ULTRA_TRIG_PIN, ULTRA_ECHO_PIN, 
                              ULTRA_MIN_DISTANCE_CM, ULTRA_MAX_DISTANCE_CM, 
                              ULTRA_TIMEOUT_US);

WiFiManager wifiManager(WIFI_SSID, WIFI_PASSWORD);

WiFiClient espClient;
IOService ioService(MQTT_BROKER, MQTT_PORT, MQTT_USER, MQTT_PASS, 
                    MQTT_TOPIC_BASE, HTTP_SERVER, HTTP_ENDPOINT);

TelemetryPacket telemetryPacket;
Heartbeat heartbeat(LED_BUILTIN);

// =============================================================================
// VARIÁVEIS GLOBAIS
// =============================================================================

float readingsBuffer[MEDIAN_SAMPLES];
uint8_t readingsIndex = 0;
bool bufferFilled = false;

uint32_t lastReadingTime = 0;
uint32_t lastTelemetryTime = 0;

// =============================================================================
// FUNÇÕES AUXILIARES
// =============================================================================

/**
 * Calcula mediana de um array
 */
float calculateMedian(float* values, uint8_t size) {
  // Copiar array para ordenação
  float sorted[size];
  memcpy(sorted, values, size * sizeof(float));
  
  // Bubble sort
  for (uint8_t i = 0; i < size - 1; i++) {
    for (uint8_t j = 0; j < size - i - 1; j++) {
      if (sorted[j] > sorted[j + 1]) {
        float temp = sorted[j];
        sorted[j] = sorted[j + 1];
        sorted[j + 1] = temp;
      }
    }
  }
  
  // Retornar mediana
  if (size % 2 == 0) {
    return (sorted[size / 2 - 1] + sorted[size / 2]) / 2.0f;
  } else {
    return sorted[size / 2];
  }
}

/**
 * Calcula volume do reservatório cilíndrico
 */
float calculateVolume(float nivelCm) {
  // Aplicar offset (distância do sensor ao topo)
  float nivelReal = nivelCm - SENSOR_OFFSET_CM;
  
  if (nivelReal < 0) {
    return 0.0f;
  }
  
  // Volume cilíndrico: V = π * r² * h
  float raio = (RESERVOIR_DIAMETER_CM / 2.0f) / 100.0f; // metros
  float altura = nivelReal / 100.0f; // metros
  
  float volumeM3 = PI * raio * raio * altura;
  
  return volumeM3;
}

/**
 * Calcula percentual de ocupação
 */
float calculatePercentual(float nivelCm) {
  float nivelReal = nivelCm - SENSOR_OFFSET_CM;
  float alturaMaxima = RESERVOIR_HEIGHT_CM - SENSOR_OFFSET_CM;
  
  if (alturaMaxima <= 0) {
    return 0.0f;
  }
  
  float percentual = (nivelReal / alturaMaxima) * 100.0f;
  
  return constrain(percentual, 0.0f, 100.0f);
}

/**
 * Lê sensor e adiciona ao buffer
 */
void readSensor() {
  float distance = ultraSensor.readDistanceCm();
  
  if (distance > 0) {
    // Adicionar ao buffer circular
    readingsBuffer[readingsIndex] = distance;
    readingsIndex = (readingsIndex + 1) % MEDIAN_SAMPLES;
    
    if (readingsIndex == 0) {
      bufferFilled = true;
    }
    
    Serial.printf("[SENSOR] Leitura: %.2f cm (buffer: %d/%d)\n", 
                  distance, bufferFilled ? MEDIAN_SAMPLES : readingsIndex, MEDIAN_SAMPLES);
  } else {
    Serial.println("[SENSOR] Leitura inválida");
  }
}

/**
 * Processa e envia telemetria
 */
void sendTelemetry() {
  if (!bufferFilled && readingsIndex < 3) {
    Serial.println("[TELEMETRY] Buffer insuficiente. Aguardando mais leituras...");
    return;
  }
  
  // Calcular mediana
  uint8_t sampleCount = bufferFilled ? MEDIAN_SAMPLES : readingsIndex;
  float median = calculateMedian(readingsBuffer, sampleCount);
  
  Serial.printf("[TELEMETRY] Mediana: %.2f cm (%d amostras)\n", median, sampleCount);
  
  // Calcular volume e percentual
  float volumeM3 = calculateVolume(median);
  float percentual = calculatePercentual(median);
  
  Serial.printf("[TELEMETRY] Volume: %.3f m³ (%.1f%%)\n", volumeM3, percentual);
  
  // Atualizar packet
  telemetryPacket.setBattery(3.3f); // ESP32-C3 não tem ADC de bateria nativo
  telemetryPacket.setRSSI(wifiManager.getRSSI());
  telemetryPacket.updateUptime();
  
  // Construir JSON
  String json = telemetryPacket.buildJSON(median, volumeM3, percentual);
  
  Serial.println("[TELEMETRY] JSON:");
  Serial.println(json);
  
  // Enviar
  if (ioService.sendTelemetry(json)) {
    Serial.println("[TELEMETRY] ✓ Enviado com sucesso");
    heartbeat.blink(2, 100); // Piscar 2x
  } else {
    Serial.println("[TELEMETRY] ✗ Falha ao enviar");
    heartbeat.blink(5, 50); // Piscar 5x rápido (erro)
  }
}

/**
 * Detecta falha do sensor (stuck ou timeout)
 */
void checkSensorHealth() {
  uint32_t timeSinceLastReading = ultraSensor.getTimeSinceLastReading();
  uint16_t errorCount = ultraSensor.getErrorCount();
  
  // Sensor stuck (sem leitura por 60 segundos)
  if (timeSinceLastReading > 60000) {
    Serial.println("[FAULT] ⚠️ Sensor STUCK (sem leitura válida por >60s)");
    // TODO: Enviar evento de falha
  }
  
  // Muitos erros consecutivos
  if (errorCount > 10) {
    Serial.println("[FAULT] ⚠️ Sensor com muitos erros consecutivos");
    // TODO: Enviar evento de falha
  }
}

// =============================================================================
// SETUP
// =============================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n");
  Serial.println("================================");
  Serial.println("   AGUADA - Node Telemetria");
  Serial.println("================================");
  Serial.printf("Node: %s\n", NODE_NAME);
  Serial.printf("Sensor: %s\n", SENSOR_ID);
  Serial.printf("Elemento: %s\n", ELEMENTO_ID);
  Serial.println("================================\n");
  
  // Inicializar heartbeat
  heartbeat.begin();
  heartbeat.blink(3, 200);
  
  // Inicializar sensor
  ultraSensor.begin();
  
  // Conectar WiFi
  if (!wifiManager.connect()) {
    Serial.println("[SETUP] Falha ao conectar WiFi. Reiniciando...");
    delay(5000);
    ESP.restart();
  }
  
  // Configurar MAC address
  telemetryPacket.setNodeMac(wifiManager.getMacAddress());
  
  // Inicializar I/O Service
  ioService.begin(&espClient);
  ioService.connectMQTT();
  
  // Configurar watchdog (120 segundos)
  esp_task_wdt_init(WATCHDOG_TIMEOUT_SEC, true);
  esp_task_wdt_add(NULL);
  
  Serial.println("\n[SETUP] ✓ Inicialização completa\n");
  
  // Primeira leitura
  readSensor();
  lastReadingTime = millis();
  lastTelemetryTime = millis();
}

// =============================================================================
// LOOP
// =============================================================================

void loop() {
  uint32_t now = millis();
  
  // Reset watchdog
  esp_task_wdt_reset();
  
  // Atualizar heartbeat
  heartbeat.update();
  
  // Manter WiFi
  wifiManager.maintain();
  
  // Manter MQTT
  ioService.maintain();
  
  // Leitura periódica (10 segundos)
  if (now - lastReadingTime >= READING_INTERVAL_MS) {
    readSensor();
    checkSensorHealth();
    lastReadingTime = now;
  }
  
  // Telemetria periódica (30 segundos)
  if (now - lastTelemetryTime >= TELEMETRY_INTERVAL_MS) {
    if (wifiManager.isConnected()) {
      sendTelemetry();
    } else {
      Serial.println("[TELEMETRY] WiFi desconectado. Pulando envio.");
    }
    lastTelemetryTime = now;
  }
  
  delay(100);
}
