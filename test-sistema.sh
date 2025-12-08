#!/bin/bash
# ==============================================================================
# AGUADA - Script de Testes Automatizados
# ==============================================================================

set -e

echo "=============================================================="
echo "🧪 AGUADA - Testes Automatizados do Sistema"
echo "=============================================================="
echo ""

PROJECT_ROOT=$(cd "$(dirname "$0")" && pwd)
cd "$PROJECT_ROOT"

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

BACKEND_URL="http://localhost:3000/api"
TEST_PASSED=0
TEST_FAILED=0

# ==============================================================================
# Funções de Teste
# ==============================================================================

test_endpoint() {
    local method=$1
    local endpoint=$2
    local data=$3
    local expected_status=$4
    local description=$5
    
    echo -n "  🔍 $description... "
    
    if [ "$method" = "POST" ]; then
        response=$(curl -s -w "\n%{http_code}" -X POST "$BACKEND_URL$endpoint" \
            -H "Content-Type: application/json" \
            -d "$data" 2>/dev/null)
    else
        response=$(curl -s -w "\n%{http_code}" "$BACKEND_URL$endpoint" 2>/dev/null)
    fi
    
    status_code=$(echo "$response" | tail -n 1)
    body=$(echo "$response" | head -n -1)
    
    if [ "$status_code" -eq "$expected_status" ]; then
        echo -e "${GREEN}✓${NC} ($status_code)"
        TEST_PASSED=$((TEST_PASSED + 1))
        return 0
    else
        echo -e "${RED}✗${NC} (esperado: $expected_status, obtido: $status_code)"
        TEST_FAILED=$((TEST_FAILED + 1))
        echo "       Resposta: $body"
        return 1
    fi
}

# ==============================================================================
# Testes
# ==============================================================================

echo "📡 Verificando conectividade..."
echo ""

# Teste 1: Health Check
test_endpoint "GET" "/health" "" 200 "Health Check"

# Teste 2: Listar sensores
test_endpoint "GET" "/sensors" "" 200 "Listar Sensores"

# Teste 3: Obter últimas leituras
test_endpoint "GET" "/readings/latest" "" 200 "Obter Últimas Leituras"

# Teste 4: Enviar telemetria - RCON distance
test_endpoint "POST" "/telemetry" \
    '{"mac":"20:6E:F1:6B:77:58","type":"distance_cm","value":24480,"battery":5000,"uptime":100,"rssi":-50}' \
    200 "Enviar Telemetria - RCON distance_cm"

# Teste 5: Enviar telemetria - RCON valve_in
test_endpoint "POST" "/telemetry" \
    '{"mac":"20:6E:F1:6B:77:58","type":"valve_in","value":1,"battery":5000,"uptime":100,"rssi":-50}' \
    200 "Enviar Telemetria - RCON valve_in"

# Teste 6: Enviar telemetria - RCAV distance
test_endpoint "POST" "/telemetry" \
    '{"mac":"DC:06:75:67:6A:CC","type":"distance_cm","value":18050,"battery":5000,"uptime":50,"rssi":-45}' \
    200 "Enviar Telemetria - RCAV distance_cm"

# Teste 7: Verificar alertas
test_endpoint "GET" "/alerts" "" 200 "Obter Alertas"

# Teste 8: Verificar estatísticas
test_endpoint "GET" "/stats/daily" "" 200 "Estatísticas Diárias"

echo ""
echo "=============================================================="
echo "📊 Resumo dos Testes"
echo "=============================================================="
echo -e "  ${GREEN}✓ Passou${NC}:   $TEST_PASSED"
echo -e "  ${RED}✗ Falhou${NC}:   $TEST_FAILED"
echo "  ${YELLOW}Total${NC}:     $((TEST_PASSED + TEST_FAILED))"
echo ""

if [ $TEST_FAILED -eq 0 ]; then
    echo -e "${GREEN}🎉 Todos os testes passaram!${NC}"
    echo ""
    echo "URLs de acesso:"
    echo "  Dashboard: http://localhost"
    echo "  API: http://localhost:3000/api"
    echo "  Grafana: http://localhost:3001"
    exit 0
else
    echo -e "${RED}⚠️  Alguns testes falharam!${NC}"
    echo ""
    echo "Verificar:"
    echo "  1. docker compose ps (verificar se containers estão rodando)"
    echo "  2. docker compose logs backend (verificar erros)"
    echo "  3. Banco de dados conectado"
    echo "  4. Redis rodando"
    exit 1
fi
