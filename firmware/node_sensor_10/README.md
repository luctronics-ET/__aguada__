# AGUADA Firmware - ESP32-C3 SuperMini

Firmware para nodes de telemetria do sistema AGUADA de monitoramento hidráulico.

## 🎯 Especificações

- **MCU**: ESP32-C3 SuperMini
- **Framework**: ESP-IDF 5.x + Arduino as Component
- **Sensor**: AJ-SR04M (Ultrassônico à prova d'água)
- **Comunicação**: MQTT (QoS 1) + HTTP fallback
- **Intervalo de leitura**: 10 segundos
- **Intervalo de telemetria**: 30 segundos
- **Filtro**: Mediana de 11 amostras

## 📐 Hardware

### Pinout ESP32-C3 SuperMini

```
GPIO1 (Trig) -----> AJ-SR04M Trigger
GPIO0 (Echo) -----> AJ-SR04M Echo
GPIO8 (LED)  -----> LED onboard (heartbeat)
```

### Sensor AJ-SR04M

- **Range**: 20cm - 450cm
- **Precisão**: ±0.5cm
- **Alimentação**: 5V
- **Timeout**: 30ms
- **Velocidade do som**: 343 m/s (0.0343 cm/µs)

## 📦 Dependências

### Arduino Libraries (via platformio.ini ou Arduino IDE)

```ini
lib_deps = 
    bblanchon/ArduinoJson@^6.21.3
    knolleary/PubSubClient@^2.8
```

### ESP-IDF Components

- arduino (como component)
- esp_wifi
- esp_http_client
- nvs_flash

## 🔧 Configuração

Edite `main/config_pins.h`:

```cpp
// WiFi
#define WIFI_SSID         "SUA_REDE"
#define WIFI_PASSWORD     "SUA_SENHA"

// MQTT
#define MQTT_BROKER       "192.168.1.100"
#define MQTT_PORT         1883
#define MQTT_USER         "usuario"
#define MQTT_PASS         "senha"

// HTTP Fallback
#define HTTP_SERVER       "http://192.168.1.100:3000"

// Node ID
#define NODE_NAME         "node_10"
#define SENSOR_ID         "SEN_CON_01"
#define ELEMENTO_ID       "reservatorio_agua"

// Parâmetros do Reservatório
#define SENSOR_OFFSET_CM       20.0f       // distancia do sensor ao nivel max (cm)
#define RESERVOIR_MAX_LEVEL_CM    450.0f   // distancia do nivel/altura maxima da agua (cm) | volume = 100%
#define RESERVOIR_DIAMETER_CM  500.0f      
#define RESERVOIR_VOLUME_MAX_M3   80.0f     // VOLUME MAXIMO. AJUSTAR COM FATOR DE CORRECAO PARA QUE O VOLUME DE MAX_LEVEL_CM X AREA DA BASE = RESERVOIR_VOLUME_MAX_M3   80.0f 
```

## 🚀 Compilação e Flash

### Usando ESP-IDF

```bash
# Configurar ambiente ESP-IDF
. $HOME/esp/esp-idf/export.sh

# Configurar target
idf.py set-target esp32c3

# Configurar projeto (menuconfig)
idf.py menuconfig

# Compilar
idf.py build

# Flash
idf.py -p /dev/ttyACM0 flash

# Monitor serial
idf.py -p /dev/ttyACM0 monitor
```

### Usando VSCode + ESP-IDF Extension

1. Abrir pasta do projeto
2. Ctrl+Shift+P → "ESP-IDF: Set Espressif Device Target" → esp32c3
3. Ctrl+Shift+P → "ESP-IDF: Build Project"
4. Ctrl+Shift+P → "ESP-IDF: Flash Device"
5. Ctrl+Shift+P → "ESP-IDF: Monitor Device"

## 📊 Funcionamento

### 1. Inicialização

```
[SETUP] Inicializar sensor ultrassônico
[SETUP] Conectar WiFi
[SETUP] Conectar MQTT
[SETUP] Configurar Watchdog (120s)
```

### 2. Loop Principal

```
Cada 10s:  Ler sensor → Buffer circular (11 amostras)
Cada 30s:  Calcular mediana → Volume → Enviar telemetria
Cada 1s:   Heartbeat LED
Contínuo:  Manter WiFi/MQTT, Reset watchdog
```

### 3. Processamento de Dados

```
Leitura raw (cm) 
    ↓
Buffer circular [11 amostras]
    ↓
Mediana (filtro)
    ↓
Aplicar offset (40cm)
    ↓
Calcular volume (V = π*r²*h)
    ↓
Calcular percentual
    ↓
JSON payload
    ↓
MQTT (QoS 1) ou HTTP fallback
```

### 4. Detecção de Falhas

- **Timeout**: Sem resposta do sensor por >30ms
- **Out of range**: Leitura <20cm ou >450cm
- **Stuck sensor**: Sem leitura válida por >60s
- **WiFi lost**: Reconexão automática a cada 30s
- **Watchdog**: Reset automático se loop travado por >120s

## 📡 Formato de Telemetria

```json
{
  "node_mac": "AA:BB:CC:DD:EE:01",
  "datetime": "",
  "data": [
    {
      "label": "nivel_cm",
      "value": 245.5,
      "unit": "cm"
    }
  ],
  "meta": {
    "battery": 3.3,
    "rssi": -65,
    "uptime": 3600,
    "firmware_version": "1.0.0",
    "sensor_id": "SEN_CON_01",
    "elemento_id": "res_cons",
    "volume_m3": 63.254,
    "percentual": 72.5
  }
}
```

## 🔍 Debug

### Serial Monitor

```bash
# Baudrate: 115200
# Formato de log:

[ULTRA] Leitura: 245.50 cm (buffer: 11/11)
[TELEMETRY] Mediana: 245.20 cm (11 amostras)
[TELEMETRY] Volume: 63.254 m³ (72.5%)
[MQTT] ✓ Telemetria enviada
```

### LED Heartbeat

- **Piscar 1Hz**: Sistema operando normalmente
- **2 piscadas**: Telemetria enviada com sucesso
- **5 piscadas rápidas**: Erro ao enviar telemetria
- **3 piscadas (boot)**: Inicialização completa

## 📁 Estrutura de Arquivos

```
firmware_node10/
├── CMakeLists.txt              # Build system (ESP-IDF)
├── sdkconfig.defaults          # Configurações padrão
├── README.md                   # Este arquivo
└── main/
    ├── CMakeLists.txt          # Component build
    ├── config_pins.h           # Configurações de pins/WiFi/MQTT
    ├── main.cpp                # Loop principal
    ├── ultra.h/cpp             # Driver sensor ultrassônico
    ├── packet.h/cpp            # Construção de payload JSON
    ├── wifi.h/cpp              # WiFi manager
    ├── ios.h/cpp               # MQTT + HTTP service
    └── heartbeat.h/cpp         # LED heartbeat
```

## 🧪 Testes

### 1. Teste de Sensor

```cpp
// No setup(), adicionar:
for (int i = 0; i < 10; i++) {
  float dist = ultraSensor.readDistanceCm();
  Serial.printf("Leitura %d: %.2f cm\n", i, dist);
  delay(1000);
}
```

### 2. Teste de Conectividade

```bash
# Ping do ESP32
ping 192.168.YY.XXX       // YY: subrede    XXX: IP

# Verificar MQTT broker
mosquitto_sub -h 192.168.0.100 -t "aguada/telemetry/#" -v
```

### 3. Teste de HTTP Fallback

```bash
# Desligar MQTT broker e verificar logs
# Deve aparecer: [HTTP] Tentando fallback...
```

## 🔒 Segurança

- WiFi WPA2
- MQTT com autenticação
- Sem armazenamento de credenciais em plain text (usar nvs_flash em produção)
- Watchdog timer habilitado

## 🚀 Performance

- **Latência de leitura**: ~30ms
- **Tempo de envio MQTT**: <100ms
- **Consumo de memória**: ~80KB RAM
- **Uptime**: >30 dias contínuos

## 📚 Referências

- [ESP32-C3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf)
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/)
- [AJ-SR04M Manual](https://www.amazon.com/HiLetgo-Ultrasonic-Distance-Waterproof-Transducer/dp/B07TKVPPHV)
- [AGUADA RULES.md](../../RULES.md)
