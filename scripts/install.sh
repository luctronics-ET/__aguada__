#!/bin/bash

# AGUADA - Script de Instalação Completa
# Instala todos os componentes do sistema

set -e

echo "================================"
echo "  AGUADA - Instalação Completa"
echo "================================"
echo ""

# Cores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Funções auxiliares
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Verificar se é root
if [ "$EUID" -eq 0 ]; then 
    print_error "Não execute este script como root"
    exit 1
fi

# 1. Atualizar sistema
print_info "Atualizando sistema..."
sudo apt update
sudo apt upgrade -y

# 2. Instalar dependências básicas
print_info "Instalando dependências básicas..."
sudo apt install -y \
    git curl wget \
    build-essential \
    python3 python3-pip \
    cmake ninja-build \
    libssl-dev libffi-dev

# 3. Instalar Node.js
print_info "Instalando Node.js 18..."
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt install -y nodejs

# 4. Instalar PostgreSQL + TimescaleDB
print_info "Instalando PostgreSQL 15..."
sudo apt install -y postgresql-15 postgresql-contrib-15

print_info "Instalando TimescaleDB..."
echo "deb https://packagecloud.io/timescale/timescaledb/ubuntu/ $(lsb_release -c -s) main" | sudo tee /etc/apt/sources.list.d/timescaledb.list
wget --quiet -O - https://packagecloud.io/timescale/timescaledb/gpgkey | sudo apt-key add -
sudo apt update
sudo apt install -y timescaledb-2-postgresql-15

# Configurar TimescaleDB
sudo timescaledb-tune --quiet --yes

# 5. Instalar Redis
print_info "Instalando Redis..."
sudo apt install -y redis-server
sudo systemctl enable redis-server
sudo systemctl start redis-server

# 6. Instalar MQTT Broker (Mosquitto)
print_info "Instalando Mosquitto..."
sudo apt install -y mosquitto mosquitto-clients
sudo systemctl enable mosquitto
sudo systemctl start mosquitto

# 7. Instalar Grafana
print_info "Instalando Grafana..."
sudo apt-get install -y software-properties-common
sudo add-apt-repository -y "deb https://packages.grafana.com/oss/deb stable main"
wget -q -O - https://packages.grafana.com/gpg.key | sudo apt-key add -
sudo apt-get update
sudo apt-get install -y grafana
sudo systemctl enable grafana-server
sudo systemctl start grafana-server

# 8. Instalar ESP-IDF (opcional)
read -p "Instalar ESP-IDF para desenvolvimento firmware? (s/N) " -n 1 -r
echo
if [[ $REPLY =~ ^[Ss]$ ]]; then
    print_info "Instalando ESP-IDF..."
    mkdir -p ~/esp
    cd ~/esp
    git clone --recursive https://github.com/espressif/esp-idf.git
    cd esp-idf
    ./install.sh esp32c3
    print_info "Execute: source ~/esp/esp-idf/export.sh"
fi

# 9. Configurar banco de dados
print_info "Configurando banco de dados..."
cd "$(dirname "$0")/.."

sudo -u postgres psql << EOF
CREATE DATABASE aguada;
CREATE USER aguada_user WITH PASSWORD 'aguada_password';
GRANT ALL PRIVILEGES ON DATABASE aguada TO aguada_user;
\c aguada
CREATE EXTENSION IF NOT EXISTS timescaledb;
EOF

# Aplicar schema
sudo -u postgres psql -d aguada -f database/schema.sql

# 10. Instalar dependências do backend
print_info "Instalando dependências do backend..."
cd backend
npm install

# Copiar arquivo .env
if [ ! -f .env ]; then
    cp .env.example .env
    print_warn "Configure o arquivo backend/.env antes de executar"
fi

# 11. Configurar MQTT
print_info "Configurando MQTT..."
sudo bash -c 'cat > /etc/mosquitto/conf.d/aguada.conf << EOF
listener 1883
allow_anonymous false
password_file /etc/mosquitto/passwd
EOF'

# Criar usuário MQTT
sudo mosquitto_passwd -b -c /etc/mosquitto/passwd aguada_node mqtt_pass
sudo systemctl restart mosquitto

print_info ""
print_info "================================"
print_info "  ✓ Instalação Completa!"
print_info "================================"
print_info ""
print_info "Próximos passos:"
print_info "1. Configure backend/.env"
print_info "2. Configure firmware/node_10/main/config_pins.h"
print_info "3. Execute: cd backend && npm start"
print_info "4. Acesse Grafana: http://localhost:3000 (admin/admin)"
print_info ""
print_info "Serviços rodando:"
print_info "- PostgreSQL: porta 5432"
print_info "- Redis: porta 6379"
print_info "- MQTT: porta 1883"
print_info "- Grafana: porta 3000"
