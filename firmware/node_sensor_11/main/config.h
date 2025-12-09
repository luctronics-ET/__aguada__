/**
 * AGUADA v1.1 - Configuração do Sensor Node
 * 
 * Protocolo AGUADA-1 padronizado:
 * - distance_mm: Distância em milímetros (inteiro)
 * - vcc_bat_mv: Tensão da bateria/alimentação em mV
 * - rssi: Intensidade do sinal em dBm
 * 
 * Firmware universal para todos os reservatórios.
 * Diferenciação por MAC address (hardware).
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "driver/gpio.h"

// ============================================================================
// VERSÃO E IDENTIFICAÇÃO
// ============================================================================
#define FIRMWARE_VERSION    "v1.1.0"
#define FIRMWARE_NAME       "AGUADA Node Sensor"
#define PROTOCOL_VERSION    "AGUADA-1"

// ============================================================================
// CONFIGURAÇÃO DO GATEWAY
// ============================================================================
// MAC do ESP32 gateway (80:F3:DA:62:A7:84)
static const uint8_t GATEWAY_MAC[6] = {0x80, 0xF3, 0xDA, 0x62, 0xA7, 0x84};

// Canal ESP-NOW (deve corresponder ao canal WiFi do gateway)
#define ESPNOW_CHANNEL      11

// ============================================================================
// PINOS GPIO (HARDWARE PADRÃO)
// ============================================================================
//
// ┌─────────────────────────────────────────────────────────────────────────┐
// │                    DIAGRAMA DE CONEXÕES ESP32-C3                        │
// ├─────────────────────────────────────────────────────────────────────────┤
// │                                                                         │
// │  ┌───────────────┐          ┌─────────────────┐                         │
// │  │  ESP32-C3     │          │   AJ-SR04M      │                         │
// │  │  SuperMini    │          │   Ultrassônico  │                         │
// │  │               │          │                 │                         │
// │  │  GPIO 1 ──────┼───────── │ TRIG            │                         │
// │  │  GPIO 0 ──────┼───────── │ ECHO            │                         │
// │  │  5V    ───────┼───────── │ VCC             │                         │
// │  │  GND   ───────┼───────── │ GND             │                         │
// │  │               │          └─────────────────┘                         │
// │  │               │                                                      │
// │  │               │          ┌─────────────────┐                         │
// │  │               │          │ Divisor VCC     │                         │
// │  │               │          │                 │                         │
// │  │  GPIO 4 ──────┼───────── │ MEIO (ADC)      │◄── 10kΩ ──┬── VCC (5V)  │
// │  │  GND   ───────┼───────── │ GND             │           │             │
// │  │               │          │                 │◄── 10kΩ ──┴── GND       │
// │  │               │          └─────────────────┘                         │
// │  │               │                                                      │
// │  │  GPIO 8 ──────┼── LED ──── GND (330Ω em série)                       │
// │  └───────────────┘                                                      │
// │                                                                         │
// │  DIVISOR DE TENSÃO VCC:                                                 │
// │  ─────────────────────                                                  │
// │  VCC (5V) ─── R1 (10kΩ) ─┬─ R2 (10kΩ) ─── GND                           │
// │                          │                                              │
// │                          └─── GPIO 4 (ADC)                              │
// │                                                                         │
// │  Fórmula: V_adc = VCC × R2/(R1+R2) = VCC × 0.5                          │
// │  Logo:    VCC = V_adc × 2                                               │
// │                                                                         │
// │  Com VCC=5V: V_adc = 2.5V (dentro do range 0-3.3V do ADC)               │
// │                                                                         │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Sensor Ultrassônico AJ-SR04M
#define PIN_TRIG            GPIO_NUM_1      // Trigger (output) → AJ-SR04M TRIG
#define PIN_ECHO            GPIO_NUM_0      // Echo (input) ← AJ-SR04M ECHO

// LED de Status
#define PIN_LED_STATUS      GPIO_NUM_8      // LED heartbeat (output)

// ADC para leitura de VCC (via divisor resistivo)
// GPIO4 = ADC1_CH4 no ESP32-C3
// IMPORTANTE: Conectar divisor 10k+10k entre VCC e GND, ponto médio no GPIO4
#define PIN_VCC_ADC         GPIO_NUM_4      // ADC input ← Divisor VCC
#define ADC_CHANNEL         ADC_CHANNEL_4
#define ADC_UNIT            ADC_UNIT_1
#define ADC_ATTEN           ADC_ATTEN_DB_12     // 0-3.3V range

// ============================================================================
// SENSOR ULTRASSÔNICO AJ-SR04M
// ============================================================================
// Range do sensor
#define SENSOR_MIN_MM       20      // Mínimo detectável (2cm)
#define SENSOR_MAX_MM       4500    // Máximo detectável (450cm)

// Configuração de leitura
#define SENSOR_TIMEOUT_US   60000   // Timeout 60ms (para 450cm max)
#define SAMPLES_PER_READ    11      // Número de amostras para mediana
#define SAMPLE_INTERVAL_MS  100     // Intervalo entre amostras (100ms)

// ============================================================================
// TEMPOS E INTERVALOS
// ============================================================================
// Ciclo de leitura
#define READ_INTERVAL_MS    2000    // Ler sensor a cada 2 segundos

// Heartbeat (envio forçado mesmo sem mudança)
#define HEARTBEAT_MS        30000   // Forçar envio a cada 30 segundos

// ============================================================================
// COMPRESSÃO DE DADOS (DEADBAND)
// ============================================================================
// Delta mínimo para considerar mudança significativa
#define DELTA_DISTANCE_MM   15      // ±15mm (1.5cm) de variação
#define DELTA_VCC_MV        100     // ±100mV de variação

// Estabilidade (considerado estável se desvio padrão < threshold)
#define STABLE_STDDEV_MM    5.0     // Desvio padrão para considerar estável

// ============================================================================
// ESP-NOW
// ============================================================================
#define ESPNOW_QUEUE_SIZE   6       // Tamanho da fila de pacotes
#define ESPNOW_MAX_RETRIES  3       // Tentativas de envio
#define ESPNOW_RETRY_MS     500     // Delay entre tentativas

// Tamanho máximo do payload JSON
#define MAX_PAYLOAD_SIZE    200

// ============================================================================
// BATERIA / ALIMENTAÇÃO (ADC)
// ============================================================================
// Divisor resistivo: 10k + 10k = 2:1 (5V → 2.5V no ADC)
// Com atenuação 12dB: range 0-2450mV (efetivo)
// Fórmula: VCC = (ADC_mV × DIVISOR) 
#define VCC_DIVIDER_RATIO   2.0     // R1=10k, R2=10k → ratio = 2
#define VCC_USB_MV          5000    // Fallback se ADC não inicializado
#define VCC_ADC_SAMPLES     5       // Amostras para média

// Limites de tensão (para alertas)
#define VCC_MIN_MV          4500    // Abaixo disso = alerta
#define VCC_MAX_MV          5500    // Acima disso = anormal

// ============================================================================
// FILTRAGEM AVANÇADA (EMA - Exponential Moving Average)
// ============================================================================
// Suavização: 0.0 = sem suavização, 1.0 = máxima suavização
// EMA = alpha * novo_valor + (1 - alpha) * valor_anterior
#define EMA_ALPHA           0.3     // 30% novo, 70% histórico
#define USE_EMA_FILTER      1       // 1 = usar EMA após mediana

// Histerese (previne oscilação no limiar do delta)
// Só envia se delta > DELTA_MM + HISTERESE ao subir
// Só envia se delta > DELTA_MM - HISTERESE ao descer
#define HYSTERESIS_MM       3       // ±3mm de histerese

// ============================================================================
// COMPRESSÃO RLE (Run-Length Encoding)
// ============================================================================
// Conta leituras consecutivas estáveis e envia contador no heartbeat
#define USE_RLE             1       // 1 = ativar RLE
#define RLE_MAX_COUNT       255     // Máximo de leituras estáveis (uint8_t)

// ============================================================================
// AGREGAÇÃO TEMPORAL
// ============================================================================
// Coleta estatísticas e envia min/max/avg no heartbeat
#define USE_AGGREGATION     1       // 1 = ativar agregação
#define AGG_WINDOW_SIZE     60      // Janela de agregação (nº de leituras)

// ============================================================================
// PAYLOAD BINÁRIO
// ============================================================================
// Reduz ~60 bytes JSON para ~16 bytes binário
// Formato: [MAGIC(2)][MAC(6)][DIST(2)][VCC(2)][RSSI(1)][FLAGS(1)][CRC(2)]
#define USE_BINARY_PAYLOAD  0       // 0 = JSON, 1 = binário
#define BINARY_MAGIC        0xAD01  // "AGUADA-1" identifier
#define BINARY_PAYLOAD_SIZE 16      // Bytes do payload binário

// Flags do payload binário
#define FLAG_HEARTBEAT      0x01    // Envio por heartbeat
#define FLAG_DELTA          0x02    // Envio por delta
#define FLAG_ERROR          0x04    // Erro de leitura
#define FLAG_AGGREGATED     0x08    // Contém dados agregados
#define FLAG_LOW_BATTERY    0x10    // VCC abaixo do limite

// ============================================================================
// DEBUG
// ============================================================================
#define DEBUG_LEVEL         ESP_LOG_INFO    // ESP_LOG_DEBUG para verbose
#define STATS_INTERVAL      10              // Log stats a cada N envios

#endif // CONFIG_H
