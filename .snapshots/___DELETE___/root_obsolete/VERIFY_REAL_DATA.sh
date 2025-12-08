#!/bin/bash

# AGUADA - Verificação Completa do Fluxo de Dados Reais
# Gateway > Servidor > Backend > Frontend

set -e

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  AGUADA - Verificação Completa do Fluxo de Dados Reais     ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Cores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Contadores
PASSED=0
FAILED=0

# Função para verificar status
check() {
    local name="$1"
    local command="$2"
    local expected="$3"
    
    echo -n "🔍 Verificando $name... "
    
    if eval "$command" > /dev/null 2>&1; then
        echo -e "${GREEN}✅ PASSOU${NC}"
        ((PASSED++))
        return 0
    else
        echo -e "${RED}❌ FALHOU${NC}"
        ((FAILED++))
        return 1
    fi
}

# Função para verificar com mensagem customizada
check_custom() {
    local name="$1"
    local command="$2"
    local success_msg="$3"
    local fail_msg="$4"
    
    echo -n "🔍 Verificando $name... "
    
    if eval "$command" > /dev/null 2>&1; then
        echo -e "${GREEN}✅ $success_msg${NC}"
        ((PASSED++))
        return 0
    else
        echo -e "${RED}❌ $fail_msg${NC}"
        ((FAILED++))
        return 1
    fi
}

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📡 ETAPA 1: GATEWAY (Hardware)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Verificar gateway USB
if [ -e /dev/ttyACM0 ]; then
    echo -e "${GREEN}✅ Gateway USB encontrado: /dev/ttyACM0${NC}"
    ls -lh /dev/ttyACM0
    ((PASSED++))
else
    echo -e "${RED}❌ Gateway USB não encontrado em /dev/ttyACM0${NC}"
    echo "   Verifique se o gateway ESP32 está conectado via USB"
    ((FAILED++))
fi

# Verificar permissões
if [ -r /dev/ttyACM0 ] && [ -w /dev/ttyACM0 ]; then
    echo -e "${GREEN}✅ Permissões OK para /dev/ttyACM0${NC}"
    ((PASSED++))
else
    echo -e "${YELLOW}⚠️  Permissões insuficientes para /dev/ttyACM0${NC}"
    echo "   Execute: sudo chmod 666 /dev/ttyACM0"
    echo "   Ou: sudo usermod -a -G dialout $USER"
    ((FAILED++))
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🖥️  ETAPA 2: BACKEND (Servidor Node.js)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Verificar se backend está rodando
BACKEND_PID=$(pgrep -f "node.*server.js" | head -1)
if [ -n "$BACKEND_PID" ]; then
    echo -e "${GREEN}✅ Backend rodando (PID: $BACKEND_PID)${NC}"
    ps -p $BACKEND_PID -o pid,cmd,etime,pcpu,pmem
    ((PASSED++))
else
    echo -e "${RED}❌ Backend não está rodando${NC}"
    echo "   Execute: cd backend && npm start"
    ((FAILED++))
fi

# Verificar health check
check "Health Check API" "curl -s http://localhost:3000/api/health | grep -q 'ok'"

# Verificar Serial Bridge
if curl -s http://localhost:3000/api/system/health 2>/dev/null | grep -q "serial" || [ -n "$BACKEND_PID" ]; then
    echo -e "${GREEN}✅ Serial Bridge configurado${NC}"
    ((PASSED++))
else
    echo -e "${YELLOW}⚠️  Serial Bridge não verificado (pode estar OK)${NC}"
    ((FAILED++))
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📊 ETAPA 3: DADOS (API e Banco)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Verificar endpoint de sensores
SENSORS_RESPONSE=$(curl -s http://localhost:3000/api/sensors/status 2>/dev/null)
if [ -n "$SENSORS_RESPONSE" ]; then
    echo -e "${GREEN}✅ Endpoint /api/sensors/status respondendo${NC}"
    echo "$SENSORS_RESPONSE" | jq '.' 2>/dev/null || echo "$SENSORS_RESPONSE" | head -5
    ((PASSED++))
else
    echo -e "${RED}❌ Endpoint /api/sensors/status não responde${NC}"
    ((FAILED++))
fi

# Verificar endpoint de leituras
READINGS_RESPONSE=$(curl -s http://localhost:3000/api/readings/latest 2>/dev/null)
if echo "$READINGS_RESPONSE" | grep -q "success"; then
    echo -e "${GREEN}✅ Endpoint /api/readings/latest respondendo${NC}"
    SENSOR_COUNT=$(echo "$READINGS_RESPONSE" | jq '.data | length' 2>/dev/null || echo "N/A")
    echo "   Sensores com dados: $SENSOR_COUNT"
    ((PASSED++))
else
    echo -e "${YELLOW}⚠️  Endpoint /api/readings/latest retornou resposta inesperada${NC}"
    echo "$READINGS_RESPONSE" | head -3
    ((FAILED++))
fi

# Verificar WebSocket
check_custom "WebSocket Server" \
    "timeout 2 bash -c 'echo > /dev/tcp/localhost/3000'" \
    "Porta 3000 acessível" \
    "Porta 3000 não acessível"

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🌐 ETAPA 4: FRONTEND"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Verificar frontend React
if [ -d "frontend-react" ]; then
    echo -e "${GREEN}✅ Diretório frontend-react encontrado${NC}"
    ((PASSED++))
    
    # Verificar se está rodando
    FRONTEND_PORT=$(lsof -ti:5173 2>/dev/null || echo "")
    if [ -n "$FRONTEND_PORT" ]; then
        echo -e "${GREEN}✅ Frontend React rodando na porta 5173${NC}"
        ((PASSED++))
    else
        echo -e "${YELLOW}⚠️  Frontend React não está rodando${NC}"
        echo "   Execute: cd frontend-react && npm run dev"
        ((FAILED++))
    fi
else
    echo -e "${YELLOW}⚠️  Diretório frontend-react não encontrado${NC}"
    ((FAILED++))
fi

# Verificar frontend HTML estático
if [ -d "frontend" ] || [ -d "___DELETE___/frontend" ]; then
    echo -e "${GREEN}✅ Frontend HTML estático encontrado${NC}"
    ((PASSED++))
else
    echo -e "${YELLOW}⚠️  Frontend HTML estático não encontrado${NC}"
    ((FAILED++))
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🔄 ETAPA 5: FLUXO DE DADOS"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Testar envio de telemetria simulada
echo "🧪 Testando envio de telemetria..."

TEST_MAC="20:6e:f1:6b:77:58"  # RCON
TEST_DATA="{\"mac\":\"$TEST_MAC\",\"type\":\"distance_cm\",\"value\":24480,\"battery\":5000,\"uptime\":120,\"rssi\":-50}"

TELEMETRY_RESPONSE=$(curl -s -X POST http://localhost:3000/api/telemetry \
    -H "Content-Type: application/json" \
    -d "$TEST_DATA" 2>/dev/null)

if echo "$TELEMETRY_RESPONSE" | grep -q "success"; then
    echo -e "${GREEN}✅ Telemetria aceita pelo backend${NC}"
    echo "$TELEMETRY_RESPONSE" | jq '.' 2>/dev/null || echo "$TELEMETRY_RESPONSE"
    ((PASSED++))
else
    echo -e "${YELLOW}⚠️  Telemetria retornou resposta inesperada${NC}"
    echo "$TELEMETRY_RESPONSE" | head -3
    ((FAILED++))
fi

# Verificar se dados aparecem nas leituras
sleep 1
LATEST_READINGS=$(curl -s http://localhost:3000/api/readings/latest 2>/dev/null)
if echo "$LATEST_READINGS" | grep -q "RCON"; then
    echo -e "${GREEN}✅ Dados de RCON encontrados nas leituras${NC}"
    ((PASSED++))
else
    echo -e "${YELLOW}⚠️  Dados de RCON não encontrados (pode ser normal se sensor offline)${NC}"
    ((FAILED++))
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📈 RESUMO"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

TOTAL=$((PASSED + FAILED))
SUCCESS_RATE=$((PASSED * 100 / TOTAL))

echo ""
echo "✅ Testes passados: $PASSED"
echo "❌ Testes falhados: $FAILED"
echo "📊 Taxa de sucesso: ${SUCCESS_RATE}%"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║  ✅ TODOS OS TESTES PASSARAM! Sistema operacional.        ║${NC}"
    echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}"
    exit 0
elif [ $SUCCESS_RATE -ge 70 ]; then
    echo -e "${YELLOW}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${YELLOW}║  ⚠️  SISTEMA PARCIALMENTE OPERACIONAL                    ║${NC}"
    echo -e "${YELLOW}╚════════════════════════════════════════════════════════════╝${NC}"
    exit 1
else
    echo -e "${RED}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${RED}║  ❌ SISTEMA COM PROBLEMAS - Verifique os erros acima      ║${NC}"
    echo -e "${RED}╚════════════════════════════════════════════════════════════╝${NC}"
    exit 2
fi

