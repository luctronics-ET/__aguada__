#!/bin/bash
# =============================================================================
# Script de Teste do Fluxo de Dados AGUADA
# Simula telemetria via Serial para testar o backend
# =============================================================================

set -e

# Cores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║         AGUADA - Teste de Fluxo de Dados                   ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════════╝${NC}"

# Configurações
BACKEND_URL="${BACKEND_URL:-http://localhost:5000}"
API_ENDPOINT="${BACKEND_URL}/api/telemetry"

# MACs dos sensores conhecidos (do banco de dados)
MAC_RCON="20:6E:F1:6B:77:58"
MAC_RCAV="DC:06:75:67:6A:CC"

# Função para enviar telemetria AGUADA-1
send_telemetry() {
    local mac="$1"
    local distance_mm="$2"
    local vcc_mv="${3:-5000}"
    local rssi="${4:--50}"
    
    local payload="{\"mac\":\"${mac}\",\"distance_mm\":${distance_mm},\"vcc_bat_mv\":${vcc_mv},\"rssi\":${rssi}}"
    
    echo -e "${YELLOW}📤 Enviando:${NC} ${payload}"
    
    response=$(curl -s -w "\n%{http_code}" -X POST "${API_ENDPOINT}" \
        -H "Content-Type: application/json" \
        -d "${payload}" 2>&1)
    
    http_code=$(echo "$response" | tail -n1)
    body=$(echo "$response" | head -n -1)
    
    if [ "$http_code" = "200" ] || [ "$http_code" = "201" ]; then
        echo -e "${GREEN}✅ OK (${http_code}): ${body}${NC}"
        return 0
    else
        echo -e "${RED}❌ Erro (${http_code}): ${body}${NC}"
        return 1
    fi
}

# Verificar backend
echo ""
echo -e "${BLUE}🔍 Verificando backend em ${BACKEND_URL}...${NC}"
if curl -s "${BACKEND_URL}/api/health" > /dev/null 2>&1; then
    echo -e "${GREEN}✅ Backend está rodando${NC}"
else
    echo -e "${RED}❌ Backend não está acessível em ${BACKEND_URL}${NC}"
    echo -e "${YELLOW}   Inicie o backend com: cd backend && npm run dev${NC}"
    exit 1
fi

# Verificar sensores cadastrados
echo ""
echo -e "${BLUE}🔍 Verificando sensores cadastrados...${NC}"
sensors=$(curl -s "${BACKEND_URL}/api/sensors" 2>/dev/null)
echo -e "${GREEN}   Sensores: ${sensors}${NC}"

# Teste 1: Enviar telemetria RCON
echo ""
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}Teste 1: Telemetria RCON (formato AGUADA-1)${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
send_telemetry "${MAC_RCON}" 2450 5000 -45

# Teste 2: Enviar telemetria RCAV
echo ""
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}Teste 2: Telemetria RCAV (formato AGUADA-1)${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
send_telemetry "${MAC_RCAV}" 1850 4900 -52

# Teste 3: Enviar várias leituras
echo ""
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}Teste 3: Sequência de leituras RCON${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
for dist in 2450 2465 2480 2495 2510; do
    send_telemetry "${MAC_RCON}" ${dist} 5000 -45
    sleep 0.5
done

# Verificar leituras salvas
echo ""
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}Verificando últimas leituras...${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
curl -s "${BACKEND_URL}/api/readings/latest" | python3 -m json.tool 2>/dev/null || \
    curl -s "${BACKEND_URL}/api/readings/latest"

# Verificar status dos sensores
echo ""
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}Status dos sensores:${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
curl -s "${BACKEND_URL}/api/status/sensors" | python3 -m json.tool 2>/dev/null || \
    curl -s "${BACKEND_URL}/api/status/sensors"

echo ""
echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║                 Teste concluído!                           ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}"
