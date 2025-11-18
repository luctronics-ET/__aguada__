#!/bin/bash

# AGUADA System Startup Script
# Inicia gateway, backend e frontend

set -e

PROJECT_ROOT="/home/luciano/Área de trabalho/__aguada__"
BACKEND_DIR="$PROJECT_ROOT/backend"
FRONTEND_DIR="$PROJECT_ROOT/frontend"
GATEWAY_DIR="$PROJECT_ROOT/firmware/gateway_esp_idf"

echo "╔══════════════════════════════════════════════════════════════════╗"
echo "║                   AGUADA System Startup                         ║"
echo "║              Monitoramento Hidráulico de Reservatórios          ║"
echo "╚══════════════════════════════════════════════════════════════════╝"
echo ""

# Color codes
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Function to print status
print_status() {
    echo -e "${GREEN}[✓]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[!]${NC} $1"
}

print_error() {
    echo -e "${RED}[✗]${NC} $1"
}

# Check prerequisites
echo -e "\n${YELLOW}1. Verificando pré-requisitos...${NC}"

if ! command -v node &> /dev/null; then
    print_error "Node.js não instalado"
    exit 1
fi
print_status "Node.js encontrado: $(node --version)"

if ! command -v npm &> /dev/null; then
    print_error "npm não instalado"
    exit 1
fi
print_status "npm encontrado: $(npm --version)"

if ! command -v idf.py &> /dev/null; then
    print_warning "ESP-IDF não no PATH (gateway não será recompilado)"
else
    print_status "ESP-IDF encontrado"
fi

# Backend setup
echo -e "\n${YELLOW}2. Preparando Backend...${NC}"

cd "$BACKEND_DIR"

if [ ! -d "node_modules" ]; then
    print_status "Instalando dependências..."
    npm install
else
    print_status "Dependências já instaladas"
fi

if [ ! -f ".env" ]; then
    print_warning ".env não encontrado, copiando de .env.example"
    cp .env.example .env
else
    print_status ".env configurado"
fi

print_status "Backend pronto"

# Frontend check
echo -e "\n${YELLOW}3. Verificando Frontend...${NC}"

if [ -f "$FRONTEND_DIR/index.html" ]; then
    print_status "Dashboard HTML encontrado"
else
    print_warning "Dashboard HTML não encontrado em $FRONTEND_DIR/index.html"
fi

# Gateway check
echo -e "\n${YELLOW}4. Verificando Gateway...${NC}"

if [ -f "$GATEWAY_DIR/build/aguada_gateway.bin" ]; then
    print_status "Gateway firmware compilado"
else
    print_warning "Gateway firmware não compilado. Execute: idf.py -C $GATEWAY_DIR build"
fi

# Display startup information
echo -e "\n${YELLOW}5. Informações de Inicialização:${NC}"

echo -e "
  Backend API:       http://192.168.0.100:3000/api
  Dashboard:         http://192.168.0.100:3000/dashboard
  Health Check:      http://192.168.0.100:3000/api/health
  
  Database:          PostgreSQL @ 192.168.0.100:5432
  Redis Cache:       @ 192.168.0.100:6379
  Gateway:           ESP32-C3 (MAC: 80:f1:b2:50:2e:c4)
  
  Sensores Nodes:    5 × ESP32-C3 SuperMini
  Protocolo:         ESP-NOW (CH1, 2.4GHz)
  Intervalo:         Heartbeat 30s (individual variables)
"

# Options
echo -e "${YELLOW}6. Próximas etapas:${NC}"
echo ""
echo "  Para INICIAR O BACKEND (desenvolvimento):"
echo "    cd $BACKEND_DIR"
echo "    npm run dev"
echo ""
echo "  Para INICIAR O BACKEND (produção):"
echo "    cd $BACKEND_DIR"
echo "    npm start"
echo ""
echo "  Para ACESSAR O DASHBOARD:"
echo "    Abra no navegador: file://$FRONTEND_DIR/index.html"
echo "    (Ou configure servidor web para servir frontend)"
echo ""
echo "  Para FLASHEAR O GATEWAY:"
echo "    cd $GATEWAY_DIR"
echo "    idf.py -p /dev/ttyACM0 flash monitor"
echo ""
echo "  Para FLASHEAR OS SENSORES:"
echo "    cd $PROJECT_ROOT/firmware/node_sensor_10"
echo "    idf.py -p /dev/ttyACM0 flash monitor"
echo ""

print_status "Sistema pronto para ser iniciado!"
echo ""
