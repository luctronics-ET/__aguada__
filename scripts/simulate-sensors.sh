#!/bin/bash
# Simula leituras de sensores ESP32 para testes

echo "🔄 Iniciando simulação de sensores..."
echo "   Pressione Ctrl+C para parar"

while true; do
    docker compose -f "/home/luciano/Área de trabalho/aguada/docker-compose.yml" exec -T postgres psql -U aguada -d aguada << 'SQL' > /dev/null 2>&1
INSERT INTO aguada.leituras_raw (sensor_id, elemento_id, variavel, valor, unidade, fonte, datetime)
SELECT 
    s.sensor_id,
    s.elemento_id,
    s.variavel,
    CASE 
        WHEN s.variavel LIKE '%distance%' THEN (4500 + random() * 1000)::NUMERIC(10,2)
        WHEN s.variavel LIKE '%valve%' OR s.variavel LIKE '%sound%' THEN ROUND(random())::NUMERIC(10,2)
        ELSE 0
    END,
    CASE WHEN s.variavel LIKE '%distance%' THEN 'cm' ELSE 'bool' END,
    'esp-now',
    NOW()
FROM aguada.sensores s;
SQL
    echo "✅ $(date '+%H:%M:%S') - 20 leituras inseridas"
    sleep 30
done
