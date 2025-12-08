#!/bin/bash

# AGUADA - Quick Test Verification Script
# Verifica se todos os componentes estão funcionando

set -e

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║          AGUADA - Sistema de Verificação                  ║"
echo "║        Monitoramento Hidráulico de 5 Reservatórios        ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Verification functions
check_gateway() {
    echo "🔍 Verificando Gateway..."
    
    # Check if gateway_esp_idf exists
    if [ -f "firmware/gateway_esp_idf/main/main.c" ]; then
        echo -e "${GREEN}✓ Gateway ESP-IDF (main.c) encontrado${NC}"
        
        # Check for critical functions
        if grep -q "espnow_recv_cb" firmware/gateway_esp_idf/main/main.c; then
            echo -e "${GREEN}✓ Callback ESP-NOW implementado${NC}"
        else
            echo -e "${RED}✗ Callback ESP-NOW não encontrado${NC}"
        fi
        
        if grep -q "http_post_task" firmware/gateway_esp_idf/main/main.c; then
            echo -e "${GREEN}✓ Task HTTP POST implementada${NC}"
        else
            echo -e "${RED}✗ Task HTTP POST não encontrada${NC}"
        fi
        
        if grep -q "xQueueCreate" firmware/gateway_esp_idf/main/main.c; then
            echo -e "${GREEN}✓ Queue Pipeline implementada${NC}"
        else
            echo -e "${RED}✗ Queue Pipeline não encontrada${NC}"
        fi
    else
        echo -e "${RED}✗ Gateway ESP-IDF não encontrado${NC}"
    fi
    
    echo ""
}

check_backend() {
    echo "🔍 Verificando Backend Node.js..."
    
    if [ -f "backend/src/server.js" ]; then
        echo -e "${GREEN}✓ Arquivo server.js encontrado${NC}"
        
        if grep -q "telemetryController.receiveTelemetry" backend/src/routes/api.routes.js; then
            echo -e "${GREEN}✓ Endpoint POST /api/telemetry implementado${NC}"
        else
            echo -e "${RED}✗ Endpoint de telemetria não encontrado${NC}"
        fi
        
        if grep -q "app.use.*limiter" backend/src/server.js; then
            echo -e "${GREEN}✓ Rate limiting configurado${NC}"
        else
            echo -e "${RED}✗ Rate limiting não configurado${NC}"
        fi
        
        if grep -q "cors" backend/src/server.js; then
            echo -e "${GREEN}✓ CORS habilitado${NC}"
        else
            echo -e "${RED}✗ CORS não habilitado${NC}"
        fi
    else
        echo -e "${RED}✗ Backend server.js não encontrado${NC}"
    fi
    
    echo ""
}

check_frontend() {
    echo "🔍 Verificando Frontend..."
    
    files=("index.html" "history.html" "alerts.html" "system.html" "config.html")
    assets=("assets/style.css" "assets/app.js")
    
    for file in "${files[@]}"; do
        if [ -f "frontend/$file" ]; then
            echo -e "${GREEN}✓ Página $file encontrada${NC}"
        else
            echo -e "${RED}✗ Página $file não encontrada${NC}"
        fi
    done
    
    for asset in "${assets[@]}"; do
        if [ -f "frontend/$asset" ]; then
            echo -e "${GREEN}✓ Asset $asset encontrado${NC}"
        else
            echo -e "${RED}✗ Asset $asset não encontrado${NC}"
        fi
    done
    
    echo ""
}

check_database() {
    echo "🔍 Verificando Database..."
    
    if [ -f "database/schema.sql" ]; then
        echo -e "${GREEN}✓ Schema SQL encontrado${NC}"
        
        if grep -q "leituras_raw" database/schema.sql; then
            echo -e "${GREEN}✓ Tabela leituras_raw definida${NC}"
        fi
        
        if grep -q "leituras_processadas" database/schema.sql; then
            echo -e "${GREEN}✓ Tabela leituras_processadas definida${NC}"
        fi
    else
        echo -e "${YELLOW}⚠ Schema SQL não encontrado${NC}"
    fi
    
    echo ""
}

check_dependencies() {
    echo "🔍 Verificando Dependências..."
    
    # Backend
    if [ -f "backend/package.json" ]; then
        echo -e "${GREEN}✓ package.json encontrado${NC}"
        
        if grep -q "express" backend/package.json; then
            echo -e "${GREEN}✓ Express configurado${NC}"
        fi
        
        if grep -q "pg" backend/package.json; then
            echo -e "${GREEN}✓ PostgreSQL driver configurado${NC}"
        fi
    fi
    
    echo ""
}

# Main execution
check_gateway
check_backend
check_frontend
check_database
check_dependencies

echo "╔════════════════════════════════════════════════════════════╗"
echo "║                   VERIFICAÇÃO CONCLUÍDA                    ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "📋 Próximos Passos:"
echo ""
echo "1. Backend:"
echo "   cd backend"
echo "   npm install"
echo "   npm run dev"
echo ""
echo "2. Frontend:"
echo "   Abrir em navegador: http://localhost:3000"
echo ""
echo "3. Flashing Firmware:"
echo "   cd firmware/gateway_esp_idf"
echo "   idf.py -p /dev/ttyACM0 flash monitor"
echo ""
echo "4. Testing:"
echo "   POST http://localhost:3000/api/telemetry"
echo "   Com payload JSON do sensor"
echo ""
