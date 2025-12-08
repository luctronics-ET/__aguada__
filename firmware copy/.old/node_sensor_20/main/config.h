#ifndef CONFIG_H
#define CONFIG_H

#include "driver/gpio.h"

// ============================================================================
// AGUADA NODE SENSOR 20 - DUAL ULTRASONIC (IE01 + IE02)
// ============================================================================
// Tipo: TYPE_DUAL_ULTRA
// Monitora 2 reservatórios (IE01 e IE02) com um único ESP32-C3
// Versão: v1.2.0-fixed-channel (Canal 11 fixo)
// ============================================================================

#define FIRMWARE_VERSION "v1.2.0-fixed-channel"

// ----------------------------------------------------------------------------
// SENSOR IE01 (Cisterna Ilha do Engenho 01)
// ----------------------------------------------------------------------------
#define IE01_TRIG_PIN       GPIO_NUM_0      // Ultrasonic trigger IE01
#define IE01_ECHO_PIN       GPIO_NUM_1      // Ultrasonic echo IE01
#define IE01_SOUND_PIN      GPIO_NUM_5      // Detector de som IE01
#define IE01_VALVE_IN_PIN   GPIO_NUM_7      // Válvula entrada IE01
#define IE01_VALVE_OUT_PIN  GPIO_NUM_8      // Válvula saída IE01

// ----------------------------------------------------------------------------
// SENSOR IE02 (Cisterna Ilha do Engenho 02)
// ----------------------------------------------------------------------------
#define IE02_TRIG_PIN       GPIO_NUM_2      // Ultrasonic trigger IE02
#define IE02_ECHO_PIN       GPIO_NUM_3      // Ultrasonic echo IE02
#define IE02_SOUND_PIN      GPIO_NUM_6      // Detector de som IE02
#define IE02_VALVE_IN_PIN   GPIO_NUM_9      // Válvula entrada IE02
#define IE02_VALVE_OUT_PIN  GPIO_NUM_10     // Válvula saída IE02

// ----------------------------------------------------------------------------
// LED de status (heartbeat)
// ----------------------------------------------------------------------------
#define LED_STATUS          GPIO_NUM_8      // LED onboard (compartilhado)

// ----------------------------------------------------------------------------
// Gateway ESP-NOW (Canal Fixo para Rede "luciano")
// ----------------------------------------------------------------------------
static uint8_t gateway_mac[6] = {0x80, 0xf1, 0xb2, 0x50, 0x2e, 0xc4};
#define ESPNOW_CHANNEL      11              // Canal fixo (alinhado com gateway)

// ----------------------------------------------------------------------------
// Sensor Ultrassônico AJ-SR04M
// ----------------------------------------------------------------------------
#define MIN_DISTANCE_CM     20              // Distância mínima (cm)
#define MAX_DISTANCE_CM     450             // Distância máxima (cm)
#define TIMEOUT_US          30000           // Timeout 30ms
#define VALUE_MULTIPLIER    100             // Multiplicador (cm → int)

// ----------------------------------------------------------------------------
// Filtros e Deadband
// ----------------------------------------------------------------------------
#define SAMPLES_FOR_MEDIAN  11              // Número de amostras para mediana
#define SAMPLE_INTERVAL_MS  200             // Intervalo entre amostras (ms)
#define DEADBAND_CM         2               // Deadband ±2cm

// ----------------------------------------------------------------------------
// Heartbeat e Transmissão
// ----------------------------------------------------------------------------
#define HEARTBEAT_INTERVAL_MS   30000       // Heartbeat a cada 30 segundos
#define BLINK_INTERVAL_MS       3000        // Blink LED a cada 3 segundos

// ----------------------------------------------------------------------------
// Identificadores dos Sensores (para protocolo)
// ----------------------------------------------------------------------------
#define SENSOR_IE01_ID      "IE01"
#define SENSOR_IE02_ID      "IE02"

#endif // CONFIG_H
