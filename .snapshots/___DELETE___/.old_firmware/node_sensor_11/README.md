# AGUADA v1.1 - Node Sensor 11

## Protocolo AGUADA-1

Firmware universal para sensores ultrassônicos ESP32-C3 + AJ-SR04M.

### Formato do Pacote JSON

```json
{
  "mac": "80:F1:B2:50:31:34",
  "distance_mm": 2450,
  "vcc_bat_mv": 5000,
  "rssi": -50
}
```

### Campos

| Campo | Tipo | Unidade | Descrição |
|-------|------|---------|-----------|
| `mac` | string | - | MAC address do node |
| `distance_mm` | int32 | mm | Distância medida (0=timeout, 1=out-of-range) |
| `vcc_bat_mv` | int32 | mV | Tensão de alimentação |
| `rssi` | int32 | dBm | Intensidade do sinal |

### Lógica de Envio

1. **Delta**: Envia quando distância muda ±20mm ou tensão muda ±100mV
2. **Heartbeat**: Envia a cada 30 segundos mesmo sem mudança
3. **Primeira leitura**: Sempre envia após boot

### Configuração

| Parâmetro | Valor | Descrição |
|-----------|-------|-----------|
| `READ_INTERVAL_MS` | 2000 | Leitura a cada 2 segundos |
| `HEARTBEAT_MS` | 30000 | Heartbeat a cada 30 segundos |
| `SAMPLES_PER_READ` | 11 | Amostras para mediana |
| `DELTA_DISTANCE_MM` | 20 | Variação mínima (2cm) |
| `ESPNOW_CHANNEL` | 11 | Canal WiFi/ESP-NOW |

### Pinout ESP32-C3 SuperMini

| GPIO | Direção | Função | Conectar a |
|------|---------|--------|------------|
| GPIO 0 | INPUT | ECHO | AJ-SR04M pino ECHO |
| GPIO 1 | OUTPUT | TRIG | AJ-SR04M pino TRIG |
| GPIO 4 | INPUT (ADC) | VCC Monitor | Divisor de tensão (ponto médio) |
| GPIO 8 | OUTPUT | LED Status | LED + 330Ω → GND |
| 5V | POWER | Alimentação | AJ-SR04M pino VCC + Divisor VCC |
| GND | POWER | Terra | AJ-SR04M pino GND + Divisor GND |

### Diagrama de Conexões

```
┌─────────────────────────────────────────────────────────────────┐
│                    CONEXÕES ESP32-C3 SuperMini                  │
└─────────────────────────────────────────────────────────────────┘

  ESP32-C3                    AJ-SR04M
 ┌─────────┐                 ┌─────────┐
 │         │                 │         │
 │  GPIO1 ─┼─────────────────┼─ TRIG   │
 │  GPIO0 ─┼─────────────────┼─ ECHO   │
 │    5V ──┼─────────────────┼─ VCC    │
 │   GND ──┼─────────────────┼─ GND    │
 │         │                 └─────────┘
 │         │
 │         │     Divisor de Tensão VCC
 │         │    ┌───────────────────────┐
 │         │    │                       │
 │  GPIO4 ─┼────┼── PONTO MÉDIO ◄──────┐│
 │         │    │                      ││
 │         │    │  5V ──[10kΩ]──┬──[10kΩ]── GND
 │         │    │               │       │
 │         │    │               └───────┘
 │         │    └───────────────────────┘
 │         │
 │  GPIO8 ─┼── LED ──[330Ω]── GND
 │         │
 └─────────┘
```

### Divisor de Tensão para Monitoramento VCC

**Componentes:**
- R1: 10kΩ (entre 5V e GPIO4)
- R2: 10kΩ (entre GPIO4 e GND)

**Funcionamento:**
- Razão: 2:1 (5V → 2.5V no ADC)
- Fórmula: `VCC = ADC_leitura_mV × 2`
- Range ADC: 0-3.3V (com atenuação 12dB)

**Exemplo:**
- VCC = 5000mV → ADC lê 2500mV → Firmware calcula 2500 × 2 = 5000mV ✓
- VCC = 4200mV (bateria) → ADC lê 2100mV → Firmware calcula 4200mV ✓

> ⚠️ **IMPORTANTE**: Sem o divisor conectado, o ADC lê valores baixos (~800mV)
> e o firmware exibe warning "VCC fora do range".

### Build & Flash

```bash
cd firmware/node_sensor_11
source ~/esp/esp-idf/export.sh
idf.py set-target esp32c3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

### Métricas

O firmware exibe estatísticas a cada 10 envios:

```
📊 Stats: TX=100 OK=98 FAIL=2 Delta=45 HB=53
```

- `TX`: Total de leituras
- `OK`: Pacotes enviados com sucesso
- `FAIL`: Pacotes com falha
- `Delta`: Envios por mudança
- `HB`: Envios por heartbeat

### Códigos de Erro

| distance_mm | Significado |
|-------------|-------------|
| 0 | Timeout (sensor não respondeu) |
| 1 | Fora de range (< 20mm ou > 4500mm) |
| > 0 | Valor válido em mm |

## Changelog

### v1.1.0
- Protocolo AGUADA-1 padronizado
- Distância em mm (não mais cm×100)
- Campo `vcc_bat_mv` para tensão
- Mediana de 11 amostras
- Leitura a cada 2s, envio apenas delta/heartbeat
- Heartbeat configurável (30s default)
- Métricas detalhadas

### v1.0.0
- Versão inicial (node_sensor_10)
- Distância em cm×100
- Campos valve_in, valve_out, sound_in (removidos em v1.1)
