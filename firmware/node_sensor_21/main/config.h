/**
 * AGUADA v1.2 - Node Sensor 21 (Dual Ultrasonic)
 * 
 * Firmware para monitoramento de 2 cisternas: IE01 e IE02
 * Cada sensor envia pacotes como se fosse um node separado:
 * - IE01: Usa MAC real do ESP32-C3
 * - IE02: Usa MAC virtual (AA:BB:CC:DD:1E:02)
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "driver/gpio.h"

// ============================================================================
// VERSÃO E IDENTIFICAÇÃO
// ============================================================================
#define FIRMWARE_VERSION    "v1.2.0"
#define FIRMWARE_NAME       "AGUADA Node Sensor 21 (Dual)"
#define PROTOCOL_VERSION    "AGUADA-1"

// ============================================================================
// CONFIGURAÇÃO DO GATEWAY
// ============================================================================
static const uint8_t GATEWAY_MAC[6] = {0x80, 0xf1, 0xb2, 0x50, 0x2e, 0xc4};
#define ESPNOW_CHANNEL      11

// ============================================================================
// MAC VIRTUAL PARA IE02
// ============================================================================
// IE01: Usa MAC real do ESP32-C3
// IE02: Usa MAC virtual para aparecer como node separado
// AA:BB:CC:DD:1E:02 ("1E" = IE, "02" = cisterna 2)

// ============================================================================
// PINOS GPIO - DUAL ULTRASONIC
// ============================================================================
//
// ┌─────────────────────────────────────────────────────────────────────────┐
// │                    NODE SENSOR 21 (DUAL ULTRASONIC)                     │
// │                    IE01 + IE02 (Cisternas Ilha do Engenho)              │
// ├─────────────────────────────────────────────────────────────────────────┤
// │                                                                         │
// │  ESP32-C3              AJ-SR04M #1 (IE01)    AJ-SR04M #2 (IE02)         │
// │  ─────────             ─────────────────     ─────────────────          │
// │  GPIO 1 ────────────── TRIG                                             │
// │  GPIO 0 ────────────── ECHO                                             │
// │  GPIO 3 ──────────────────────────────────── TRIG                       │
// │  GPIO 2 ──────────────────────────────────── ECHO                       │
// │  5V     ────────────── VCC ───────────────── VCC                        │
// │  GND    ────────────── GND ───────────────── GND                        │
// │                                                                         │
// │  GPIO 4 ────────────── Divisor VCC (10k+10k)                            │
// │  GPIO 8 ────────────── LED Status                                       │
// │                                                                         │
// └─────────────────────────────────────────────────────────────────────────┘

// Sensor Ultrassônico IE01 (Cisterna 1)
#define PIN_TRIG_IE01       GPIO_NUM_1
#define PIN_ECHO_IE01       GPIO_NUM_0

// Sensor Ultrassônico IE02 (Cisterna 2)
#define PIN_TRIG_IE02       GPIO_NUM_3
#define PIN_ECHO_IE02       GPIO_NUM_2

// LED de Status
#define PIN_LED_STATUS      GPIO_NUM_8

// ADC para leitura de VCC
#define PIN_VCC_ADC         GPIO_NUM_4
#define ADC_CHANNEL         ADC_CHANNEL_4
#define ADC_UNIT            ADC_UNIT_1
#define ADC_ATTEN           ADC_ATTEN_DB_12

// ============================================================================
// SENSOR ULTRASSÔNICO AJ-SR04M
// ============================================================================
#define SENSOR_MIN_MM       20
#define SENSOR_MAX_MM       4500
#define SENSOR_TIMEOUT_US   60000
#define SAMPLES_PER_READ    11
#define SAMPLE_INTERVAL_MS  100

// ============================================================================
// TEMPOS E INTERVALOS
// ============================================================================
#define READ_INTERVAL_MS    2000
#define HEARTBEAT_MS        120000

// ============================================================================
// COMPRESSÃO DE DADOS (DEADBAND)
// ============================================================================
#define DELTA_DISTANCE_MM   15
#define DELTA_VCC_MV        100
#define HYSTERESIS_MM       5

// ============================================================================
// ESP-NOW
// ============================================================================
#define ESPNOW_QUEUE_SIZE   6
#define ESPNOW_MAX_RETRIES  3
#define ESPNOW_RETRY_MS     500
#define MAX_PAYLOAD_SIZE    200

// ============================================================================
// BATERIA / ALIMENTAÇÃO (ADC)
// ============================================================================
#define VCC_DIVIDER_RATIO   2.0
#define VCC_USB_MV          5000
#define VCC_ADC_SAMPLES     5
#define VCC_MIN_MV          4500
#define VCC_MAX_MV          5500

// ============================================================================
// FILTRAGEM
// ============================================================================
#define EMA_ALPHA           0.3
#define USE_EMA_FILTER      1
#define USE_RLE             1
#define RLE_MAX_COUNT       255
#define STATS_INTERVAL      10

#endif // CONFIG_H
