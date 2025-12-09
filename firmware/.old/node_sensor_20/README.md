# AGUADA Node Sensor 20 - Dual Ultrasonic

Firmware ESP32-C3 para monitoramento simultâneo de **2 reservatórios** (IE01 e IE02) com um único microcontrolador.

## 📋 Especificações

### Tipo de Firmware
**NODE_DUAL_ULTRA** - Dual Ultrasonic Sensor Node

### Reservatórios Monitorados
- **IE01** - Cisterna Ilha do Engenho 01 (254m³)
- **IE02** - Cisterna Ilha do Engenho 02 (254m³)

### Hardware
- **Microcontrolador**: ESP32-C3 SuperMini
- **Sensores**: 2x AJ-SR04M (ultrassônico)
- **Detectores de som**: 2x GPIO digital
- **Válvulas**: 2x GPIO digital (1 por reservatório)
- **LED**: 1x heartbeat status

## 🔌 Pinagem GPIO

### IE01 (Cisterna 01)
| GPIO | Função | Descrição |
|------|--------|-----------|
| 0 | TRIG | Trigger ultrassônico IE01 |
| 1 | ECHO | Echo ultrassônico IE01 |



### IE02 (Cisterna 02)
| GPIO | Função | Descrição |
|------|--------|-----------|
| 3 | TRIG | Trigger ultrassônico IE02 |
| 2 | ECHO | Echo ultrassônico IE02 |

### Sistema
| GPIO | Função | Descrição |
|------|--------|-----------|
| 8 | LED_STATUS | LED de heartbeat (onboard) |


### ESP-NOW
- **Gateway MAC**: `80:f1:b2:50:2e:c4`
- **Canal WiFi**: 1 (2.4GHz)
- **Criptografia**: Desabilitada (simplificação)

### Formato de Dados (Individual)

Cada variável é enviada em um pacote JSON separado:

```json
// Distância IE01
{"mac":"XX:XX:XX:XX:XX:XX","type":"distance_cm","value":25480,"battery":5000,"uptime":120,"rssi":-50}

// Distância2 (opcional)
{"mac":"XX:XX:XX:XX:XX:XX","type":"distance2_cm","value":18350,"battery":5000,"uptime":120,"rssi":-50}


### Tipos de Variáveis

| Type          | Descrição       | Unidade | Valores |
|---------------|-----------------|---------|---------|
| `distance_cm` | Distância IE01  | cm × 100 | 2000-45000 (2-500cm) |
| `distance2_cm`| Distância IE02  | cm × 100 | 2000-45000 (2-500cm) |
| 

### Valores Especiais (distance_cm)
- `0` - Timeout (sensor não respondeu)
- `1` - Fora do range (< 2 cm ou > 500cm)

## ⚙️ Configuração

### Filtros e Deadband
```c
#define SAMPLES_FOR_MEDIAN  5      // 5 amostras
#define SAMPLE_INTERVAL_MS  2000     // 2000 ms (2s) entre amostras
#define DEADBAND_CM         2       // ±2cm threshold
```

### Heartbeat
```c
#define HEARTBEAT_INTERVAL_MS   60000   // 60 segundos
#define BLINK_INTERVAL_MS       60000    // LED pisca a cada 60s
```

## 🚀 Build e Flash

### Pré-requisitos
- ESP-IDF 6.x
- Python 3.8+
- USB-C cable

### Compilar
```bash
cd firmware/node_sensor_20
idf.py set-target esp32c3
idf.py build
```

### Gravar
```bash
# Descobrir porta USB
ls -la /dev/ttyACM*

# Flash
idf.py -p /dev/ttyACM0 flash

# Monitorar serial
idf.py -p /dev/ttyACM0 monitor

# Flash + Monitor (combinado)
idf.py -p /dev/ttyACM0 flash monitor
```

### Saída Esperada
```
I (403) AGUADA_NODE20: === AGUADA NODE 20 - DUAL ULTRASONIC (IE01 + IE02) ===
I (404) AGUADA_NODE20: Firmware: TYPE_DUAL_ULTRA
I (409) AGUADA_NODE20: ESP-IDF: v6.1.0
I (414) AGUADA_NODE20: GPIO inicializado (IE01: trig=0 echo=1 | IE02: trig=2 echo=3)
I (1752) AGUADA_NODE20: Node MAC: dc:06:75:67:6a:dd
I (1753) AGUADA_NODE20: ESP-NOW OK - Gateway: 80:F1:B2:50:2E:C4
I (1758) AGUADA_NODE20: Telemetry task iniciada
I (4162) AGUADA_NODE20: IE01: 254.80 cm (11 amostras)
I (6523) AGUADA_NODE20: IE02: 183.50 cm (11 amostras)
I (6524) AGUADA_NODE20: → {"mac":"dc:06:75:67:6a:dd","type":"distance_cm","value":25480,...}
I (6534) AGUADA_NODE20: → {"mac":"dc:06:75:67:6a:dd","type":"IE02_distance_cm","value":18350,...}
I (6584) AGUADA_NODE20: → {"mac":"dc:06:75:67:6a:dd","type":"IE01_sound_in","value":0,...}
I (6594) AGUADA_NODE20: → {"mac":"dc:06:75:67:6a:dd","type":"IE02_sound_in","value":0,...}
```

## 🐛 Troubleshooting

### Problema: "Poucas amostras válidas"
**Causa**: Sensor ultrassônico retornando timeouts

**Soluções**:
1. Verificar conexões GPIO (TRIG e ECHO)
2. Verificar alimentação do sensor (5V)
3. Verificar obstáculos no caminho do ultrassom
4. Testar cada sensor individualmente

### Problema: "ESP-NOW send error"
**Causa**: Gateway fora de alcance ou offline

**Soluções**:
1. Verificar se gateway está ligado
2. Reduzir distância (< 250m)
3. Verificar MAC do gateway em `config.h`
4. Verificar canal WiFi (deve ser 1)

### Problema: Valores de válvulas sempre 0
**Causa**: Pull-down ativo (comportamento normal sem válvulas conectadas)

**Solução**: Conectar válvulas reais aos GPIOs 7, 8, 9, 10

## 📊 Estatísticas

### Uso de Memória
```
RAM total: 400 KB
RAM usada: ~55 KB (13.75%)
Flash usada: ~760 KB (28%)
```

### Performance
- **Tempo de leitura**: ~2.2s por sensor (11 amostras × 200ms)
- **Ciclo completo**: ~5s (2 sensores + envio de 8 variáveis)
- **Taxa de transmissão**: 8 pacotes ESP-NOW a cada 30s
- **Latência ESP-NOW**: < 10ms

## 📝 Notas Técnicas

### Diferenças vs node_sensor_10
| Característica | node_sensor_10 | node_sensor_20 |
|----------------|----------------|----------------|
| Reservatórios | 1 (RCON, RCAV ou RB03) | 2 (IE01 + IE02) |
| Sensores ultrassônicos | 1 | 2 |
| GPIOs usados | 6 | 11 |
| Variáveis enviadas | 4 | 8 |
| Prefixo no type | Nenhum | IE01_ ou IE02_ |
| Firmware | TYPE_SINGLE_ULTRA | TYPE_DUAL_ULTRA |

### Cálculo de Volume
O backend calcula o volume usando as dimensões das cisternas IE:
- **Comprimento**: 1040 cm
- **Largura**: 407 cm
- **Altura máxima**: 600 cm
- **Volume total**: 254m³ por cisterna

Fórmula: `Volume (m³) = (Comp × Larg × Altura_água) / 1.000.000`

## 🔒 Segurança

- ✅ ESP-NOW sem criptografia (rede local confiável)
- ✅ Pull-down em todos os GPIOs de entrada
- ✅ Validação de range (20-450cm)
- ✅ Timeout em leituras (30ms)
- ✅ Proteção contra ruído (mediana de 11 amostras)

## 📄 Licença

MIT License - Ver [LICENSE](../../LICENSE)

## 👥 Autor

Equipe AGUADA - 2025
