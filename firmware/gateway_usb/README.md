# AGUADA Gateway USB

Firmware mínimo para ESP32-C3 SuperMini que funciona como bridge ESP-NOW → Serial USB.

## Características

- **Recebe pacotes ESP-NOW** de todos os sensores (broadcast)
- **Envia JSON via USB Serial** (printf/stdout)
- **Não precisa de WiFi** - funciona offline
- **LED indica recepção** de pacotes
- **Status periódico** a cada 60 segundos
- **Canal fixo 11** (mesmo dos sensores)

## Hardware

- ESP32-C3 SuperMini
- Conexão USB-C direta ao computador
- LED interno GPIO 8

## Compilação

```bash
cd firmware/gateway_usb
idf.py set-target esp32c3
idf.py build
```

## Flash

```bash
idf.py -p /dev/ttyACM0 flash monitor
```

## Formato de Saída Serial

### Telemetria dos Sensores

```json
{
  "mac": "XX:XX:XX:XX:XX:XX",
  "distance_mm": 2450,
  "vcc_bat_mv": 4900,
  "rssi": -50
}
```

### Status do Gateway (a cada 60s)

```json
{
  "mac": "YY:YY:YY:YY:YY:YY",
  "type": "gateway_status",
  "rx": 150,
  "proc": 150,
  "drops": 0,
  "uptime": 3600,
  "channel": 11,
  "version": "v2.0.0"
}
```

### Boot do Gateway

```json
{
  "mac": "YY:YY:YY:YY:YY:YY",
  "type": "gateway_boot",
  "channel": 11,
  "version": "v2.0.0"
}
```

## Configuração dos Sensores

Após flashar o gateway, anote o MAC exibido no boot:

```
GATEWAY MAC: XX:XX:XX:XX:XX:XX
```

Configure este MAC no arquivo `config.h` dos sensores:

```c
static const uint8_t GATEWAY_MAC[6] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX};
```

## Backend Serial Bridge

O backend já tem um componente Serial Bridge que lê de `/dev/ttyACM0`.
Ele processa as linhas JSON e envia para a API de telemetria.

## Troubleshooting

### Gateway não recebe pacotes

1. Verifique se o canal é o mesmo (11)
2. Verifique se o MAC do gateway está correto nos sensores
3. Monitore com `idf.py monitor` para ver logs

### Permissão de porta serial

```bash
sudo usermod -a -G dialout $USER
# Logout e login novamente
```

### Ver dados brutos

```bash
cat /dev/ttyACM0
# ou
screen /dev/ttyACM0 115200
```
