#!/bin/bash
# Script para verificar status do sistema

echo "=== STATUS DO SISTEMA AGUADA ==="
echo ""

echo "1. Backend (porta 5000):"
if curl -s http://localhost:5000/api/health > /dev/null 2>&1; then
    echo "   ✓ Backend rodando"
    curl -s http://localhost:5000/api/health | jq .
else
    echo "   ✗ Backend não está respondendo"
fi

echo ""
echo "2. PostgreSQL:"
if sudo -u postgres psql -d aguada_db -c "SELECT 1;" > /dev/null 2>&1; then
    echo "   ✓ PostgreSQL rodando"
else
    echo "   ✗ PostgreSQL não está acessível"
fi

echo ""
echo "3. Elementos cadastrados:"
sudo -u postgres psql -d aguada_db -c "SELECT COUNT(*) as total FROM aguada.elementos;"

echo ""
echo "4. Sensores cadastrados:"
sudo -u postgres psql -d aguada_db -c "SELECT sensor_id, elemento_id, node_mac, status FROM aguada.sensores;"

echo ""
echo "5. Leituras no banco:"
sudo -u postgres psql -d aguada_db -c "SELECT COUNT(*) as total FROM aguada.leituras_raw;"

echo ""
echo "=== FIM DO STATUS ==="
