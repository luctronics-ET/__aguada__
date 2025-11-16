# AGUADA Gateway WiFi - ESP32-C3 SuperMini

Gateway central para receber telemetria de múltiplos nodes e encaminhar para o servidor MQTT/HTTP.

## 🎯 Função

O gateway atua como **concentrador de telemetria**, recebendo dados de vários nodes ESP32 e gerenciando o envio para o backend com:

- ✅ Fila de mensagens com retry automático
- ✅ Registro de nodes online/offline
- ✅ Envio via MQTT (QoS 1) + HTTP fallback
- ✅ Status periódico do gateway
- ✅ Estatísticas em tempo real

## 📐 Hardware

### ESP32-C3 SuperMini (Gateway)

```
GPIO8  (LED)  -----> LED onboard (heartbeat)
GPIO10 (LED)  -----> LED WiFi status
GPIO2  (LED)  -----> LED MQTT status
GPIO20 (RX)   -----> Comunicação Serial (opcional)
GPIO21 (TX)   -----> Comunicação Serial (opcional)
```

## 🔧 Configuração

Edite `main/config_pins.h`:

```cpp
// WiFi
#define WIFI_SSID         "SUA_REDE"
#define WIFI_PASSWORD     "SUA_SENHA"

// MQTT
#define MQTT_BROKER       "192.168.1.100"
#define MQTT_PORT         1883
#define MQTT_USER         "aguada_node"
#define MQTT_PASS         "mqtt_pass"

// HTTP Fallback
#define HTTP_SERVER       "http://192.168.1.100:3000"

// Gateway Settings
#define MAX_NODES         10
#define QUEUE_SIZE        50
#define RETRY_ATTEMPTS    3
```

## 📦 Arquitetura

```
┌──────────┐
│ Node 01  │──┐
└──────────┘  │
              │     ┌─────────────┐
┌──────────┐  │     │             │
│ Node 03  │──┼────→│  Gateway 00 │──MQTT──→ Broker ──→ Backend
└──────────┘  │     │  (ESP32-C3) │                       API
              │     └─────────────┘
┌──────────┐  │            │
│ Node 04  │──┘            └──HTTP fallback──→ Backend API
└──────────┘
```

## 🚀 Componentes

### 1. QueueManager
Gerencia fila de mensagens com:
- Fila FIFO de até 50 mensagens
- Mensagens prioritárias (inserção no início)
- Retry automático (até 3 tentativas)
- Estatísticas (recebidas/enviadas/descartadas)

### 2. NodeRegistry
Registra e monitora nodes:
- Auto-registro de nodes desconhecidos
- Detecção de nodes offline (>5 min sem msg)
- Estatísticas por node (msgs, RSSI, last seen)
- Listagem de todos os nodes

### 3. GatewayIO
Gerencia comunicação com servidor:
- Envio via MQTT (preferencial)
- HTTP fallback automático
- Status periódico do gateway (60s)
- Processamento da fila

## 📡 Protocolo de Comunicação

### Recepção de Telemetria (Serial)

Nodes enviam JSON via Serial para o gateway:

```json
{
  "node_mac": "AA:BB:CC:DD:EE:01",
  "datetime": "2025-11-16T14:30:00Z",
  "data": [
    { "label": "nivel_cm", "value": 245.5, "unit": "cm" }
  ],
  "meta": {
    "battery": 3.8,
    "rssi": -65,
    "uptime": 3600
  }
}
```

### Envio para Servidor (MQTT)

Gateway publica em: `aguada/telemetry/gateway`

### Status do Gateway (MQTT)

Publicado em: `aguada/status` (a cada 60s)

```json
{
  "gateway": "gateway_00",
  "status": "online",
  "uptime": 3600,
  "queue_size": 5,
  "msgs_sent": 142,
  "msgs_dropped": 2
}
```

## 🔄 Fluxo de Processamento

```
1. Node envia telemetria via Serial
   ↓
2. Gateway recebe e valida JSON
   ↓
3. Atualiza registro do node (last_seen, RSSI)
   ↓
4. Enfileira mensagem
   ↓
5. Processa fila (a cada 1s)
   ↓
6. Tenta enviar via MQTT
   ↓
7. Se falhar, usa HTTP fallback
   ↓
8. Se falhar novamente, retry (até 3x)
   ↓
9. Após 3 falhas, descarta
```

## 📊 Estatísticas

### Comando Serial: `stats`

```
╔═══════════════════════════════════════════════════════════╗
║           AGUADA GATEWAY - ESTATÍSTICAS                  ║
╠═══════════════════════════════════════════════════════════╣
║ Uptime:          3600 segundos
║ WiFi:            CONECTADO (RSSI: -65 dBm)
║ MQTT:            CONECTADO
╠═══════════════════════════════════════════════════════════╣
║ Fila:            5 mensagens
║ Recebidas:       142
║ Enviadas:        137
║ Descartadas:     2
╠═══════════════════════════════════════════════════════════╣
║ Nodes Total:     3
║ Nodes Online:    3
╚═══════════════════════════════════════════════════════════╝
```

### Comando Serial: `nodes`

```
[REGISTRY] ==================== NODES REGISTRADOS ====================
Total: 3 | Online: 3
MAC Address       | Name     | Sensor      | Msgs  | RSSI | Status
----------------------------------------------------------------
AA:BB:CC:DD:EE:01 | node_01  | SEN_CAV_01  | 48    | -62  | ONLINE (25s)
AA:BB:CC:DD:EE:03 | node_03  | SEN_IE01_01 | 46    | -58  | ONLINE (12s)
AA:BB:CC:DD:EE:04 | node_04  | SEN_CON_01  | 48    | -65  | ONLINE (8s)
================================================================
```

## 🔍 Comandos Serial (Debug)

```bash
stats       # Mostra estatísticas
nodes       # Lista nodes registrados
clear       # Limpa fila de mensagens
restart     # Reinicia gateway
{...}       # Envia JSON de telemetria manualmente
```

## 🚀 Compilação e Flash

```bash
# Configurar target
idf.py set-target esp32c3

# Compilar
idf.py build

# Flash
idf.py -p /dev/ttyACM0 flash

# Monitor
idf.py -p /dev/ttyACM0 monitor
```

## 🔧 Testes

### 1. Teste de Conectividade

```bash
# Conectar ao Serial Monitor
idf.py -p /dev/ttyACM0 monitor

# Verificar logs de conexão WiFi/MQTT
```

### 2. Teste de Envio Manual

```bash
# No Serial Monitor, colar JSON de teste:
{"node_mac":"AA:BB:CC:DD:EE:99","datetime":"2025-11-16T14:30:00Z","data":[{"label":"nivel_cm","value":250.0,"unit":"cm"}],"meta":{"battery":3.8,"rssi":-65}}
```

### 3. Verificar MQTT

```bash
# Subscriber MQTT
mosquitto_sub -h 192.168.1.100 -t "aguada/#" -v
```

## 💡 LEDs de Status

| LED | GPIO | Significado |
|-----|------|-------------|
| Builtin | 8 | Heartbeat (1 Hz) |
| WiFi | 10 | ON = WiFi conectado |
| MQTT | 2 | ON = MQTT conectado |

## 🔒 Segurança

- WiFi WPA2
- MQTT com autenticação
- Validação de JSON
- Timeout de conexão (5s)
- Watchdog timer (60s)

## 📈 Performance

- **Throughput**: Até 100 msgs/minuto
- **Latência**: <1s (fila vazia)
- **Queue capacity**: 50 mensagens
- **Retry policy**: 3 tentativas
- **Node timeout**: 5 minutos

## 📚 Dependências

- ESP-IDF 5.x
- Arduino as Component
- ArduinoJson 6.x
- PubSubClient 2.8

## 🛠️ Troubleshooting

### Gateway não conecta WiFi
- Verificar SSID/senha em `config_pins.h`
- Verificar alcance do sinal WiFi

### MQTT não conecta
- Verificar broker rodando: `systemctl status mosquitto`
- Verificar IP/porta/credenciais

### Mensagens não chegam ao servidor
- Verificar logs do gateway: `stats`
- Verificar fila: se cheia, aumentar `QUEUE_SIZE`
- Verificar backend rodando

### Node aparece OFFLINE
- Verificar se node está enviando telemetria
- Timeout padrão: 5 minutos

## 📝 TODO

- [ ] Suporte BLE para receber de nodes BLE
- [ ] Interface web de configuração
- [ ] OTA (Over-The-Air) updates
- [ ] Criptografia de payload
- [ ] Banco de dados local (cache)

## 📚 Referências

- [ESP32-C3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf)
- [AGUADA RULES.md](../../docs/RULES.md)
- [Backend API](../../backend/README.md)
