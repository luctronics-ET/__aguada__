#!/bin/bash
# Script de inicialização do sistema AGUADA com dados reais
# Conecta gateway ESP32 via USB e inicia backend

set -e

echo "================================================"
echo "🚀 AGUADA - Inicialização com Dados Reais"
echo "================================================"
echo ""

# Cores
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

# Diretório base
AGUADA_DIR="/home/luciano/Área de trabalho/__aguada__"
cd "$AGUADA_DIR"

# 1. Verificar gateway USB
echo "1️⃣  Verificando gateway ESP32..."
if [ -e "/dev/ttyACM0" ]; then
    echo -e "${GREEN}✅ Gateway encontrado em /dev/ttyACM0${NC}"
    
    # Verificar permissões
    if [ -r "/dev/ttyACM0" ] && [ -w "/dev/ttyACM0" ]; then
        echo -e "${GREEN}✅ Permissões OK${NC}"
    else
        echo -e "${YELLOW}⚠️  Sem permissões. Adicionando usuário ao grupo dialout...${NC}"
        sudo usermod -a -G dialout $USER
        echo -e "${YELLOW}   Você precisará fazer logout/login para aplicar${NC}"
        echo -e "${YELLOW}   Temporariamente, execute: sudo chmod 666 /dev/ttyACM0${NC}"
        
        # Aplicar permissão temporária
        sudo chmod 666 /dev/ttyACM0 || true
    fi
else
    echo -e "${RED}❌ Gateway não encontrado em /dev/ttyACM0${NC}"
    echo ""
    echo "Verifique:"
    echo "  - Gateway está conectado via USB?"
    echo "  - Gateway está com firmware correto?"
    echo "  - Dispositivos disponíveis:"
    ls -la /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || echo "    (nenhum dispositivo serial encontrado)"
    echo ""
    exit 1
fi
echo ""

# 2. Verificar database
echo "2️⃣  Verificando PostgreSQL/TimescaleDB..."
if pg_isready -h 192.168.0.100 -p 5432 -U aguada_user -d aguada > /dev/null 2>&1; then
    echo -e "${GREEN}✅ Database online${NC}"
else
    echo -e "${YELLOW}⚠️  Database não acessível (continuando...)${NC}"
fi
echo ""

# 3. Verificar Redis
echo "3️⃣  Verificando Redis..."
if redis-cli -h 192.168.0.100 ping > /dev/null 2>&1; then
    echo -e "${GREEN}✅ Redis online${NC}"
else
    echo -e "${YELLOW}⚠️  Redis não acessível (continuando...)${NC}"
fi
echo ""

# 4. Instalar dependências (se necessário)
echo "4️⃣  Verificando dependências do backend..."
cd backend
if [ ! -d "node_modules" ]; then
    echo "   Instalando dependências..."
    npm install
else
    echo -e "${GREEN}✅ Dependências instaladas${NC}"
fi
cd ..
echo ""

# 5. Iniciar backend
echo "5️⃣  Iniciando backend com Serial Bridge..."
echo ""
echo "================================================"
echo "📡 BACKEND AGUADA - MODO DADOS REAIS"
echo "================================================"
echo ""
echo "Configuração:"
echo "  - Serial Port: /dev/ttyACM0"
echo "  - Baud Rate:   115200"
echo "  - Backend:     http://localhost:3000"
echo "  - Database:    192.168.0.100:5432"
echo ""
echo "O backend irá:"
echo "  1. Conectar ao gateway via USB serial"
echo "  2. Receber JSON de telemetria dos sensores"
echo "  3. Armazenar no PostgreSQL/TimescaleDB"
echo "  4. Transmitir via WebSocket para frontend"
echo ""
echo "Aguardando dados dos sensores ESP32..."
echo "================================================"
echo ""

cd backend
npm start
