# 🚀 AGUADA Firmware Modernization Proposal

**Data**: 2025-12-10  
**Versão**: 2.0  
**Status**: PROPOSTA

---

## 📊 Executive Summary

Proposta de modernização dos firmwares ESP32 do sistema AGUADA baseada em **padrões de mercado**, **frameworks consolidados** e **melhores práticas IoT industriais**.

### Objetivos

- ✅ Aumentar confiabilidade (uptime > 99.9%)
- ✅ Reduzir consumo de energia (bateria: 6 meses → 2 anos)
- ✅ Facilitar manutenção e debug
- ✅ Padronizar com protocolos industriais
- ✅ Preparar para escalabilidade (100+ nodes)

---

## 🔍 Análise do Estado Atual

### Pontos Fortes ✅

1. **Protocolo AGUADA-1** bem definido (JSON compacto)
2. **ESP-NOW** para comunicação de baixa latência
3. **Mediana filtering** para ruído ultrassônico
4. **Delta encoding** para economia de banda
5. **Heartbeat** confiável (30s)
6. **Firmware universal** (MAC-based ID)

### Pontos de Melhoria ⚠️

1. **Sem OTA** (Over-The-Air updates) - requer acesso físico
2. **Sem persistência** - perde dados em reboot
3. **Sem watchdog** robusto - pode travar indefinidamente
4. **Logging básico** - dificulta debug remoto
5. **Sem criptografia** - vulnerável a spoofing
6. **Sem RTOS tasks** organizadas - código monolítico
7. **Sem power management** - consume 100% mesmo idle
8. **Sem telemetria de saúde** - não reporta temperatura CPU, RAM, etc.

---

## 🏗️ Arquitetura Proposta: ESP-IDF Component-Based

### Framework Base: **ESP-IDF 5.3 LTS**

- ✅ Suporte oficial até 2028
- ✅ Component system para modularidade
- ✅ FreeRTOS nativo
- ✅ OTA robusto com rollback
- ✅ Power management avançado
- ✅ NVS (Non-Volatile Storage) para config

### Opções de Framework (Comparativo)

| Framework         | Prós                                           | Contras                       | Recomendação                    |
| ----------------- | ---------------------------------------------- | ----------------------------- | ------------------------------- |
| **ESP-IDF**       | Controle total, performance máxima, OTA nativo | Curva de aprendizado          | ⭐⭐⭐⭐⭐ **RECOMENDADO**      |
| **Arduino-ESP32** | Fácil, muitas libs, comunidade                 | Menos eficiente, OTA limitado | ⭐⭐⭐ Prototipagem rápida      |
| **PlatformIO**    | Multi-framework, CI/CD                         | Overhead de abstração         | ⭐⭐⭐⭐ Bom para times grandes |
| **Zephyr RTOS**   | Industrial, certificado                        | Complexidade, documentação    | ⭐⭐ Overkill para AGUADA       |
| **Mongoose OS**   | Cloud-first, OTA                               | Descontinuado (2023)          | ❌ Não usar                     |

**Decisão**: **ESP-IDF 5.3 LTS** mantém controle e adiciona features industriais.

---

## 🎯 Estrutura de Componentes Modular

```
firmware/
├── components/
│   ├── aguada_core/           # Core AGUADA (protocolo, types)
│   │   ├── include/aguada_protocol.h
│   │   ├── aguada_config.c
│   │   └── CMakeLists.txt
│   ├── aguada_sensor/         # Sensores (ultrassônico, ADC)
│   │   ├── ultrasonic_ajsr04m.c
│   │   ├── adc_battery.c
│   │   └── sensor_fusion.c
│   ├── aguada_comm/           # Comunicação (ESP-NOW, MQTT)
│   │   ├── espnow_transport.c
│   │   ├── mqtt_client.c     # Para gateway
│   │   └── http_client.c
│   ├── aguada_storage/        # Persistência (NVS, SPIFFS)
│   │   ├── nvs_config.c
│   │   ├── circular_buffer.c # Ring buffer em RAM
│   │   └── flash_log.c
│   ├── aguada_power/          # Gerenciamento de energia
│   │   ├── deep_sleep.c
│   │   ├── light_sleep.c
│   │   └── power_budget.c
│   ├── aguada_ota/            # Atualizações OTA
│   │   ├── ota_manager.c
│   │   ├── rollback.c
│   │   └── signature_verify.c
│   └── aguada_health/         # Telemetria de saúde
│       ├── system_monitor.c
│       ├── error_reporter.c
│       └── diagnostics.c
├── node_sensor_v2/            # Node modernizado
│   ├── main/
│   │   ├── main.c             # Task orchestration
│   │   ├── tasks/
│   │   │   ├── sensor_task.c
│   │   │   ├── comm_task.c
│   │   │   ├── watchdog_task.c
│   │   │   └── health_task.c
│   │   └── CMakeLists.txt
│   └── sdkconfig.defaults
└── gateway_v2/                # Gateway modernizado
    └── main/
        ├── main.c
        ├── tasks/
        └── CMakeLists.txt
```

---

## 📡 Protocolo AGUADA-2: Evolução

### Mantém Compatibilidade com AGUADA-1

```json
// AGUADA-1 (atual) - Manter suporte
{
  "mac": "80:F1:B2:50:31:34",
  "distance_mm": 2450,
  "vcc_bat_mv": 5000,
  "rssi": -50
}
```

### AGUADA-2: Adiciona Telemetria de Saúde

```json
{
  "v": 2, // Protocol version
  "mac": "80:F1:B2:50:31:34",
  "ts": 1702234567, // Unix timestamp
  "data": {
    "distance_mm": 2450,
    "vcc_bat_mv": 4200,
    "rssi": -50
  },
  "health": {
    "uptime_s": 86400, // Uptime em segundos
    "free_heap": 180000, // RAM livre (bytes)
    "min_heap": 150000, // Mínimo histórico
    "cpu_temp": 45, // Temperatura CPU (°C)
    "reboot_reason": 1, // 1=normal, 2=watchdog, 3=panic
    "fw_version": "2.0.1",
    "tx_ok": 2880, // Pacotes enviados OK
    "tx_fail": 5, // Falhas de envio
    "sensor_errors": 2 // Erros de leitura
  }
}
```

### Formato Binário Compacto (Opcional - Low Power)

```c
// 32 bytes - Para modo deep sleep
typedef struct __attribute__((packed)) {
    uint16_t magic;            // 0xAD02 = AGUADA-2
    uint8_t version;           // Protocol version
    uint8_t mac[6];            // MAC address
    uint32_t timestamp;        // Unix timestamp
    int16_t distance_mm;       // Distância (-32k a +32k)
    uint16_t vcc_mv;           // Tensão (0-65535mV)
    int8_t rssi;               // RSSI (-128 a +127)
    uint8_t flags;             // Bit flags (error, low_bat, etc)
    uint32_t uptime_s;         // Uptime
    uint32_t free_heap;        // RAM livre
    int8_t cpu_temp;           // Temperatura
    uint16_t tx_ok;            // Pacotes OK
    uint16_t tx_fail;          // Pacotes falhos
    uint16_t crc16;            // Checksum
} aguada2_packet_t;
```

---

## ⚡ Power Management: Modos de Operação

### Modo 1: **Always-On** (Atual)

- **Consumo**: ~80mA @ 3.3V = 264mW
- **Bateria 2000mAh**: ~25 horas
- **Uso**: Gateway (sempre conectado)

### Modo 2: **Light Sleep** (NOVO - Recomendado)

```c
// Acorda a cada 2s para ler sensor
// ESP-NOW mantém conexão
// Consumo médio: 15mA
// Bateria 2000mAh: ~5.5 dias
```

### Modo 3: **Deep Sleep** (NOVO - Ultra Low Power)

```c
// Acorda a cada 30s via RTC timer
// Re-inicia WiFi/ESP-NOW a cada ciclo
// Consumo médio: 0.15mA (dormindo) + 80mA*2s (acordado)
// Bateria 2000mAh: ~180 dias (6 meses)
```

### Modo 4: **Modem Sleep** (NOVO - Híbrido)

```c
// CPU ativo, WiFi em power save
// Acordar WiFi sob demanda
// Consumo: 30-40mA
// Bateria 2000mAh: ~2 dias
```

### Comparativo de Autonomia

| Modo        | Consumo Médio | Bateria 2000mAh | Bateria 5000mAh | Recomendação              |
| ----------- | ------------- | --------------- | --------------- | ------------------------- |
| Always-On   | 80mA          | 25h             | 62h             | Gateway USB               |
| Light Sleep | 15mA          | 5.5 dias        | 14 dias         | **Sensor 5V DC** ⭐       |
| Deep Sleep  | 0.5mA\*       | 166 dias        | 416 dias        | **Sensor bateria** ⭐⭐⭐ |
| Modem Sleep | 35mA          | 2.4 dias        | 6 dias          | Teste apenas              |

\*média considerando wake-up cycles

---

## 🔒 Segurança: Criptografia e Autenticação

### ESP-NOW Secure (NOVO)

```c
// Chave compartilhada (256-bit AES)
uint8_t espnow_key[16] = {
    0xAB, 0xCD, 0xEF, ... // Gerar aleatório na produção
};

esp_now_set_pmk(espnow_key);
```

### Message Authentication Code (HMAC-SHA256)

```c
// Assina cada pacote
typedef struct {
    aguada2_packet_t payload;
    uint8_t hmac[32];  // HMAC-SHA256 signature
} signed_packet_t;
```

### Recomendações

- ✅ **ESP-NOW PMK** para criptografia básica (gratuito)
- ✅ **HMAC** para autenticação de mensagens
- ⚠️ **TLS/HTTPS** no gateway → backend (já implementado?)
- ⚠️ **Provisioning seguro** - não hardcode keys no código!

---

## 🔄 OTA (Over-The-Air Updates)

### Arquitetura OTA

```
┌──────────────┐  HTTPS   ┌──────────────┐  ESP-NOW  ┌──────────────┐
│   Backend    │ ────────► │   Gateway    │ ─────────► │  Node Sensor │
│  (OTA Server)│           │  (OTA Relay) │            │  (Target)    │
└──────────────┘           └──────────────┘            └──────────────┘
       │                          │                            │
       │ 1. Upload firmware.bin   │                            │
       │ ────────────────────────►│                            │
       │                          │ 2. Notify update available │
       │                          │ ──────────────────────────►│
       │                          │                            │ 3. Download chunks
       │                          │◄────────────────────────────│
       │                          │                            │ 4. Write to flash
       │                          │                            │ 5. Verify & reboot
       │                          │◄────────────────────────────│ 6. Confirm success
       │◄─────────────────────────│
```

### Partições Flash (4MB)

```
0x9000   - nvs        (24KB) - Config persistente
0xF000   - otadata    (8KB)  - OTA status
0x10000  - factory    (1MB)  - Firmware original (rollback)
0x110000 - ota_0      (1MB)  - Slot OTA primário
0x210000 - ota_1      (1MB)  - Slot OTA secundário
0x310000 - spiffs     (960KB)- Logs/cache
```

### Features OTA

- ✅ **Rollback automático** - volta firmware anterior se falhar boot
- ✅ **Differential updates** - envia apenas diff binário (economia 70%)
- ✅ **Staged rollout** - atualiza 1 node por vez (canary deployment)
- ✅ **Version management** - reporta versão em health

---

## 🧪 Framework de Testes

### Unit Tests (ESP-IDF Unity)

```c
// components/aguada_sensor/test/test_ultrasonic.c
TEST_CASE("Ultrasonic sensor timeout", "[sensor]") {
    int32_t distance = ultrasonic_read(0);  // timeout = 0ms
    TEST_ASSERT_EQUAL_INT32(-1, distance);
}

TEST_CASE("Ultrasonic median filter", "[sensor]") {
    int32_t samples[] = {100, 105, 102, 200, 103};  // 200 = outlier
    int32_t median = calculate_median(samples, 5);
    TEST_ASSERT_EQUAL_INT32(103, median);
}
```

### Integration Tests

```bash
# CI/CD Pipeline
pytest tests/integration/test_espnow_gateway.py
pytest tests/integration/test_sensor_backend.py
```

### Hardware-in-the-Loop (HIL)

```python
# Simula sensor físico com mock
import serial
ser = serial.Serial('/dev/ttyACM0', 115200)

def test_sensor_heartbeat():
    time.sleep(35)  # Espera heartbeat (30s + margem)
    data = ser.readline()
    packet = json.loads(data)
    assert 'mac' in packet
    assert 'distance_mm' in packet
```

---

## 📊 Monitoring e Observabilidade

### Prometheus Metrics (Gateway expõe)

```
# HELP aguada_packets_total Total packets received
# TYPE aguada_packets_total counter
aguada_packets_total{node="80:F1:B2:50:31:34",type="distance"} 2880

# HELP aguada_sensor_distance_mm Current distance reading
# TYPE aguada_sensor_distance_mm gauge
aguada_sensor_distance_mm{node="80:F1:B2:50:31:34",reservoir="RCON"} 2450

# HELP aguada_node_uptime_seconds Node uptime
# TYPE aguada_node_uptime_seconds counter
aguada_node_uptime_seconds{node="80:F1:B2:50:31:34"} 86400

# HELP aguada_battery_mv Battery voltage
# TYPE aguada_battery_mv gauge
aguada_battery_mv{node="80:F1:B2:50:31:34"} 4200
```

### Logging Estruturado (JSON)

```json
{
  "timestamp": "2025-12-10T20:45:30Z",
  "level": "ERROR",
  "component": "ultrasonic",
  "node": "80:F1:B2:50:31:34",
  "message": "Sensor timeout after 30ms",
  "context": {
    "attempts": 3,
    "last_valid": 2450,
    "gpio_trig": 1,
    "gpio_echo": 0
  }
}
```

---

## 🎯 Roadmap de Implementação

### Fase 1: Refatoração (2 semanas)

- [ ] Migrar código para componentes ESP-IDF
- [ ] Implementar RTOS tasks estruturadas
- [ ] Adicionar NVS para configuração
- [ ] Implementar watchdog robusto
- [ ] Logging estruturado

**Entregável**: Firmware v2.0 com mesma funcionalidade mas código limpo

### Fase 2: Power Management (1 semana)

- [ ] Implementar Light Sleep mode
- [ ] Benchmark consumo real
- [ ] Testes de autonomia (24h+)

**Entregável**: Firmware v2.1 com 5x mais autonomia

### Fase 3: OTA (2 semanas)

- [ ] Implementar OTA manager
- [ ] Backend OTA server
- [ ] Gateway OTA relay
- [ ] Testes de rollback

**Entregável**: Firmware v2.2 com OTA funcional

### Fase 4: Segurança (1 semana)

- [ ] ESP-NOW PMK encryption
- [ ] HMAC signing
- [ ] Key provisioning

**Entregável**: Firmware v2.3 com criptografia

### Fase 5: Telemetria Avançada (1 semana)

- [ ] Protocolo AGUADA-2
- [ ] Health metrics
- [ ] Prometheus exporter

**Entregável**: Firmware v3.0 production-ready

---

## 💰 Análise de Custo-Benefício

### Investimento

- **Tempo desenvolvimento**: ~7 semanas (1 dev full-time)
- **Custo estimado**: R$ 20.000 - 30.000 (dev + testes)
- **Hardware adicional**: R$ 500 (baterias para testes)

### Retorno

- ✅ **Redução manutenção**: 70% menos visitas (OTA remoto)
- ✅ **Economia bateria**: 5-10x mais autonomia
- ✅ **Confiabilidade**: 99.9% uptime (watchdog + rollback)
- ✅ **Escalabilidade**: Suporta 100+ nodes sem mudanças
- ✅ **Debug remoto**: Logs estruturados facilitam troubleshooting
- ✅ **Segurança**: Proteção contra spoofing/tampering

**ROI estimado**: 6-12 meses

---

## 🛠️ Ferramentas e Bibliotecas Recomendadas

### Desenvolvimento

- ✅ **ESP-IDF 5.3 LTS** - Framework base
- ✅ **PlatformIO** (opcional) - Build system alternativo
- ✅ **esptool.py** - Flash tool
- ✅ **ESP-IDF Monitor** - Serial monitor avançado

### CI/CD

- ✅ **GitHub Actions** - Automated builds
- ✅ **Docker** - Build environment padronizado
- ✅ **pytest** - Integration tests
- ✅ **cppcheck** - Static analysis

### Debugging

- ✅ **OpenOCD + GDB** - Hardware debugging (ESP32-C3 tem JTAG!)
- ✅ **ESP-IDF Core Dump** - Post-mortem analysis
- ✅ **Heap tracing** - Memory leak detection
- ✅ **Task Watchdog** - Deadlock detection

### Bibliotecas Úteis

```c
// JSON: cJSON (já incluído no ESP-IDF)
#include "cJSON.h"

// MQTT: esp-mqtt (nativo)
#include "mqtt_client.h"

// HTTP: esp_http_client (nativo)
#include "esp_http_client.h"

// Cryptografia: mbedtls (nativo)
#include "mbedtls/sha256.h"
#include "mbedtls/aes.h"

// OTA: esp_ota_ops (nativo)
#include "esp_ota_ops.h"
```

---

## 📚 Referências e Standards

### Padrões IoT

- **MQTT 5.0** - ISO/IEC 20922
- **CoAP** - RFC 7252 (alternativa leve ao HTTP)
- **LwM2M** - OMA Lightweight M2M (device management)
- **JSON Schema** - Validação de payloads

### ESP32 Best Practices

- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [ESP32 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32-c3_technical_reference_manual_en.pdf)
- [ESP-NOW Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/api-reference/network/esp_now.html)
- [Low Power Design ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/low-power-mode.html)

### Segurança

- **OWASP IoT Top 10** - Security guidelines
- **ESP32 Secure Boot** - Chain of trust
- **Flash Encryption** - Protect firmware

---

## 🎓 Training e Documentação

### Para a Equipe

1. **ESP-IDF Crash Course** (1 semana)

   - FreeRTOS tasks
   - Component system
   - NVS storage
   - OTA updates

2. **Hands-on Workshop** (2 dias)

   - Flash firmware v2.0
   - Configurar OTA
   - Debug com OpenOCD
   - Análise de crashes

3. **Documentação Técnica**
   - Architecture Decision Records (ADRs)
   - API documentation (Doxygen)
   - Troubleshooting guide

---

## ✅ Checklist de Migração

### Pré-requisitos

- [ ] ESP-IDF 5.3 instalado
- [ ] Hardware de teste (2x ESP32-C3)
- [ ] Backup firmware atual
- [ ] Plano de rollback

### Development

- [ ] Criar branch `firmware-v2`
- [ ] Setup CI/CD pipeline
- [ ] Implementar componentes
- [ ] Unit tests (cobertura > 70%)
- [ ] Integration tests

### Testing

- [ ] Teste funcional (todas features)
- [ ] Teste de stress (24h contínuo)
- [ ] Teste de autonomia (bateria)
- [ ] Teste OTA (10 cycles)
- [ ] Teste de segurança

### Deployment

- [ ] Pilot em 1 node (RCON)
- [ ] Monitoramento 48h
- [ ] Rollout gradual (1 node/dia)
- [ ] Documentação de deploy

---

## 🚦 Decisão: Go/No-Go

### ✅ GO - Se:

- Sistema atual apresenta problemas frequentes
- Planejamento de expansão (>10 nodes)
- Budget disponível (R$ 30k)
- Time técnico disponível (1-2 devs × 2 meses)

### ⚠️ NO-GO - Se:

- Sistema atual 100% estável
- Apenas 3-4 nodes (escala pequena)
- Budget limitado
- Time sobrecarregado

### 🎯 RECOMENDAÇÃO FINAL

**GO com Roadmap Faseado**:

1. **Curto prazo (1 mês)**: Fase 1 (Refatoração) + Fase 2 (Power Mgmt)

   - Impacto imediato: código limpo + 5x autonomia
   - Baixo risco: mantém funcionalidade atual

2. **Médio prazo (2-3 meses)**: Fase 3 (OTA) + Fase 4 (Security)

   - Impacto alto: reduz manutenção física em 70%
   - ROI rápido: paga investimento em 6 meses

3. **Longo prazo (4-6 meses)**: Fase 5 (Telemetria) + Escalabilidade
   - Prepara sistema para crescimento
   - Monitoring profissional

---

**Autor**: GitHub Copilot (Claude Sonnet 4.5)  
**Revisão**: Equipe AGUADA  
**Próximo passo**: Revisar proposta e aprovar roadmap
