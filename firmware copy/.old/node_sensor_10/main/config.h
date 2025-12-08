/**
 * AGUADA - Configurações do Sensor Node
 * 
 * Firmware único para todos os 5 reservatórios:
 * - RCON (Castelo Consumo)
 * - RCAV (Castelo Incêndio)
 * - RB03 (Casa de Bombas)
 * - IE01 (Cisterna 1)
 * - IE02 (Cisterna 2)
 * 
 * Diferenciação por MAC address (hardware)
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "driver/gpio.h"

// Identificação (genérica - diferenciação por MAC)
#define FIRMWARE_VERSION "v1.2.0-fixed-channel"

// Configuração do Gateway (MAC do ESP32-C3 gateway)
static uint8_t gateway_mac[6] = {0x80, 0xf1, 0xb2, 0x50, 0x2e, 0xc4};

// ========== CANAL FIXO ESP-NOW ==========
// Canal 11 configurado para SSID "luciano"
#define ESPNOW_CHANNEL 11

// ========== PINOS GPIO (FIXOS PARA TODOS OS NODES) ==========
// AJ-SR04M Ultrasonic Sensor
#define TRIG_PIN GPIO_NUM_1
#define ECHO_PIN GPIO_NUM_0

// Válvulas Digitais
//#define VALVE_IN_PIN GPIO_NUM_2   // Válvula de entrada
#//define VALVE_OUT_PIN GPIO_NUM_3  // Válvula de saída

// Detector de Som (água entrando)
//#define SOUND_IN_PIN GPIO_NUM_5   // Digital input

// LEDs
#define LED_STATUS GPIO_NUM_8     // LED de status

// ========== SENSOR ULTRASSÔNICO ==========
#define MIN_DISTANCE_CM 0        // Mínimo do AJ-SR04M
#define MAX_DISTANCE_CM 500       // Máximo do AJ-SR04M
#define SAMPLES_FOR_MEDIAN 5     // Filtro de mediana
#define ULTRASONIC_TIMEOUT_MS 60  // Timeout de leitura
#define TIMEOUT_US (ULTRASONIC_TIMEOUT_MS * 1000)

// ========== COMPRESSÃO E TRANSMISSÃO ==========
#define DEADBAND_CM 2           // Variação mínima para transmitir (±2cm)
#define HEARTBEAT_INTERVAL_MS 60000  // Heartbeat a cada 60s
#define STABLE_STDDEV 0.5         // Desvio padrão para "estável"

// ========== ESP-NOW ==========
#define ESPNOW_QUEUE_SIZE 6
#define MAX_RETRIES 3
#define RETRY_DELAY_MS 1000

// ========== MULTIPLICADOR INTEIRO ==========
// Todos os valores float são multiplicados por 100 para transmissão
#define VALUE_MULTIPLIER 100

#endif // CONFIG_H
