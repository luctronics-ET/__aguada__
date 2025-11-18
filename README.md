# AGUADA - Sistema de Monitoramento Hidráulico IoT

Sistema completo de monitoramento de reservatórios de água usando ESP32, sensores ultrassônicos, MQTT e TimescaleDB.

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-5.x-green.svg)](https://github.com/espressif/esp-idf)
[![Node.js](https://img.shields.io/badge/Node.js-18+-green.svg)](https://nodejs.org/)
[![PostgreSQL](https://img.shields.io/badge/PostgreSQL-15+-blue.svg)](https://www.postgresql.org/)

## 📋 Visão Geral

Sistema de monitoramento em tempo real de 5 reservatórios de água com:
- 📊 Telemetria automática a cada 30 segundos
- 📡 Comunicação ESP-NOW entre sensores e gateway (até 250m)
- 🔄 Compressão inteligente de dados (>90% de redução)
- 🚨 Detecção automática de eventos (abastecimento, consumo, vazamento)
- 📈 Dashboard Grafana em tempo real
- 📱 Relatórios diários automáticos (06:00h)
- 🏭 Casa de Bombas com reservatório e 2 bombas (elétrica/diesel)

## 🏗️ Arquitetura

```
┌─────────────┐
│  ESP32-C3   │──ESP-NOW──┐
│ + AJ-SR04M  │           │
│  (Sensor)   │           │
└─────────────┘           │     ┌──────────────┐     ┌────────────┐
                          ├────→│   Gateway    │─────→│  Backend   │────→│ PostgreSQL │
┌─────────────┐           │     │   ESP32-C3   │ MQTT │  Node.js   │     │ TimescaleDB│
│  ESP32-C3   │──ESP-NOW──┤     └──────────────┘     └──────────────┘     └────────────┘
│ + AJ-SR04M  │           │                                  │                     │
│  (Sensor)   │           │                                  ↓                     ↓
└─────────────┘           │                          ┌──────────────┐     ┌────────────┐
                          │                          │   Grafana    │←────│   Redis    │
┌─────────────┐           │                          │  Dashboard   │     │   Queue    │
│  ESP32-C3   │──ESP-NOW──┘                          └──────────────┘     └────────────┘
│ + AJ-SR04M  │
│  (Sensor)   │
└─────────────┘
```

## 📂 Estrutura do Projeto

```
aguada/
├── 📄 README.md                    # Este arquivo
├── 📄 LICENSE                      # Licença MIT
├── 📁 docs/                        # Documentação
│   ├── RULES.md                   # Regras e padrões do sistema
│   ├── SETUP.md                   # Guia de instalação
│   ├── CHANGELOG.md               # Histórico de versões
│   ├── SUMMARY.md                 # Resumo executivo
│   ├── AGUADA_REF_DEF_RULES.txt  # Regras detalhadas
│   └── ESP32_C3_SUPER_MINI_PINOUT.md
│
├── 📁 firmware/                    # Firmware ESP32-C3
│   ├── gateway_00/                # Gateway ESP-NOW → MQTT
│   │   ├── main/                  # Código principal
│   │   │   ├── main.c/cpp
│   │   │   ├── config_pins.h
│   │   │   ├── gateway_io.h/cpp
│   │   │   ├── node_registry.h/cpp
│   │   │   └── queue_manager.h/cpp
│   │   ├── CMakeLists.txt
│   │   ├── sdkconfig.defaults
│   │   └── README.md
│   │
│   └── node_10/                   # Node sensor (res_cons) - USAB
│       ├── main/                  # Código principal
│       │   ├── main.cpp
│       │   ├── config_pins.h
│       │   ├── ultra.h/cpp        # Sensor ultrassônico
│       │   ├── wifi.h/cpp         # ESP-NOW
│       │   ├── ios.h/cpp          # GPIO
│       │   ├── packet.h/cpp       # Protocolo
│       │   └── heartbeat.h/cpp    # Heartbeat
│       ├── CMakeLists.txt
│       ├── sdkconfig.defaults
│       └── README.md
│
├── 📁 backend/                     # Backend Node.js
│   ├── src/
│   │   ├── config/               # Database, Redis, Logger
│   │   ├── controllers/          # API controllers
│   │   ├── routes/               # Express routes
│   │   ├── schemas/              # Zod validation schemas
│   │   ├── services/             # Business logic
│   │   │   ├── sensor.service.js
│   │   │   ├── reading.service.js
│   │   │   ├── volume.service.js
│   │   │   ├── compression.service.js
│   │   │   └── event.service.js
│   │   └── server.js             # Entry point
│   ├── package.json
│   ├── .env.example
│   └── README.md
│
├── 📁 database/                    # PostgreSQL/TimescaleDB
│   ├── schema.sql                # Schema completo
│   ├── migrations/               # Migrações
│   └── seeds/                    # Dados iniciais
│
├── 📁 config/                      # Configurações JSON
│   ├── reservoirs.json           # Specs dos reservatórios
│   ├── sensors.json              # Mapeamento de sensores
│   ├── network_topology.json     # Grafo hidráulico
│   └── thresholds.json           # Thresholds do sistema
│
├── 📁 dashboard/                   # Grafana dashboards
│   ├── provisioning/
│   └── dashboards/
│
├── 📁 scripts/                     # Scripts utilitários
│   ├── install.sh                # Instalação completa
│   ├── backup.sh                 # Backup do banco
│   └── deploy.sh                 # Deploy automático
│
├── 📁 tests/                       # Testes automatizados
│   ├── backend/
│   ├── firmware/
│   └── integration/
│
├── 📁 docker/                      # Docker Compose
    ├── docker-compose.yml
    └── Dockerfile.*

├── 📁 mcp-server/                  # MCP Server (Model Context Protocol)
    ├── src/
    │   └── index.ts              # Servidor MCP
    ├── dist/                     # Código compilado
    ├── package.json
    ├── tsconfig.json
    ├── README.md
    └── QUICKSTART.md             # Guia rápido
```

## 🚀 Quick Start

### 1. Pré-requisitos

```bash
# Sistema operacional
Ubuntu 20.04+ / Debian 11+

# Software necessário
- ESP-IDF 5.x
- Node.js 18+
- PostgreSQL 15+ com TimescaleDB
- MQTT Broker (Mosquitto/EMQX)
- Redis 7+
- Grafana 10+
```

### 2. Instalação Rápida

```bash
# Clone o repositório
git clone https://github.com/seu-usuario/aguada.git
cd aguada

# Execute o script de instalação
./scripts/install.sh

# Ou instale manualmente cada componente
cd backend && npm install
cd ../database && psql -U postgres -f schema.sql
cd ../firmware/node_10 && idf.py build
```

### 3. Configuração

```bash
# Backend
cp backend/.env.example backend/.env
nano backend/.env

# Firmware
nano firmware/node_10/main/config_pins.h
```

### 4. Executar

```bash
# Backend
cd backend && npm start

# Firmware (flash no ESP32)
cd firmware/node_10
idf.py -p /dev/ttyACM0 flash monitor
```

## 📊 Funcionalidades

### ✅ Implementado

- [x] Firmware ESP32-C3 com sensor AJ-SR04M
- [x] Filtro de mediana (11 amostras)
- [x] Cálculo de volume e percentual
- [x] Backend API REST (3 endpoints)
- [x] Validação rigorosa (Zod schemas)
- [x] Compressão de dados (deadband 2cm)
- [x] Detecção de eventos (abastecimento, vazamento, nível crítico)
- [x] Schema PostgreSQL + TimescaleDB
- [x] Retenção e compressão automática
- [x] Sistema de auditoria completo
- [x] Comunicação ESP-NOW sensor → gateway
- [x] Gateway com WiFi + MQTT QoS 1
- [x] HTTP fallback no gateway
- [x] Watchdog timer
- [x] Detecção de falhas de sensor

### 🔄 Em Desenvolvimento

- [ ] Dashboard Grafana
- [ ] Relatório diário automático (06:00h)
- [ ] Cálculo de consumo por período
- [ ] Interface web de configuração
- [ ] App mobile
- [ ] Sistema de alertas (email/SMS)

### 🎯 Roadmap (v2.0)

- [ ] Controle automático de bombas
- [ ] Machine Learning para predição
- [ ] Simulador hidráulico
- [ ] Multi-tenancy
- [ ] API GraphQL

## 🤖 MCP Server (Model Context Protocol)

O AGUADA inclui um servidor MCP que fornece contexto inteligente sobre o sistema para assistentes de IA como o GitHub Copilot.

### Ferramentas Disponíveis

- **`get_telemetry`** - Buscar dados de telemetria dos sensores
- **`get_reservoir_status`** - Status atual de um reservatório
- **`get_system_overview`** - Visão geral completa do sistema
- **`analyze_consumption`** - Análise de padrões de consumo
- **`check_events`** - Verificar eventos hidráulicos

### Recursos Disponíveis

- Configurações: reservatórios, sensores, topologia da rede
- Documentação: schema do banco, API docs
- Dados em tempo real (em produção)

### Como Usar

```bash
# Instalar dependências
cd mcp-server
npm install

# Compilar
npm run build

# Testar com MCP Inspector
npm run inspector
```

O servidor está pré-configurado no VS Code (`.vscode/settings.json`). Recarregue a janela para ativar.

Ver [mcp-server/QUICKSTART.md](mcp-server/QUICKSTART.md) para guia completo.

## 🔧 Configuração Técnica

### Configuração Técnica

### Reservatórios Monitorados (5 Total)

| ID | Nome | Alias | Tipo | Volume | Sensor | Node | Local | Hardware |
|----|------|-------|------|--------|--------|------|-------|----------|
| RCON | Castelo Consumo | CON | Cilíndrico | 80m³ | SEN_CON_01 | ESP32 #1 | Cobertura A | Ultra, 2 Válvulas, Som |
| RCAV | Castelo Incêndio | CAV | Cilíndrico | 80m³ | SEN_CAV_01 | ESP32 #2 | Cobertura B | Ultra, 2 Válvulas, Som |
| RB03 | Reservatório B03 | B03 | Cilíndrico | 80m³ | SEN_B03_01 | ESP32 #3 | Casa Bombas | Ultra, 2 Válvulas, Som |
| IE01 | Cisterna IE 01 | IE01 | Retangular | 254m³ | SEN_IE01_01 | ESP32 #4 | Subsolo | Ultra, 2 Válvulas, Som |
| IE02 | Cisterna IE 02 | IE02 | Retangular | 254m³ | SEN_IE02_01 | ESP32 #5 | Subsolo | Ultra, 2 Válvulas, Som |

**Nota:** Todos os 5 nodes usam **firmware idêntico** (TYPE_SINGLE_ULTRA). Cada reservatório tem seu próprio ESP32-C3.

### Casa de Bombas N03 (CB03)

- **Reservatório RB03**: 80m³ (armazenamento intermediário)
- **Bombas B03E/B03D**: Elementos **independentes** (não controlados por ESP32)
- **Sensor ESP32-C3**: Monitora apenas nível, válvulas e som
- **Função**: Recalcar água das cisternas IE para RCON ou RCAV

### Recursos de Hardware por Node

**Todos os 5 Nodes ESP32-C3 são idênticos:**
- ✅ **1 sensor ultrassônico** AJ-SR04M (distance_cm)
- ✅ **2 válvulas** digitais GPIO (valve_in, valve_out)
- ✅ **1 detector de som** GPIO (sound_in - detecta água entrando)
- ✅ **RSSI** - força do sinal ESP-NOW
- ✅ **Battery** - fonte DC 5V (5000mV)
- ✅ **Uptime** - contador desde boot

**Firmware Único:**
- Mesmo binário em todos os 5 ESP32-C3
- Diferenciação via **MAC address** (hardware)
- Backend resolve mapeamento MAC → reservatório
- GPIOs fixos definidos em `config_pins.h`

### Dados Enviados (Individual)

- **Deadband**: 2cm (variação mínima para nova leitura)
- **Window Size**: 11 amostras (filtro de mediana)
- **Stability StdDev**: 0.5cm (desvio padrão máximo)
- **Redução de dados**: >90%

### Thresholds de Eventos

- **Abastecimento**: ΔV > +50L + duração >300s
- **Vazamento**: Taxa < -15L/h por >1h
- **Nível Crítico CAV**: <70% por >10min
- **Sensor Fault**: Stuck >60s ou timeout >300s

## 📡 API Endpoints

### POST /api/telemetry
Recebe telemetria do gateway ESP32 via MQTT/HTTP.

**Formato Simplificado - Envio Individual:**

```json
{
  "mac": "dc:06:75:67:6a:cc",
  "type": "distance_cm",
  "value": 24480,
  "battery": 5000,
  "uptime": 3600,
  "rssi": -50
}
```

**Campos:**
- `mac`: MAC address do node (identificação única)
- `type`: tipo de dado (`distance_cm`, `sound_in`, `valve_in`, `valve_out`)
- `value`: valor como inteiro (distance_cm multiplicado por 100, estados 0/1)
- `battery`: tensão em mV (fonte DC 5V = 5000mV)
- `uptime`: segundos desde boot
- `rssi`: força do sinal em dBm
- `datetime`: adicionado pelo servidor ao receber

**Conversão no Backend:**
```javascript
// distance_cm: int → float
const distance_cm = value / 100.0;  // 24480 → 244.8 cm

// Estados: int → boolean
const sound_in = value === 1;  // 0 ou 1
const valve_in = value === 1;
```

### 📊 Detector de Som - Água Entrando

**Funcionalidade:** Detecta ruído de água caindo/entrando no reservatório (abastecimento)

**Benefícios:**
1. **Detecção de abastecimento** - Som de água entrando confirma que está enchendo
2. **Validação cruzada** - Confirma aumento de nível é abastecimento real
3. **Timestamp preciso** - Marca exato momento que água começa a entrar
4. **Complementa nível** - Detecta início antes do nível subir significativamente
5. **Alarme antecipado** - Identifica abastecimento não programado

**Implementação:**
- GPIO 5 (modo digital INPUT)
- GPIO 21 para IE02 (dual sensor)
- Enviado como `type: "sound_detected", value: 0/1`
- Mudança de estado gera transmissão imediata

### POST /api/manual-reading
Registra leitura manual.

### POST /api/calibration
Registra calibração de sensor.

## 🧪 Testes

```bash
# Backend
cd backend
npm test
npm run test:watch

# Firmware
cd firmware/node_10
idf.py build
idf.py flash monitor
```

## 📈 Performance

- **API Latency**: <100ms (p95)
- **Throughput**: 100 leituras/segundo
- **Database Compression**: >90% redução
- **Uptime**: >99.9%

## 🔒 Segurança

- ESP-NOW com criptografia (LMK)
- WiFi WPA2 (gateway)
- MQTT com autenticação
- JWT para API
- Rate limiting (60 req/min)
- HTTPS (produção)
- SQL injection protection
- Input validation (Zod)

## 📚 Documentação

- [📖 Guia de Instalação](docs/SETUP.md)
- [📋 Regras do Sistema](docs/RULES.md)
- [📝 Changelog](docs/CHANGELOG.md)
- [📊 Resumo Executivo](docs/SUMMARY.md)
- [🔧 Backend API](backend/README.md)
- [📱 Firmware ESP32](firmware/node_10/README.md)

## 🤝 Contribuindo

1. Fork o projeto
2. Crie uma branch (`git checkout -b feature/nova-funcionalidade`)
3. Commit suas mudanças (`git commit -am 'Adiciona nova funcionalidade'`)
4. Push para a branch (`git push origin feature/nova-funcionalidade`)
5. Abra um Pull Request

## 📄 Licença

Este projeto está licenciado sob a MIT License - veja o arquivo [LICENSE](LICENSE) para detalhes.

## 👥 Autores

- **Equipe AGUADA** - *Desenvolvimento inicial*

## 🙏 Agradecimentos

- Espressif (ESP-IDF)
- TimescaleDB
- Grafana Labs
- Comunidade open source

## 📞 Suporte

- 📧 Email: suporte@aguada.local
- 🐛 Issues: [GitHub Issues](https://github.com/seu-usuario/aguada/issues)
- 📖 Wiki: [GitHub Wiki](https://github.com/seu-usuario/aguada/wiki)

---

**Versão**: 1.0.0  
**Última atualização**: 16 de novembro de 2025  
**Status**: ✅ Produção
