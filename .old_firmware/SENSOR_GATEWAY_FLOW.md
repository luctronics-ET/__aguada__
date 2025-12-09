# AGUADA - Fluxo de Telemetria Completo

## 📡 Arquitetura de Comunicação

```
┌──────────────────┐               ┌──────────────────┐               ┌──────────────────┐
│  Sensor Node     │               │   Gateway WiFi   │               │   Backend API    │
│  (node_sensor_10)│               │  (gateway_00)    │               │  (Node.js)       │
├──────────────────┤               ├──────────────────┤               ├──────────────────┤
│ ESP32-C3         │               │ ESP32-C3         │               │ Express          │
│ + AJ-SR04M       │    ESP-NOW    │ + WiFi STA       │     MQTT      │ + PostgreSQL     │
│                  │ ────────────> │                  │ ────────────> │ + TimescaleDB    │
│ Lê sensor        │   JSON 250B   │ Recebe           │   JSON        │ Armazena         │
│ a cada 30s       │               │ Publica MQTT     │               │ Comprime >90%    │
└──────────────────┘               └──────────────────┘               └──────────────────┘
```

## 🔧 Componentes Implementados

### ✅ Sensor Node (node_sensor_10)
- **Firmware**: ESP-IDF 6.x nativo (C)
- **Hardware**: ESP32-C3 SuperMini + AJ-SR04M
- **Função**: Ler sensor ultrassônico e enviar via ESP-NOW
- **Status**: ✅ Compilado (657KB, 37% free)

**Características:**
- Leitura a cada 2 segundos
- Filtro de mediana (5 amostras)
- Envio JSON via ESP-NOW
- 3 tentativas automáticas COM MESMO PACOTE
- LEDs de status e transmissão (3 PISCADAS DE 1S A CADA HERTBEAT)

**Formato JSON enviado:**
```json
{
  "mac":"AA:BB:CC:DD:EE:FF",
  "data.label": "distance_mm",              //distancia em mm
  "data.value": 2450,                      //distancia em mm
  "VccBatt_mv": 4900,                      //Tensao Batt ou Vin em mV
  "rssi":-50                               // intensidade enlace em db

}
```

**Pinout:**
- GPIO 1: TRIG (AJ-SR04M)
- GPIO 0: ECHO (AJ-SR04M)
- GPIO 8: LED Status


### ✅ Gateway WiFi (gateway_00_arduino)
- **Firmware**: Arduino (PlatformIO/Arduino IDE)
- **Hardware**: ESP32-C3 SuperMini
- **Função**: Receber ESP-NOW e publicar MQTT
- **Status**: ✅ Testado e funcionando

**Características:**
- WiFi conectado à rede "luciano"
- Recebe ESP-NOW na mesma frequência (Canal 11)
- Publica no MQTT broker (192.168.0.117:1883)
- Tópico: `aguada/telemetry`
- Heartbeat LED 1Hz

**Configuração Atual:**
```cpp
WIFI_SSID = "luciano"
WIFI_PASSWORD = "19852012"
MQTT_BROKER = "192.168.0.117"
MQTT_PORT = 1883
MQTT_TOPIC = "aguada/telemetry"
```

### ⏳ Backend API (Node.js)
- **Status**: Código pronto, aguardando deploy
- **Endpoints**:
  - POST /api/telemetry - Recebe dados via MQTT
  - POST /api/manual-reading - Leitura manual
  - POST /api/calibration - Calibração de sensores

**Serviços:**
- Compressão deadband (>90% redução)
- Detecção de eventos (abastecimento, vazamento, nível crítico)
- Cálculo de volume
- Armazenamento TimescaleDB

## 📝 Próximos Passos

### 1. Gravar Firmware no Sensor Node

```bash
cd /home/luciano/Área\ de\ trabalho/bms.aguada/firmware/node_sensor_11

# Verificar porta USB
ls /dev/ttyACM* /dev/ttyUSB*

# Gravar firmware
. $HOME/esp/esp-idf/export.sh
idf.py -p /dev/ttyACM0 flash monitor
```

### 2. Ajustar MAC do Gateway

No arquivo `node_sensor_10/main/config.h`, linha 24:
```c
// Substitua pelo MAC real do gateway (visto no monitor serial)
static uint8_t gateway_mac[6] = {0x80, 0xf1, 0xb2, 0x50, 0x2e, 0xc4};
```

### 3. Iniciar MQTT Broker

```bash
# Opção 1: Mosquitto local
sudo systemctl start mosquitto
mosquitto_sub -h localhost -t "aguada/#" -v

# Opção 2: Docker
docker run -d -p 1883:1883 -p 9001:9001 eclipse-mosquitto
```

### 4. Iniciar Backend API

```bash
cd backend
npm install
cp .env.example .env
# Editar .env com credenciais do PostgreSQL
npm start
```

### 5. Testar Fluxo Completo

**Terminal 1 - Monitor do Sensor:**
```bash
cd firmware/node_sensor_10
idf.py -p /dev/ttyACM0 monitor
```

**Terminal 2 - Monitor do Gateway:**
```bash
# Arduino Serial Monitor ou
screen /dev/ttyACM1 115200
```

**Terminal 3 - MQTT Subscriber:**
```bash
mosquitto_sub -h 192.168.0.117 -t "aguada/#" -v
```

**Você deve ver:**
1. Sensor lê ultrassônico a cada 30s
2. Envia JSON via ESP-NOW
3. Gateway recebe e publica no MQTT
4. Backend processa e armazena no PostgreSQL

## 🔍 Troubleshooting

### ESP-NOW não conecta

**Problema:** Sensor envia mas gateway não recebe.

**Solução:**
1. Verificar MAC address do gateway (está correto em config.h?)
2. Ambos devem usar o mesmo canal WiFi (atualmente Canal 1)
3. Distância máxima ~100m em campo aberto, ~30m indoor

```bash
# No gateway, verificar MAC:
# Aparece no monitor serial ao iniciar
```

### MQTT não publica

**Problema:** Gateway recebe ESP-NOW mas não publica MQTT.

**Solução:**
1. Verificar se broker está rodando: `telnet 192.168.0.117 1883`
2. Checar credenciais WiFi no gateway
3. Ver logs do gateway no Serial Monitor

### Sensor lê valores errados

**Problema:** Leituras inconsistentes.

**Solução:**
1. Verificar conexões TRIG/ECHO do AJ-SR04M
2. Sensor precisa de alimentação 5V estável
3. Ajustar constantes em config.h:
   - `RESERVOIR_HEIGHT_CM`
   - `SENSOR_OFFSET_CM`

## 📊 Dados de Telemetria

### JSON do ESP-NOW (Sensor → Gateway)
```json
{
  "mac": "A0:B1:C2:D3:E4:F5",
  "type": "distancia_mm",
  "value": 245,
  "rssi": -55
}
```

### JSON do MQTT (Gateway → Backend)
```json
{
  "mac": "A0:B1:C2:D3:E4:F5",
  "type": "nivel_cm",
  "value": 245,
  "rssi": -55
}
```

### JSON armazenado no PostgreSQL
```sql
INSERT INTO leituras_raw (
    sensor_id,
    datetime,
    nivel_cm,
    volume_percent,
    rssi
) VALUES (
    'SEN_CON_01',
    NOW(),
    245.0,
    85.5,
    -55
);
```

## 🎯 Performance Esperada

- **Taxa de leitura**: 30 segundos
- **Latência ESP-NOW**: <10ms
- **Latência MQTT**: ~50ms
- **Compressão no backend**: >90% redução de dados
- **Bateria estimada**: ~1 semana (com deep sleep, a implementar)

## ✅ Status do Sistema

| Componente | Status | Observações |
|------------|--------|-------------|
| Firmware Sensor | ✅ Compilado | Pronto para gravar |
| Firmware Gateway | ✅ Rodando | WiFi conectado |
| ESP-NOW | ⏳ Aguardando teste | Precisa gravar sensor |
| MQTT Broker | ⏳ Aguardando deploy | Instalar Mosquitto |
| Backend API | ✅ Código pronto | Precisa iniciar |
| PostgreSQL | ⏳ Aguardando deploy | Executar schema.sql |
| Grafana | ⏳ Aguardando deploy | Docker compose |

## 🚀 Deploy Rápido

```bash
# 1. MQTT Broker
sudo apt install mosquitto mosquitto-clients
sudo systemctl start mosquitto

# 2. PostgreSQL + TimescaleDB
cd docker
docker-compose up -d postgres

# 3. Executar schema
psql -h localhost -U postgres -d aguada_db -f database/schema.sql

# 4. Backend API
cd backend
npm install && npm start

# 5. Gravar sensores
cd firmware/node_sensor_10
idf.py -p /dev/ttyACM0 flash

# 6. Testar
mosquitto_sub -h localhost -t "aguada/#" -v
```

---

**Sistema AGUADA v1.0** - Monitoramento Hidráulico IoT
