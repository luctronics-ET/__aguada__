# AGUADA - Guia de Configuração do Ambiente

## 1. Visão Geral

Este guia detalha a instalação e configuração completa do ambiente de desenvolvimento e produção do sistema AGUADA.

---

## 2. Requisitos do Sistema

### 2.1 Hardware Mínimo

**Servidor/PC:**
- CPU: 4 cores
- RAM: 8 GB
- Disco: 50 GB SSD
- Rede: Ethernet 100 Mbps

**Nodes ESP32:**
- ESP32-C3 SuperMini
- Sensor AJ-SR04M
- Fonte 5V/1A

### 2.2 Sistema Operacional

- Ubuntu 22.04 LTS ou Debian 11+ (recomendado)
- Windows 10/11 (com WSL2)
- macOS 12+ (Monterey ou superior)

---

## 3. Instalação do ESP-IDF (Firmware)

### 3.1 Linux/macOS

```bash
# Instalar dependências
sudo apt-get install git wget flex bison gperf python3 python3-pip \
  python3-venv cmake ninja-build ccache libffi-dev libssl-dev \
  dfu-util libusb-1.0-0

# Clonar ESP-IDF
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout v5.1.2

# Instalar ferramentas
./install.sh esp32c3

# Configurar variáveis de ambiente (adicionar ao .bashrc ou .zshrc)
. $HOME/esp/esp-idf/export.sh
```

### 3.2 VSCode + Extensão ESP-IDF

```bash
# Instalar VSCode
wget -qO- https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor > packages.microsoft.gpg
sudo install -o root -g root -m 644 packages.microsoft.gpg /etc/apt/trusted.gpg.d/
sudo sh -c 'echo "deb [arch=amd64] https://packages.microsoft.com/repos/vscode stable main" > /etc/apt/sources.list.d/vscode.list'
sudo apt update
sudo apt install code

# Abrir VSCode e instalar extensão "Espressif IDF"
code --install-extension espressif.esp-idf-extension

# Configurar extensão
# Ctrl+Shift+P → "ESP-IDF: Configure ESP-IDF Extension"
# Selecionar "Use Existing Setup" e apontar para ~/esp/esp-idf
```

### 3.3 Arduino as Component

```bash
cd $IDF_PATH/components
git clone https://github.com/espressif/arduino-esp32.git arduino
cd arduino
git checkout 2.0.14
git submodule update --init --recursive
```

### 3.4 Compilar Firmware Node 10

```bash
cd aguada/prompt_library./firmware/firmware_node10

# Configurar target
idf.py set-target esp32c3

# Configurar WiFi e MQTT (editar main/config.h)
nano main/config.h

# Build
idf.py build

# Flash (conectar ESP32 via USB)
idf.py -p /dev/ttyUSB0 flash monitor
```

---

## 4. PostgreSQL / TimescaleDB

### 4.1 Instalação PostgreSQL 15

```bash
# Adicionar repositório
sudo sh -c 'echo "deb http://apt.postgresql.org/pub/repos/apt $(lsb_release -cs)-pgdg main" > /etc/apt/sources.list.d/pgdg.list'
wget --quiet -O - https://www.postgresql.org/media/keys/ACCC4CF8.asc | sudo apt-key add -

# Instalar
sudo apt update
sudo apt install postgresql-15 postgresql-contrib-15

# Iniciar serviço
sudo systemctl start postgresql
sudo systemctl enable postgresql
```

### 4.2 Configurar Banco AGUADA

```bash
# Entrar como postgres
sudo -u postgres psql

# Criar usuário e banco
CREATE USER aguada WITH PASSWORD 'senha_segura';
CREATE DATABASE aguada_db OWNER aguada;
GRANT ALL PRIVILEGES ON DATABASE aguada_db TO aguada;
\q

# Importar schema
psql -U aguada -d aguada_db -f aguada/database/schema.sql
```

### 4.3 Instalar TimescaleDB (opcional, recomendado)

```bash
# Adicionar repositório Timescale
sudo sh -c "echo 'deb https://packagecloud.io/timescale/timescaledb/ubuntu/ $(lsb_release -c -s) main' > /etc/apt/sources.list.d/timescaledb.list"
wget --quiet -O - https://packagecloud.io/timescale/timescaledb/gpgkey | sudo apt-key add -

# Instalar
sudo apt update
sudo apt install timescaledb-2-postgresql-15

# Configurar
sudo timescaledb-tune

# Reiniciar PostgreSQL
sudo systemctl restart postgresql

# Habilitar extensão no banco
psql -U aguada -d aguada_db -c "CREATE EXTENSION IF NOT EXISTS timescaledb;"

# Converter tabela leituras_raw para hypertable
psql -U aguada -d aguada_db -c "SELECT create_hypertable('leituras_raw', 'datetime');"
```

---

## 5. MQTT Broker (Mosquitto)

### 5.1 Instalação

```bash
sudo apt update
sudo apt install mosquitto mosquitto-clients

# Habilitar serviço
sudo systemctl start mosquitto
sudo systemctl enable mosquitto
```

### 5.2 Configuração

```bash
# Criar arquivo de configuração
sudo nano /etc/mosquitto/conf.d/aguada.conf
```

Conteúdo:
```conf
listener 1883 0.0.0.0
allow_anonymous false
password_file /etc/mosquitto/passwd

# WebSocket (opcional para dashboard web)
listener 9001
protocol websockets
```

### 5.3 Criar Usuários

```bash
# Criar arquivo de senhas
sudo mosquitto_passwd -c /etc/mosquitto/passwd aguada_node
# Digitar senha quando solicitado

# Adicionar mais usuários
sudo mosquitto_passwd /etc/mosquitto/passwd aguada_backend
sudo mosquitto_passwd /etc/mosquitto/passwd aguada_dashboard

# Reiniciar
sudo systemctl restart mosquitto

# Testar
mosquitto_sub -h localhost -t "aguada/#" -u aguada_backend -P senha
# Em outro terminal
mosquitto_pub -h localhost -t "aguada/test" -m "hello" -u aguada_node -P senha
```

---

## 6. Backend Node.js

### 6.1 Instalar Node.js 18+

```bash
# Via NodeSource
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt install -y nodejs

# Verificar
node --version  # v18.x.x
npm --version   # 9.x.x
```

### 6.2 Instalar Dependências do Projeto

```bash
cd aguada/backend
npm install

# Dependências principais:
# - express: API REST
# - pg: cliente PostgreSQL
# - mqtt: cliente MQTT
# - winston: logging
# - zod: validação de schemas
# - dotenv: variáveis de ambiente
```

### 6.3 Configurar Variáveis de Ambiente

```bash
cp .env.example .env
nano .env
```

Conteúdo:
```env
# Database
DB_HOST=localhost
DB_PORT=5432
DB_NAME=aguada_db
DB_USER=aguada
DB_PASS=senha_segura

# MQTT
MQTT_HOST=mqtt://localhost
MQTT_PORT=1883
MQTT_USER=aguada_backend
MQTT_PASS=senha

# API
API_PORT=3000
JWT_SECRET=chave_secreta_longa_aleatoria

# Logging
LOG_LEVEL=info
```

### 6.4 Iniciar Backend

```bash
# Desenvolvimento
npm run dev

# Produção
npm start

# Com PM2 (recomendado produção)
npm install -g pm2
pm2 start ecosystem.config.js
pm2 save
pm2 startup  # Seguir instruções
```

---

## 7. Grafana

### 7.1 Instalação

```bash
# Adicionar repositório
sudo apt-get install -y software-properties-common
sudo add-apt-repository "deb https://packages.grafana.com/oss/deb stable main"
wget -q -O - https://packages.grafana.com/gpg.key | sudo apt-key add -

# Instalar
sudo apt update
sudo apt install grafana

# Iniciar
sudo systemctl start grafana-server
sudo systemctl enable grafana-server
```

### 7.2 Configuração Inicial

```bash
# Acessar http://localhost:3000
# Login: admin / admin
# Trocar senha na primeira vez
```

### 7.3 Adicionar Data Source PostgreSQL

1. Configuration → Data Sources → Add data source → PostgreSQL
2. Configurar:
   - Host: `localhost:5432`
   - Database: `aguada_db`
   - User: `aguada`
   - Password: `senha_segura`
   - SSL Mode: `disable` (local) ou `require` (produção)
   - Version: `15`

### 7.4 Importar Dashboard

```bash
# Copiar JSON do dashboard
cp aguada/dashboard/aguada_main.json /var/lib/grafana/dashboards/

# Ou importar via interface
# Dashboards → Import → Upload JSON file
```

---

## 8. Docker Compose (alternativa simplificada)

Se preferir usar Docker:

```yaml
# docker-compose.yml
# Note: this project uses non-default host ports to avoid conflicts with services
# that may already run on the host. Final host port mappings used in this
# workspace (local dev) are documented below.

version: '3.8'

services:
  postgres:
    image: timescale/timescaledb:latest-pg15
    environment:
      POSTGRES_DB: aguada_db
      POSTGRES_USER: aguada
      POSTGRES_PASSWORD: senha_segura
    volumes:
      - pgdata:/var/lib/postgresql/data
      - ./database/schema.sql:/docker-entrypoint-initdb.d/schema.sql
    ports:
      - "5432:5432"

  mosquitto:
    image: eclipse-mosquitto:2
    volumes:
      - ./config/mosquitto.conf:/mosquitto/config/mosquitto.conf
      - ./config/mosquitto_passwd:/mosquitto/config/passwd
    ports:
      - "1883:1883"
      - "9001:9001"

  backend:
    build: ./backend
    environment:
      DB_HOST: postgres
      MQTT_HOST: mqtt://mosquitto
    depends_on:
      - postgres
      - mosquitto
    ports:
      - "3000:3000"

  grafana:
    image: grafana/grafana:latest
    environment:
      GF_SECURITY_ADMIN_PASSWORD: admin
    volumes:
      - grafana-data:/var/lib/grafana
    ports:
      - "3001:3000"
    depends_on:
      - postgres

volumes:
  pgdata:
  grafana-data:

--

**Port mappings used in this setup (host → container):**

- `postgres`: `5433:5432` (host 5433) — avoids conflict with host Postgres
- `redis`: `6379:6379` (host 6379)
- `mosquitto`: `1884:1883` and `9002:9001` (hosts 1884, 9002)
- `backend`: `3002:3000` (host 3002)
- `grafana`: `3001:3000` (host 3001)

If you want the original/default ports, edit `docker/docker-compose.yml` accordingly, but ensure no host services already bind those ports.
```

Iniciar tudo:
```bash
docker-compose up -d
```

---

## 9. Configuração dos Nodes ESP32

### 9.1 Editar Configurações

```cpp
// main/config.h
#define WIFI_SSID "SUA_REDE"
#define WIFI_PASS "SENHA_WIFI"

#define MQTT_HOST "mqtt://192.168.1.50"
#define MQTT_PORT 1883
#define MQTT_USER "aguada_node"
#define MQTT_PASS "senha"
#define MQTT_TOPIC "aguada/telemetry"
```

### 9.2 Configurar Reservatório (SPIFFS)

Criar `config.json` e gravar no SPIFFS:
```json
{
  "device_id": "node_cons",
  "reservatorio": {
    "tipo": "cilindrico",
    "hsensor_cm": 40,
    "altura_cm": 400,
    "diametro_cm": 510
  },
  "deadband_cm": 2.0,
  "report_sec": 30
}
```

---

## 10. Verificação e Testes

### 10.1 Testar PostgreSQL
```bash
psql -U aguada -d aguada_db -c "SELECT version();"
```

### 10.2 Testar MQTT
```bash
mosquitto_sub -h localhost -t "aguada/#" -u aguada_backend -P senha
```

### 10.3 Testar Backend
```bash
curl -X POST http://localhost:3000/api/telemetry \
  -H "Content-Type: application/json" \
  -d '{"mac":"AA:BB:CC:DD:EE:FF","ts":1700000000,"data_label":"ultra1_cm","data_value":250,"battery":3.3,"rssi":-70}'
```

### 10.4 Testar Grafana
```
http://localhost:3000
Login: admin / <senha configurada>
```

---

## 11. Troubleshooting

### PostgreSQL não inicia
```bash
sudo journalctl -u postgresql -n 50
# Verificar logs e permissões
```

### MQTT conexão recusada
```bash
sudo systemctl status mosquitto
sudo tail -f /var/log/mosquitto/mosquitto.log
```

### Backend não conecta ao banco
```bash
# Verificar .env
# Testar conexão manual
psql -U aguada -h localhost -d aguada_db
```

### Node ESP32 não conecta WiFi
- Verificar SSID/senha
- Verificar força do sinal
- Monitor serial: `idf.py monitor`

---

## 12. Próximos Passos

1. ✅ Configurar backup automático do PostgreSQL
2. ✅ Configurar SSL/TLS para MQTT em produção
3. ✅ Implementar rate limiting na API
4. ✅ Configurar alertas no Grafana
5. ✅ Testar com dados reais dos sensores

---

**Versão:** 1.0  
**Data:** 2025-11-16  
**Autor:** AGUADA Team