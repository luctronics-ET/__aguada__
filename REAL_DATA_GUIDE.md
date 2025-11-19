# 🌊 AGUADA - Guia de Dados Reais via Gateway USB

## 📡 Fluxo de Dados (Tempo Real)

```
┌─────────────────┐
│  Sensor ESP32   │ distance_cm: 24480
│  (Node RCON)    │ valve_in: 1
│  MAC: dc:06...  │ valve_out: 0
└────────┬────────┘
         │ ESP-NOW (wireless 2.4GHz)
         │ JSON individual por variável
         ▼
┌─────────────────┐
│  Gateway ESP32  │ CH1, recebe ESP-NOW
│  USB: ttyACM0   │ Converte → Serial USB
└────────┬────────┘
         │ Serial 115200 baud
         │ {"mac":"dc:06...","type":"distance_cm","value":24480,...}
         ▼
┌─────────────────┐
│  Serial Bridge  │ Node.js (serialport)
│  Backend        │ Parse JSON, valida
└────────┬────────┘
         │ HTTP POST localhost:3000/api/telemetry
         │
         ▼
┌─────────────────┐
│  Backend API    │ Processa, salva DB
│  Express.js     │ Broadcast via WebSocket
└────────┬────────┘
         │ WebSocket + HTTP GET
         │
         ▼
┌─────────────────┐
│  Frontend       │ Dashboard atualizado
│  HTML/JS        │ Gráficos, alertas
└─────────────────┘
```

---

## 🚀 Início Rápido

### 1. Conectar Hardware

```bash
# Conectar gateway ESP32 via USB
# Deve aparecer em: /dev/ttyACM0

# Verificar:
ls -la /dev/ttyACM0
```

### 2. Ajustar Permissões (se necessário)

```bash
# Adicionar usuário ao grupo dialout (permanente)
sudo usermod -a -G dialout $USER

# OU permissão temporária
sudo chmod 666 /dev/ttyACM0
```

### 3. Iniciar Sistema

```bash
# Script automático (recomendado)
./start-real-data.sh

# OU manualmente
cd backend
npm start
```

### 4. Abrir Frontend

```bash
# Em outro terminal
cd frontend
python3 -m http.server 8080

# Acessar: http://localhost:8080/index.html
```

---

## 📊 Monitoramento

### Backend Logs

O backend mostra:

```
[Serial Bridge] ✅ Conectado a /dev/ttyACM0
[Serial Bridge] Aguardando dados do gateway...

[Serial Bridge] 📡 Telemetria recebida: {
  mac: 'dc:06:75:67:6a:cc',
  type: 'distance_cm',
  value: 24480
}

[Serial Bridge] ✅ Enviado ao backend (SEN_CON_01)
```

### Frontend Console (F12)

```
[WS] Mensagem recebida: { type: 'telemetry', data: {...} }
[App] Leituras atualizadas: RCON
```

---

## 🔧 Configuração

### Backend (.env)

```bash
# Porta serial do gateway
SERIAL_PORT=/dev/ttyACM0
SERIAL_BAUD=115200

# Se gateway estiver em outra porta:
# SERIAL_PORT=/dev/ttyUSB0
```

### Gateway (firmware)

**Arquivo:** `firmware/gateway_esp_idf/main/main.c`

```c
#define WIFI_SSID "luciano"
#define WIFI_PASSWORD "Luciano19852012"
#define BACKEND_URL "http://192.168.0.100:3000/api/telemetry"
```

**Compilar e flashear:**
```bash
cd firmware/gateway_esp_idf
idf.py build
idf.py -p /dev/ttyACM0 flash
```

### Sensores (nodes)

**Arquivo:** `firmware/node_sensor_10/main/main.c`

```c
// Gateway MAC (hardcoded nos nodes)
static uint8_t gateway_mac[6] = {0x80, 0xf1, 0xb2, 0x50, 0x2e, 0xc4};

// Intervalo de transmissão
#define HEARTBEAT_INTERVAL_MS 30000  // 30s
```

---

## 🐛 Troubleshooting

### Problema: Gateway não encontrado

**Sintomas:**
```
❌ Gateway não encontrado em /dev/ttyACM0
```

**Soluções:**
1. Verificar cabo USB conectado
2. Verificar se gateway está energizado (LED piscando?)
3. Verificar outra porta: `ls /dev/tty*`
4. Reconectar USB

---

### Problema: Permissão negada

**Sintomas:**
```
Error: EACCES: permission denied, open '/dev/ttyACM0'
```

**Soluções:**
```bash
# Temporária (sessão atual)
sudo chmod 666 /dev/ttyACM0

# Permanente (requer logout/login)
sudo usermod -a -G dialout $USER
newgrp dialout  # ativa sem logout
```

---

### Problema: Nenhum dado recebido

**Sintomas:**
```
[Serial Bridge] ✅ Conectado a /dev/ttyACM0
[Serial Bridge] Aguardando dados do gateway...
(nada acontece por minutos)
```

**Diagnóstico:**
1. Verificar se gateway está recebendo ESP-NOW dos nodes
2. Monitorar serial do gateway diretamente:

```bash
# Desligar backend primeiro!
idf.py -p /dev/ttyACM0 monitor

# Deve mostrar:
# I (12345) AGUADA_GATEWAY: ✓ ESP-NOW recebido de: DC:06:75:67:6A:CC
```

**Causas comuns:**
- Nodes não estão ligados
- Nodes não têm MAC correto do gateway
- Canal WiFi diferente (gateway=1, node=?)
- Distância > 250m (fora de alcance ESP-NOW)

---

### Problema: Dados chegam mas não salvam

**Sintomas:**
```
[Serial Bridge] ✅ Enviado ao backend (SEN_CON_01)
(mas frontend não atualiza)
```

**Verificar:**
1. Database online? `pg_isready -h 192.168.0.100`
2. Tabelas criadas? `psql -h 192.168.0.100 -U aguada_user -d aguada -c "\dt aguada.*"`
3. Backend logs: `tail -f backend/logs/aguada.log`

---

## 📈 Validação de Funcionamento

### Checklist

- [ ] Gateway conectado em /dev/ttyACM0
- [ ] Backend iniciado sem erros
- [ ] Serial Bridge mostra "Conectado"
- [ ] Pelo menos 1 sensor node ligado
- [ ] Backend recebe telemetria (logs)
- [ ] Database salva dados
- [ ] Frontend atualiza em tempo real
- [ ] WebSocket conectado (console F12)

### Teste de Ponta a Ponta

```bash
# 1. Verificar gateway recebendo
idf.py -p /dev/ttyACM0 monitor
# Aguardar linha: "ESP-NOW recebido de: ..."

# 2. Verificar backend processando
cd backend && npm start
# Aguardar linha: "Telemetria recebida: ..."

# 3. Verificar database
psql -h 192.168.0.100 -U aguada_user -d aguada
SELECT * FROM aguada.leituras_raw ORDER BY datetime DESC LIMIT 5;

# 4. Verificar frontend
# Abrir http://localhost:8080/index.html
# Console (F12) deve mostrar: "readings-updated"
```

---

## 🔄 Fluxo Completo (30 segundos)

```
T+00s: Sensor lê ultrassônico (244.8cm)
T+00s: Sensor envia ESP-NOW → Gateway
T+01s: Gateway recebe, formata JSON
T+01s: Gateway → Serial USB → Backend
T+01s: Backend processa, salva DB
T+01s: Backend broadcast WebSocket
T+02s: Frontend recebe, renderiza
T+02s: Dashboard mostra 244.8cm ✅

(Aguarda 30s para próximo heartbeat ou mudança)
```

---

## 📝 Formato de Dados

### ESP-NOW (Sensor → Gateway)

```json
{
  "mac": "dc:06:75:67:6a:cc",
  "type": "distance_cm",
  "value": 24480,
  "battery": 5000,
  "uptime": 120,
  "rssi": -50
}
```

### Serial USB (Gateway → Backend)

```
║ Dados: {"mac":"dc:06:75:67:6a:cc","type":"distance_cm","value":24480,"battery":5000,"uptime":120,"rssi":-50}
```

### HTTP POST (Serial Bridge → API)

```bash
POST http://localhost:3000/api/telemetry
Content-Type: application/json

{
  "mac": "dc:06:75:67:6a:cc",
  "type": "distance_cm",
  "value": 24480,
  "battery": 5000,
  "uptime": 120,
  "rssi": -50
}
```

### Database (PostgreSQL)

```sql
INSERT INTO aguada.leituras_raw 
  (datetime, sensor_id, mac_address, variavel, valor, unidade, ...)
VALUES 
  ('2025-11-19 03:30:00', 'SEN_CON_01', 'dc:06:75:67:6a:cc', 'distance_cm', 24480, 'cm', ...);
```

### WebSocket (Backend → Frontend)

```json
{
  "type": "telemetry",
  "data": {
    "elemento_id": "RCON",
    "variavel": "distance_cm",
    "valor": 24480,
    "timestamp": "2025-11-19T03:30:00.000Z"
  }
}
```

---

## 🎯 Próximos Passos

1. **Ligar mais sensores** - Escalonar para 5 nodes
2. **Calibração** - Ajustar offsets de cada sensor
3. **Alertas** - Configurar limites de nível crítico
4. **Dashboards** - Grafana para visualização histórica
5. **Backup** - Rotina automática de backup do DB

---

**Documentação atualizada em:** 19/11/2025  
**Sistema:** AGUADA v1.0.0 - Dados Reais  
**Hardware:** ESP32-C3 SuperMini + AJ-SR04M
