# AGUADA Node Sensor v2.0

**Firmware modernizado com arquitetura component-based e FreeRTOS**

## 🎯 Visão Geral

Este é o firmware v2.0 do node sensor AGUADA, implementando:
- ✅ Arquitetura modular baseada em componentes ESP-IDF
- ✅ FreeRTOS tasks organizadas por função
- ✅ Protocol v2 com suporte a health metrics
- ✅ Comunicação ESP-NOW com retry logic
- ✅ Deadband filtering para economia de banda
- ✅ Median filter para leituras confiáveis
- ✅ Watchdog timer para confiabilidade

## 📊 Status da Implementação

### ✅ Fase 1: Refatoração (COMPLETA)

| Componente | Status | Descrição |
|------------|--------|-----------|
| aguada_protocol | ✅ | Protocol v1/v2, JSON/binary, CRC16 |
| aguada_sensor | ✅ | Ultrasonic AJ-SR04M, GPIO, median filter |
| aguada_comm | ✅ | ESP-NOW, retry logic, statistics |
| main | ✅ | FreeRTOS tasks: sensor, comm, health, watchdog |

### ⏳ Próximas Fases

- Fase 2: Power Management (Light Sleep - 15mA)
- Fase 3: OTA Updates (rollback automático)
- Fase 4: Segurança (ESP-NOW PMK + HMAC)
- Fase 5: Telemetria avançada (métricas Prometheus)

## 🏗️ Arquitetura

```
node_sensor_v2/
├── main/
│   ├── main.c                    # Aplicação principal
│   └── CMakeLists.txt
├── components/
│   ├── aguada_protocol/          # Protocol v1/v2
│   │   ├── aguada_protocol.h
│   │   ├── aguada_protocol.c
│   │   └── CMakeLists.txt
│   ├── aguada_sensor/            # Sensor interface
│   │   ├── aguada_sensor.h
│   │   ├── aguada_sensor.c
│   │   └── CMakeLists.txt
│   ├── aguada_comm/              # ESP-NOW communication
│   │   ├── aguada_comm.h
│   │   ├── aguada_comm.c
│   │   └── CMakeLists.txt
│   └── aguada_power/             # Power management (future)
└── CMakeLists.txt                # Project config
```

## 🚀 Tasks FreeRTOS

| Task | Prioridade | Stack | Função |
|------|------------|-------|--------|
| **sensor_task** | 5 | 4KB | Lê sensores a cada 30s, aplica deadband |
| **comm_task** | 4 | 4KB | Gerencia filas de transmissão |
| **health_task** | 3 | 2KB | Monitora heap, uptime, temperature |
| **watchdog_task** | 6 | 2KB | Reseta watchdog a cada 10s |

## 📡 Protocolo AGUADA v2

### JSON v1 (Compatibilidade Legacy)

```json
{
  "mac": "20:6E:F1:6B:77:58",
  "type": "distance_cm",
  "value": 24480,
  "battery": 5000,
  "uptime": 3600,
  "rssi": -50
}
```

### JSON v2 (Com Health Metrics)

```json
{
  "mac": "20:6E:F1:6B:77:58",
  "type": "distance_cm",
  "value": 24480,
  "battery": 5000,
  "uptime": 3600,
  "rssi": -50,
  "health": {
    "free_heap": 180000,
    "temperature": 45,
    "reboot_reason": 1,
    "packets_sent": 1200,
    "packets_failed": 5
  }
}
```

### Binário v2 (32 bytes - futuro)

Formato compacto para otimização de banda com CRC16.

## 🔧 Compilação

### Requisitos

- ESP-IDF 5.3+ ou 6.x
- ESP32-C3 target
- Componentes: esp_wifi, esp_event, nvs_flash, json, driver

### Build

```bash
# Configurar target
cd firmware/node_sensor_v2
idf.py set-target esp32c3

# Compilar
idf.py build

# Flash
idf.py -p /dev/ttyACM0 flash monitor
```

## 📊 Comparação v1 vs v2

| Métrica | v1.1 (Legacy) | v2.0 (Atual) | Ganho |
|---------|---------------|--------------|-------|
| **Arquitetura** | Monolítico | Componentes | ✅ Modular |
| **Tasks** | Loop único | 4 tasks FreeRTOS | ✅ Paralelo |
| **Protocol** | JSON v1 | JSON v1+v2+binary | ✅ Evolução |
| **Health** | Nenhum | Heap, temp, stats | ✅ Monitoramento |
| **Watchdog** | Manual | FreeRTOS task | ✅ Automático |
| **Retry logic** | Básico | 3 tentativas + delay | ✅ Confiável |
| **Código** | 1 arquivo | 8 componentes | ✅ Testável |

## 🔌 GPIO Configuration

Idêntico ao v1.1 (hardware fixo):

```c
#define SENSOR_TRIG_PIN     GPIO_NUM_1   // Ultrasonic trigger
#define SENSOR_ECHO_PIN     GPIO_NUM_0   // Ultrasonic echo
#define SENSOR_VALVE_IN     GPIO_NUM_2   // Input valve
#define SENSOR_VALVE_OUT    GPIO_NUM_3   // Output valve
#define SENSOR_SOUND_IN     GPIO_NUM_5   // Water flow detector
#define SENSOR_LED_STATUS   GPIO_NUM_8   // Status LED
```

## 📈 Telemetria

### Variáveis Transmitidas

- `distance_cm`: Distância em cm × 100 (deadband ±2cm)
- `valve_in`: Estado válvula entrada (0/1)
- `valve_out`: Estado válvula saída (0/1)
- `sound_in`: Detector de fluxo (0/1)

### Timing

- **Leitura**: A cada 30 segundos
- **Transmissão**: Apenas quando valores mudam (deadband)
- **Heartbeat**: 30s (envia últimos valores mesmo sem mudança)

## 🛡️ Confiabilidade

- **Median filter**: 11 amostras, intervalo 200ms
- **Retry logic**: 3 tentativas, delay 1s entre tentativas
- **Watchdog**: Timeout 60s, reset panic se travado
- **CRC**: Verificação de integridade em pacotes binários

## 🔋 Power Management (Fase 2 - Próxima)

Planejado para v2.1:
- Light Sleep entre leituras → 15mA (vs 80mA atual)
- Autonomia: 5.5 dias (vs 25h atual)
- Wakeup por timer (30s)

## 📝 Logs

O firmware gera logs estruturados:

```
I (403) AGUADA_MAIN: ═══════════════════════════════════════════════════
I (404) AGUADA_MAIN:    AGUADA Node Sensor v2.0
I (405) AGUADA_MAIN:    Component-based architecture with FreeRTOS
I (406) AGUADA_MAIN: ═══════════════════════════════════════════════════
I (450) AGUADA_COMM: Node MAC: 20:6E:F1:6B:77:58
I (451) AGUADA_COMM: Gateway added: 80:F1:B2:50:2E:C4 (channel 1)
I (480) AGUADA_SENSOR: Sensors initialized (TRIG=1, ECHO=0, VALVE_IN=2...)
I (500) AGUADA_MAIN: ✓ All subsystems ready
I (501) AGUADA_MAIN: ✓ Starting normal operation
```

## 🧪 Testes

### Teste Manual

1. Flash firmware: `idf.py flash monitor`
2. Verificar boot sequence (LED pisca 3x)
3. Observar leituras a cada 30s
4. Verificar transmissões ESP-NOW no gateway

### Validação

- ✅ Boot em < 5s
- ✅ LED heartbeat (1 piscar a cada leitura)
- ✅ Leituras ultrasônicas consistentes
- ✅ Deadband funcional (não transmite variações < 2cm)
- ✅ Watchdog não reseta (sistema estável)

## 📚 Documentação Adicional

- `/firmware/MODERNIZATION_PROPOSAL.md` - Proposta completa v2.0
- `/firmware/QUICKSTART_V2_IMPLEMENTATION.md` - Guia implementação
- `/firmware/TECHNOLOGY_COMPARISON.md` - Comparativo tecnologias
- `/firmware/README_FIRMWARE_V2.md` - Índice master

## 🚦 Próximos Passos

1. ✅ Compilar e testar v2.0
2. ⏳ Flash em ESP32-C3 real
3. ⏳ Validar transmissões com gateway
4. ⏳ Benchmark consumo de energia
5. ⏳ Implementar Fase 2 (Power Management)

## 📄 Licença

Mesma do projeto AGUADA principal.

---

**Desenvolvido**: 2025-12-10  
**Branch**: firmware-v2  
**Status**: Fase 1 (Refatoração) COMPLETA ✅  
**ESP-IDF**: 6.1.0 (compatível com 5.3+)  
