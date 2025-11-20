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
│   ├── node_sensor_10/            # Node sensor único (RCON, RCAV, RB03)
│       ├── main/
│       │   ├── main.c             # Firmware TYPE_SINGLE_ULTRA
│       │   └── config.h           # GPIOs: TRIG=1, ECHO=0, etc
│       ├── CMakeLists.txt
│       ├── sdkconfig.defaults
│       └── README.md
│
│   └── node_sensor_20/            # Node sensor duplo (IE01 + IE02) ✨ NOVO
│       ├── main/
│       │   ├── main.c             # Firmware TYPE_DUAL_ULTRA
│       │   └── config.h           # 2 ultrassom, 4 válvulas, 2 som
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

### ✅ Implementado - Sistema BMS/CMMS/SCADA Completo

#### Frontend (10 Páginas)
- [x] **index.html** - Dashboard principal com cards de sensores
- [x] **painel.html** - Diagrama visual hidráulico com SVG/CSS
- [x] **dados.html** - Tabelas completas com filtros, ordenação e paginação
- [x] **consumo.html** - Análise de consumo com 5 gráficos Chart.js
- [x] **abastecimento.html** - Monitoramento de abastecimento em tempo real
- [x] **manutencao.html** - Gestão CMMS de manutenção (ordens, calendário, estatísticas)
- [x] **history.html** - Histórico de leituras com gráficos
- [x] **alerts.html** - Sistema de alertas e notificações
- [x] **config.html** - Configurações de sensores e sistema
- [x] **system.html** - Status e diagnósticos do sistema

#### Backend API (32 Endpoints REST)

**Telemetria (3 endpoints)**
- [x] POST /api/telemetry - Recebe dados dos ESP32
- [x] POST /api/manual-reading - Leituras manuais
- [x] POST /api/calibration - Calibração de sensores

**Leituras (4 endpoints)**
- [x] GET /api/readings/latest - Últimas leituras
- [x] GET /api/readings/daily-summary - Resumo diário
- [x] GET /api/readings/history/:sensor_id - Histórico
- [x] GET /api/readings/export - Exportar CSV

**Sensores (4 endpoints)**
- [x] GET /api/sensors - Listar sensores
- [x] GET /api/sensors/status - Status de conexão
- [x] GET /api/sensors/:sensor_id - Detalhes do sensor
- [x] PUT /api/sensors/:sensor_id - Atualizar configuração

**Alertas (5 endpoints)**
- [x] GET /api/alerts - Listar alertas (com filtros)
- [x] GET /api/alerts/summary - Resumo de alertas
- [x] POST /api/alerts - Criar alerta
- [x] PUT /api/alerts/:alert_id/resolve - Resolver alerta
- [x] GET /api/alerts/export - Exportar CSV

**Estatísticas (4 endpoints)**
- [x] GET /api/stats/daily - Estatísticas diárias
- [x] GET /api/stats/consumption - Análise de consumo
- [x] GET /api/stats/sensors - Estatísticas de sensores
- [x] GET /api/stats/events - Estatísticas de eventos

**Sistema (4 endpoints)**
- [x] GET /api/system/health - Health check completo
- [x] GET /api/system/logs - Logs do sistema
- [x] GET /api/system/metrics - Métricas de performance
- [x] POST /api/system/restart - Reiniciar sistema

#### Recursos Avançados

**WebSocket Real-time**
- [x] Servidor WebSocket em /ws
- [x] Broadcast de leituras em tempo real
- [x] Broadcast de alertas
- [x] Cliente com reconexão automática
- [x] Ping/pong keep-alive

**Exportação de Dados**
- [x] Export leituras para CSV
- [x] Export alertas para CSV
- [x] Função genérica de exportação
- [x] Botões de export nas páginas

**Utilitários Frontend**
- [x] 30+ funções utilitárias (formatação, validação, storage)
- [x] Debounce e throttle
- [x] Toast notifications
- [x] Copy to clipboard
- [x] URL parameter helpers

**Características BMS/CMMS/SCADA**
- [x] **BMS**: Monitoramento em tempo real, dashboards, KPIs, tendências
- [x] **CMMS**: Ordens de manutenção, agendamento, calendário, estatísticas
- [x] **SCADA**: Diagrama P&ID, controle visual, indicadores de estado
- [x] **Real-time**: WebSocket para atualizações instantâneas
- [x] **Offline**: Funciona 100% em rede local sem internet

### 🔄 Em Desenvolvimento (Futuro)
- [ ] Controle automático de bombas via API
- [ ] Machine Learning para predição de consumo
- [ ] App mobile React Native
- [ ] Sistema de notificações (email/SMS)
- [ ] API GraphQL
- [ ] Multi-tenancy

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

| ID | Nome | Alias | Tipo | Volume | Sensor | Node | Firmware | Local | Hardware |
|----|------|-------|------|--------|--------|------|----------|-------|----------|
| RCON | Castelo Consumo | CON | Cilíndrico | 80m³ | SEN_CON_01 | ESP32 #1 | node_sensor_10 | Cobertura A | 1 Ultra, 2 Válvulas, Som |
| RCAV | Castelo Incêndio | CAV | Cilíndrico | 80m³ | SEN_CAV_01 | ESP32 #2 | node_sensor_10 | Cobertura B | 1 Ultra, 2 Válvulas, Som |
| RB03 | Reservatório B03 | B03 | Cilíndrico | 80m³ | SEN_B03_01 | ESP32 #3 | node_sensor_10 | Casa Bombas | 1 Ultra, 2 Válvulas, Som |
| IE01 | Cisterna IE 01 | IE01 | Retangular | 254m³ | SEN_IE01_01 | **ESP32 #4** | **node_sensor_20** | Subsolo | **2 Ultra, 4 Válvulas, 2 Som** |
| IE02 | Cisterna IE 02 | IE02 | Retangular | 254m³ | SEN_IE02_01 | **ESP32 #4** | **node_sensor_20** | Subsolo | **2 Ultra, 4 Válvulas, 2 Som** |

**Notas:**
- **RCON, RCAV, RB03**: Firmware `node_sensor_10` (TYPE_SINGLE_ULTRA) - 1 reservatório por ESP32
- **IE01 + IE02**: Firmware `node_sensor_20` (TYPE_DUAL_ULTRA) - **2 reservatórios em 1 ESP32** ✨
- **Total de ESP32-C3**: 4 microcontroladores (ao invés de 5)

### Casa de Bombas N03 (CB03)

- **Reservatório RB03**: 80m³ (armazenamento intermediário)
- **Bombas B03E/B03D**: Elementos **independentes** (não controlados por ESP32)
- **Sensor ESP32-C3**: Monitora apenas nível, válvulas e som
- **Função**: Recalcar água das cisternas IE para RCON ou RCAV

### Recursos de Hardware por Node

#### node_sensor_10 (RCON, RCAV, RB03) - TYPE_SINGLE_ULTRA

**3 ESP32-C3 com firmware idêntico:**

- ✅ **1 sensor ultrassônico** AJ-SR04M (distance_cm)
- ✅ **2 válvulas** digitais GPIO (valve_in, valve_out)
- ✅ **1 detector de som** GPIO (sound_in)
- ✅ **RSSI** - força do sinal ESP-NOW
- ✅ **Battery** - fonte DC 5V (5000mV)
- ✅ **Uptime** - contador desde boot

**Firmware:** Mesmo binário nos 3 ESP32, diferenciação via MAC address

#### node_sensor_20 (IE01 + IE02) - TYPE_DUAL_ULTRA

**1 ESP32-C3 monitora 2 reservatórios simultaneamente:**

- ✅ **2 sensores ultrassônicos** AJ-SR04M (IE01_distance_cm, IE02_distance_cm)
- ✅ **4 válvulas** digitais GPIO (IE01_valve_in/out, IE02_valve_in/out)
- ✅ **2 detectores de som** GPIO (IE01_sound_in, IE02_sound_in)
- ✅ **RSSI, Battery, Uptime** - compartilhados entre os 2 reservatórios

**Firmware:** Específico para dual, envia 8 variáveis (4 por reservatório)

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

### Telemetria
```bash
# Receber dados do ESP32
POST /api/telemetry
POST /api/manual-reading
POST /api/calibration
```

### Leituras
```bash
# Consultar leituras
GET /api/readings/latest
GET /api/readings/daily-summary
GET /api/readings/history/:sensor_id
GET /api/readings/export?format=csv
```

### Sensores
```bash
# Gerenciar sensores
GET /api/sensors
GET /api/sensors/status
GET /api/sensors/:sensor_id
PUT /api/sensors/:sensor_id
```

### Alertas
```bash
# Sistema de alertas
GET /api/alerts?status=active&level=critical
GET /api/alerts/summary
POST /api/alerts
PUT /api/alerts/:alert_id/resolve
GET /api/alerts/export?format=csv
```

### Estatísticas
```bash
# Análises e estatísticas
GET /api/stats/daily?date=2025-11-18
GET /api/stats/consumption?period=7d&group_by=day
GET /api/stats/sensors
GET /api/stats/events
```

### Sistema
```bash
# Monitoramento do sistema
GET /api/system/health
GET /api/system/logs
GET /api/system/metrics
POST /api/system/restart
```

### WebSocket
```javascript
// Conectar ao WebSocket
const ws = new WebSocket('ws://192.168.0.100:3000/ws');

// Receber eventos em tempo real
ws.onmessage = (event) => {
  const data = JSON.parse(event.data);
  
  if (data.type === 'reading') {
    // Nova leitura recebida
    console.log('Nova leitura:', data.data);
  } else if (data.type === 'alert') {
    // Novo alerta
    console.log('Alerta:', data.data);
  }
};
```

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
