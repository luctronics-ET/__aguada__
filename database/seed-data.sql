-- AGUADA - Seed Data
-- Insere dados iniciais para elementos e sensores

-- Inserir elementos (reservatórios)
INSERT INTO aguada.elementos (elemento_id, tipo, nome, descricao, parametros, status) VALUES
('RCON', 'reservatorio', 'Castelo de Consumo', 'Reservatório elevado de consumo', '{"altura_total_cm": 400, "diametro_cm": 200, "capacidade_m3": 12.57}', 'ativo'),
('RCAV', 'reservatorio', 'Castelo de Incêndio', 'Reservatório elevado de incêndio', '{"altura_total_cm": 350, "diametro_cm": 200, "capacidade_m3": 11.0}', 'ativo'),
('RB03', 'reservatorio', 'Casa de Bombas RB03', 'Reservatório da casa de bombas', '{"altura_total_cm": 300, "diametro_cm": 180, "capacidade_m3": 7.63}', 'ativo'),
('IE01', 'cisterna', 'Cisterna IE01', 'Cisterna enterrada 01', '{"altura_total_cm": 250, "comprimento_cm": 500, "largura_cm": 300, "capacidade_m3": 37.5}', 'ativo'),
('IE02', 'cisterna', 'Cisterna IE02', 'Cisterna enterrada 02', '{"altura_total_cm": 250, "comprimento_cm": 500, "largura_cm": 300, "capacidade_m3": 37.5}', 'ativo')
ON CONFLICT (elemento_id) DO NOTHING;

-- Inserir sensores
INSERT INTO aguada.sensores (sensor_id, elemento_id, node_mac, tipo, modelo, variavel, unidade, range_min, range_max, status) VALUES
('SEN_CON_01', 'RCON', '20:6E:F1:6B:77:58', 'ultrassonico', 'JSN-SR04T', 'distance_cm', 'cm', 0, 400, 'ativo'),
('SEN_CAV_01', 'RCAV', 'DC:06:75:67:6A:CC', 'ultrassonico', 'JSN-SR04T', 'distance_cm', 'cm', 0, 350, 'ativo'),
('SEN_B03_01', 'RB03', '80:F1:B2:50:31:34', 'ultrassonico', 'JSN-SR04T', 'distance_cm', 'cm', 0, 300, 'ativo'),
('SEN_IE01_01', 'IE01', '80:F1:B2:50:31:35', 'ultrassonico', 'JSN-SR04T', 'distance_cm', 'cm', 0, 250, 'ativo'),
('SEN_IE02_01', 'IE02', '80:F1:B2:50:31:36', 'ultrassonico', 'JSN-SR04T', 'distance_cm', 'cm', 0, 250, 'ativo')
ON CONFLICT (sensor_id) DO NOTHING;

-- Inserir algumas leituras de teste
INSERT INTO aguada.leituras_raw (sensor_id, elemento_id, variavel, valor, unidade, datetime, fonte, autor, modo) VALUES
('SEN_CON_01', 'RCON', 'distance_cm', 150.5, 'cm', NOW() - INTERVAL '5 minutes', 'sensor', '20:6E:F1:6B:77:58', 'automatica'),
('SEN_CON_01', 'RCON', 'distance_cm', 151.2, 'cm', NOW() - INTERVAL '4 minutes', 'sensor', '20:6E:F1:6B:77:58', 'automatica'),
('SEN_CON_01', 'RCON', 'distance_cm', 150.8, 'cm', NOW() - INTERVAL '3 minutes', 'sensor', '20:6E:F1:6B:77:58', 'automatica'),
('SEN_CON_01', 'RCON', 'distance_cm', 149.5, 'cm', NOW() - INTERVAL '2 minutes', 'sensor', '20:6E:F1:6B:77:58', 'automatica'),
('SEN_CON_01', 'RCON', 'distance_cm', 148.0, 'cm', NOW() - INTERVAL '1 minute', 'sensor', '20:6E:F1:6B:77:58', 'automatica'),
('SEN_CAV_01', 'RCAV', 'distance_cm', 180.0, 'cm', NOW() - INTERVAL '2 minutes', 'sensor', 'DC:06:75:67:6A:CC', 'automatica'),
('SEN_CAV_01', 'RCAV', 'distance_cm', 179.5, 'cm', NOW() - INTERVAL '1 minute', 'sensor', 'DC:06:75:67:6A:CC', 'automatica'),
('SEN_B03_01', 'RB03', 'distance_cm', 120.0, 'cm', NOW() - INTERVAL '1 minute', 'sensor', '80:F1:B2:50:31:34', 'automatica'),
('SEN_IE01_01', 'IE01', 'distance_cm', 100.0, 'cm', NOW() - INTERVAL '1 minute', 'sensor', '80:F1:B2:50:31:35', 'automatica'),
('SEN_IE02_01', 'IE02', 'distance_cm', 95.0, 'cm', NOW() - INTERVAL '1 minute', 'sensor', '80:F1:B2:50:31:36', 'automatica');

