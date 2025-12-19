#!/bin/bash
# Script para testar envio de telemetria

echo "Enviando dados de telemetria para http://localhost:5000/api/telemetry..."
echo ""

curl -X POST http://localhost:5000/api/telemetry \
  -H "Content-Type: application/json" \
  -d '{
    "mac": "80:F1:B2:50:31:34",
    "distance_mm": 2450,
    "vcc_bat_mv": 5000,
    "rssi": -50
  }' \
  -w "\nHTTP Status: %{http_code}\n"

echo ""
echo "Verificando dados salvos no banco..."
echo ""

sudo -u postgres psql -d aguada_db -c "SELECT leitura_id, sensor_id, elemento_id, variavel, valor, datetime FROM aguada.leituras_raw ORDER BY datetime DESC LIMIT 3;"

echo ""
echo "✓ Teste concluído!"
