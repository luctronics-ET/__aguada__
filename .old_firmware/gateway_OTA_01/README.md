# AGUADA Gateway OTA v1.0

Gateway ESP32 DevKit v1 com suporte a OTA (Over-The-Air updates) e modo repetidor para mesh.

## Características

- **ESP-NOW Receiver**: Recebe dados dos nodes sensores
- **ESP-NOW Repeater**: Modo mesh - retransmite para outros gateways
- **HTTP POST**: Envia telemetria para backend
- **OTA Updates**: Atualização de firmware via HTTP
- **Métricas**: Envio periódico de health metrics
- **Fallback Buffer**: Armazena pacotes quando offline

## Hardware

- **Placa**: ESP32 DevKit v1 (WROOM-32)
- **LED**: GPIO 2 (onboard)
- **Flash**: 4MB (partition table com OTA)

## Modos de Operação

### Modo Primário (default)

```
Sensors → [Este Gateway] → HTTP POST → Backend
```

Recebe dados via ESP-NOW e envia diretamente para o backend.

### Modo Repetidor

```
Sensors → [Este Gateway] → ESP-NOW → [Gateway Primário] → Backend
```

Recebe dados via ESP-NOW e retransmite para outro gateway via ESP-NOW.
Útil para aumentar o alcance em áreas distantes.

## Configuração

### Via menuconfig

```bash
idf.py menuconfig
# → AGUADA Gateway OTA Configuration
```

### Opções principais:

- **WiFi SSID/Password**: Rede WiFi
- **Backend Host/Port**: Servidor backend
- **ESP-NOW Channel**: Canal WiFi (deve coincidir com SSID)
- **Gateway Mode**: Primário ou Repetidor
- **Primary Gateway MAC**: MAC do gateway primário (modo repetidor)

## Build & Flash

```bash
cd firmware/gateway_OTA_01

# Configurar target (ESP32)
idf.py set-target esp32

# Configurar opções
idf.py menuconfig

# Build
idf.py build

# Flash
idf.py -p /dev/ttyUSB0 flash monitor
```

## OTA Update

O gateway verifica atualizações a cada 5 minutos:

1. GET `/api/firmware/gateway/check?mac=XX&version=X.X.X&type=OTA_01`

   - Retorna 200 se há nova versão
   - Retorna 204 se está atualizado

2. Se houver nova versão:
   - GET `/api/firmware/gateway/download?type=OTA_01`
   - Baixa e instala o novo firmware
   - Reinicia automaticamente

### Backend Endpoints (a implementar)

```javascript
// Verificar se há atualização
GET /api/firmware/gateway/check
Query: { mac, version, type }
Response: 200 (nova versão) | 204 (atualizado)

// Download do firmware
GET /api/firmware/gateway/download
Query: { type }
Response: application/octet-stream (firmware.bin)

// Upload de novo firmware (admin)
POST /api/firmware/gateway/upload
Body: multipart/form-data { file, type, version }
```

## Métricas

Enviadas a cada 60 segundos:

```json
{
  "mac": "XX:XX:XX:XX:XX:XX",
  "type": "OTA_01",
  "version": "1.0.0",
  "mode": "primary",
  "metrics": {
    "packets_received": 1234,
    "packets_sent": 1200,
    "packets_failed": 10,
    "packets_dropped": 5,
    "packets_repeated": 0,
    "http_errors": 3,
    "queue_usage_percent": 15,
    "peer_count": 5,
    "ota_checks": 12,
    "ota_updates": 0,
    "uptime_seconds": 3600
  }
}
```

## Mesh Network

### Topologia exemplo

```
         [Node RCON]     [Node RCAV]     [Node RB03]
              │               │               │
              └───────────────┴───────────────┘
                              │
                        (ESP-NOW)
                              │
                              ▼
                    [Gateway Primário]  ←─── WiFi ───→ Backend
                         (OTA_01)
                              │
                        (ESP-NOW)
                              │
              ┌───────────────┴───────────────┐
              │                               │
    [Gateway Repetidor 1]         [Gateway Repetidor 2]
         (OTA_01)                      (OTA_01)
              │                               │
        (ESP-NOW)                       (ESP-NOW)
              │                               │
        [Node IE01]                     [Node IE02]
```

## LEDs

- **Pisca lento (3s)**: Operação normal
- **Pisca rápido**: OTA em progresso
- **5 piscos no boot**: Inicialização OK

## Troubleshooting

### Gateway não recebe dados

1. Verificar canal ESP-NOW (deve coincidir com todos os nodes)
2. Verificar se nodes estão transmitindo (monitor serial)
3. Verificar distância/obstáculos

### OTA falha

1. Verificar conexão WiFi
2. Verificar endpoint do backend
3. Verificar tamanho do firmware (< 1.5MB para OTA)
4. Verificar partition table (CONFIG_PARTITION_TABLE_TWO_OTA=y)

### HTTP POST falha

1. Verificar IP do backend
2. Verificar porta (3000)
3. Verificar endpoint (/api/telemetry)
4. Verificar logs do backend

## Changelog

### v1.0.0

- Versão inicial
- ESP-NOW receiver
- HTTP POST com retry
- OTA update
- Modo repetidor (mesh)
- Métricas
