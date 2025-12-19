-- Execute este comando no terminal PostgreSQL
-- Copie e cole esta linha no psql:

\c aguada_db

INSERT INTO aguada.sensores (
  sensor_id, 
  elemento_id, 
  node_mac, 
  tipo, 
  modelo, 
  variavel, 
  unidade, 
  gpio_config, 
  status
) VALUES (
  'SEN_NODE11_01', 
  'RCON', 
  '80:F1:B2:50:31:34', 
  'ultrasonic', 
  'AJ-SR04M', 
  'distance_mm', 
  'mm', 
  '{"trig": 1, "echo": 0}', 
  'active'
) ON CONFLICT (sensor_id) DO UPDATE SET 
  node_mac = EXCLUDED.node_mac, 
  status = EXCLUDED.status;

SELECT * FROM aguada.sensores WHERE sensor_id = 'SEN_NODE11_01';
