# 🚀 AGUADA Quick Start (5 minutos)

## Antes de começar
```bash
cd /home/luciano/Área\ de\ trabalho/__aguada__
```

---

## 1️⃣ Iniciar Backend (Terminal 1)

```bash
cd backend
npm install  # Primeira vez only
npm run dev
```

**Esperado:**
```
🚀 Servidor rodando na porta 3000
📊 Ambiente: development
🔗 API: http://localhost:3000/api
```

Testar:
```bash
curl http://localhost:3000/api/health
# {"status":"ok","timestamp":"2025-11-17T...","service":"aguada-backend","version":"1.0.0"}
```

---

## 2️⃣ Abrir Dashboard (Navegador)

```
file:///home/luciano/Área\ de\ trabalho/__aguada__/frontend/index.html
```

**Ou via servidor:**
```
http://192.168.0.100:3000/dashboard
```

---

## 3️⃣ Flashear Gateway (Terminal 2)

Se ainda não foi flashed:

```bash
cd firmware/gateway_esp_idf
idf.py -p /dev/ttyACM0 flash monitor
```

**Esperado no monitor:**
```
✓ Fila ESP-NOW criada (10 slots)
✓ GPIO inicializado (LED=8)
✓ WiFi inicializado (CH1, PHY apenas)
✓ ESP-NOW inicializado
✓ Callback registrado
✓ Peer broadcast adicionado
HTTP task iniciada
✓ Gateway inicializado e pronto!
```

---

## 4️⃣ Flashear Sensores (Terminal 3)

Para cada sensor não flashed ainda:

```bash
# Conectar sensor 1 ao USB
cd firmware/node_sensor_10
idf.py -p /dev/ttyACM0 flash monitor

# Depois conectar sensor 2
idf.py -p /dev/ttyACM0 flash monitor

# ... repetir para 5 nodes
```

**Esperado no monitor:**
```
I (403) AGUADA_NODE: GPIO inicializado (TRIG=1, ECHO=0, VALVE_IN=2, VALVE_OUT=3, SOUND=5)
I (1752) AGUADA_NODE: Node MAC: 20:6E:F1:6B:77:58
I (1753) AGUADA_NODE: ESP-NOW OK - Gateway: 80:F1:B2:50:2E:C4
I (4162) AGUADA_NODE: → {"mac":"20:6E:F1:6B:77:58","type":"distance_cm","value":24480,...}
...heartbeat a cada 30s
```

---

## 5️⃣ Validar Fluxo de Dados

### No Gateway Monitor (Terminal 2)

```bash
idf.py -p /dev/ttyACM0 monitor | grep -E "Dequeued|HTTP POST"
```

Esperado:
```
Dequeued de: 20:6E:F1:6B:77:58 (XX bytes)
→ HTTP POST (status=200)
```

### No Backend Log (Terminal 1)

```bash
# Monitor dos logs
tail -f logs/aguada.log | grep -i "telemetria\|POST"
```

Esperado:
```
Telemetria recebida: node_mac=20:6E:F1:6B:77:58, readings=4
```

### No Dashboard (Navegador)

Aguardar 10 segundos para primeira atualização. Deverá mostrar:

- ✅ 5 cards de reservatórios
- ✅ Gauges com nível %
- ✅ Status das válvulas (🟢/🔴)
- ✅ Detecção de fluxo (💧/⏹️)
- ✅ Timestamps das últimas leituras
- ✅ Stats footer com sensores ativos

---

## 🔍 Troubleshooting Rápido

### Backend não inicia

```bash
# Verificar se porta 3000 está livre
lsof -i :3000

# Checar se node_modules existe
ls backend/node_modules/

# Reinstalar
cd backend && rm -rf node_modules && npm install
```

### Gateway não recebe dados

```bash
# Verificar se sensores estão transmitindo
# Terminal do sensor: deve aparecer "→ JSON" a cada 30s

# Verificar canal WiFi no gateway
idf.py -p /dev/ttyACM0 monitor | grep -i "ch1\|channel"

# Verificar MAC do gateway
idf.py -p /dev/ttyACM0 monitor | grep "Gateway MAC"
```

### Dashboard não atualiza

```bash
# Browser console: Ctrl+Shift+K (DevTools)
# Testar fetch manual:
fetch('http://192.168.0.100:3000/api/readings/latest')
  .then(r => r.json())
  .then(console.log)

# Deve retornar dados dos sensores
```

---

## 📊 Endpoints para Testar

```bash
# Health check
curl http://localhost:3000/api/health

# Últimas leituras
curl http://localhost:3000/api/readings/latest

# Status dos sensores
curl http://localhost:3000/api/sensors/status

# Resumo do dia
curl http://localhost:3000/api/readings/daily-summary

# Histórico de um sensor
curl http://localhost:3000/api/readings/history/RCON?days=7

# Test telemetry POST
curl -X POST http://localhost:3000/api/telemetry \
  -H "Content-Type: application/json" \
  -d '{
    "mac":"20:6E:F1:6B:77:58",
    "type":"distance_cm",
    "value":24480,
    "battery":5000,
    "uptime":10,
    "rssi":-50
  }'
```

---

## 🎯 Checklist de Validação

- [ ] Backend iniciando (porta 3000)
- [ ] Health check respondendo
- [ ] Gateway boot sequence OK
- [ ] Sensor 1 transmitindo heartbeat
- [ ] Sensor 2 transmitindo heartbeat
- [ ] Gateway recebendo pacotes (logs "Dequeued")
- [ ] Gateway enviando HTTP POST (logs "HTTP POST")
- [ ] Backend recebendo telemetria (logs)
- [ ] Dashboard carregando
- [ ] Dashboard mostrando dados em tempo real
- [ ] Gauges atualizando a cada 10s
- [ ] Status online de sensores

---

## 📈 Próximos Passos

1. **Flashear 3 sensores restantes** (RB03, IE01, IE02)
2. **Validar que gateway recebe todos** (5 nodes)
3. **Configurar database PostgreSQL** (se não está)
4. **Importar schema** (`database/schema.sql`)
5. **Deploy em produção** (Docker, reverse proxy, etc)

---

## 📞 Documentação Completa

- **DEPLOYMENT.md** - Setup detalhado
- **IMPLEMENTATION_SUMMARY.md** - Status da implementação
- **ARCHITECTURE.md** - Arquitetura do gateway
- **docs/RULES.md** - Especificação do sistema
- **.github/copilot-instructions.md** - Instruções de codificação

---

**Pronto? Vamos! 🚀**

Tempo estimado: 5 minutos de setup + X minutos para flashear sensores
