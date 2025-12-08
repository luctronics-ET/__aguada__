# AGUADA Firmware Minimal

Versão mínima do firmware AGUADA com arquitetura **Node + Gateway USB**.

## 📁 Estrutura

```
firmware_MIN/
├── node_minimal/       # Firmware do sensor (ESP32-C3)
├── gateway_usb/        # Firmware do gateway USB (ESP32-C3 ou ESP32)
├── bridge_usb/         # Script Python para PC/Raspberry
└── README.md           # Este arquivo
```

## 🏗️ Arquitetura

```
┌─────────────┐ ┌─────────────┐ ┌─────────────┐
│   Node 1    │ │   Node 2    │ │   Node 3    │
│ ESP32-C3    │ │ ESP32-C3    │ │ ESP32-C3    │
│ Ultrasonic  │ │ Ultrasonic  │ │ Ultrasonic  │
└──────┬──────┘ └──────┬──────┘ └──────┬──────┘
       │ ESP-NOW       │               │
       └───────────────┼───────────────┘
                       ▼
              ┌─────────────────┐
              │  Gateway USB    │
              │  ESP32-C3/DK    │
              │  (Sem WiFi!)    │
              └────────┬────────┘
                       │ USB Serial
                       ▼
              ┌─────────────────┐
              │  PC/Raspberry   │
              │  bridge_usb.py  │
              └────────┬────────┘
                       │ HTTP POST
                       ▼
              ┌─────────────────┐
              │    Backend      │
              │ localhost:3000  │
              └─────────────────┘
```

## ⚡ Vantagens do Gateway USB

| Aspecto            | Gateway WiFi    | Gateway USB    |
| ------------------ | --------------- | -------------- |
| **Rede WiFi**      | Necessária      | ❌ Não precisa |
| **Configuração**   | SSID/senha      | Plug & Play    |
| **Confiabilidade** | Depende do WiFi | Cabo físico    |
| **Alimentação**    | Separada        | Via USB        |
| **Complexidade**   | HTTP client     | Apenas Serial  |

## 🚀 Início Rápido

### 1. Compilar e Gravar Node

```bash
cd node_minimal
idf.py set-target esp32c3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

### 2. Compilar e Gravar Gateway

```bash
cd gateway_usb
idf.py set-target esp32c3  # ou esp32
idf.py build
idf.py -p /dev/ttyACM0 flash
```

### 3. Executar Bridge no PC

```bash
cd bridge_usb
pip install -r requirements.txt
python bridge_usb.py --port /dev/ttyACM0 --backend http://localhost:3000
```

## 📡 Protocolo de Comunicação

### ESP-NOW (Node → Gateway)

```json
{
  "mac": "20:6E:F1:6B:77:58",
  "type": "distance_cm",
  "value": 24480,
  "uptime": 3600
}
```

### Serial/USB (Gateway → PC)

```json
{
  "mac": "20:6E:F1:6B:77:58",
  "type": "distance_cm",
  "value": 24480,
  "uptime": 3600,
  "rssi": -45
}
```

### HTTP (Bridge → Backend)

```bash
POST /api/telemetry
Content-Type: application/json

{
  "mac": "20:6E:F1:6B:77:58",
  "type": "distance_cm",
  "value": 24480,
  "uptime": 3600,
  "rssi": -45
}
```

## 📊 Lógica de Transmissão (Node)

O node envia dados quando:

1. **Primeira leitura** - Sempre envia no boot
2. **Variação significativa** - Delta > 2cm (configurável)
3. **Heartbeat** - A cada 5 minutos (configurável)

```c
#define DEADBAND_CM         2               // Variação mínima (cm)
#define HEARTBEAT_MS        (5 * 60 * 1000) // Heartbeat 5 min
```

## 🔧 Configuração

### Node (main.c)

```c
// GPIO
#define GPIO_TRIG           GPIO_NUM_1      // Trigger ultrassônico
#define GPIO_ECHO           GPIO_NUM_0      // Echo ultrassônico
#define GPIO_LED            GPIO_NUM_8      // LED builtin

// ESP-NOW
#define ESPNOW_CHANNEL      11              // Canal WiFi/ESP-NOW
```

### Gateway (main.c)

```c
// GPIO
#define GPIO_LED            GPIO_NUM_8      // LED builtin

// ESP-NOW
#define ESPNOW_CHANNEL      11              // Deve ser igual ao node!
```

### Bridge (argumentos)

```bash
python bridge_usb.py \
    --port /dev/ttyACM0 \
    --baudrate 115200 \
    --backend http://localhost:3000 \
    --verbose
```

## 🔍 Debug

### Monitor do Node

```bash
idf.py -p /dev/ttyACM0 monitor
```

Saída esperada:

```
I (1000) NODE_MIN: AGUADA Node Minimal v1.0
I (1100) NODE_MIN: GPIO: TRIG=1, ECHO=0, LED=8
I (1200) NODE_MIN: Node MAC: 20:6E:F1:6B:77:58
I (1300) NODE_MIN: ESP-NOW iniciado (canal 11)
I (2000) NODE_MIN: Distância: 244.80 cm [FIRST]
I (2100) NODE_MIN: TX: {"mac":"20:6E:F1:6B:77:58","type":"distance_cm","value":24480}
```

### Monitor do Gateway

```bash
# O gateway envia JSON puro via printf
cat /dev/ttyACM0
```

Saída esperada:

```json
{"mac":"20:6E:F1:6B:77:58","type":"gateway_boot","channel":11}
{"mac":"20:6E:F1:6B:77:58","type":"distance_cm","value":24480,"uptime":5,"rssi":-45}
```

### Bridge Verbose

```bash
python bridge_usb.py -v
```

Saída:

```
12:34:56 [INFO] Conectado em /dev/ttyACM0 @ 115200 baud
12:34:56 [INFO] Bridge iniciada. Backend: http://localhost:3000
12:34:57 [INFO] RX: 20:6E:F1:6B:77:58 | distance_cm=24480 | RSSI=-45
12:34:57 [DEBUG] TX: 20:6E:F1:6B:77:58 → backend OK
```

## 📝 Notas

### Valor da Distância

- Multiplicado por 100 para evitar floats
- `24480` = 244.80 cm
- Backend deve dividir por 100 ao processar

### Canal ESP-NOW

- Node e Gateway DEVEM usar o mesmo canal
- Default: canal 11
- Alterar em ambos se necessário

### LED Status

| Estado                  | Significado         |
| ----------------------- | ------------------- |
| 3 piscos rápidos (boot) | Node iniciando      |
| 5 piscos rápidos (boot) | Gateway iniciando   |
| Pisca ao enviar         | Transmissão ESP-NOW |
| Pisca ao receber        | Recepção ESP-NOW    |

## 📚 Próximos Passos

Para evoluir esta versão mínima:

1. **Adicionar OTA** - Atualização via WiFi AP
2. **Adicionar sensores** - Sound, valves, voltage
3. **Modo Relay** - Node com WiFi que retransmite
4. **Deep Sleep** - Economia de energia
5. **Configuração via Web** - WiFi AP + página de config

---

**Versão**: 1.0  
**Data**: 2025-12-06  
**Autor**: AGUADA Project
