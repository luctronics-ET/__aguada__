# AGUADA Node Sensor 21 - Dual Ultrasonic

Firmware para ESP32-C3 SuperMini com **2 sensores ultrassônicos** - monitora as cisternas IE01 e IE02 simultaneamente.

## Características

- **2 sensores AJ-SR04M** no mesmo ESP32
- **2 MACs diferentes** para identificação:
  - IE01: MAC real do ESP32
  - IE02: MAC virtual `AA:BB:CC:DD:IE:02`
- **Protocolo AGUADA-1** compatível com gateway
- Leitura alternada dos sensores
- Filtro mediana (5 amostras)

## Pinout

| Função | GPIO | Sensor |
|--------|------|--------|
| TRIG1 | 1 | IE01 |
| ECHO1 | 0 | IE01 |
| TRIG2 | 3 | IE02 |
| ECHO2 | 2 | IE02 |
| VCC_ADC | 4 | Tensão bateria |
| LED | 8 | Status |

## Compilação

```bash
cd firmware/node_sensor_21
idf.py set-target esp32c3
idf.py build
```

## Flash

```bash
idf.py -p /dev/ttyACM0 -b 460800 flash monitor
```

## Protocolo

Cada sensor envia pacotes independentes:

```json
// IE01 (MAC real)
{"mac":"XX:XX:XX:XX:XX:XX","type":"distance_mm","value":1850,"vcc_bat_mv":4200,"rssi":-45}

// IE02 (MAC virtual)
{"mac":"AA:BB:CC:DD:IE:02","type":"distance_mm","value":2100,"vcc_bat_mv":4200,"rssi":-45}
```

## Versão

- **Firmware**: v1.0.0
- **ESP-IDF**: 6.1.0
- **Baseado em**: node_sensor_11 v1.1.0
