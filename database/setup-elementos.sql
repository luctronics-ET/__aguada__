-- Script para criar tabela elementos e dados iniciais
-- Execute com: PGPASSWORD="postgres" psql -h localhost -p 5432 -U postgres -d aguada_db -f database/setup-elementos.sql

-- Criar tabela elementos se não existir
CREATE TABLE IF NOT EXISTS aguada.elementos (
  elemento_id VARCHAR(50) PRIMARY KEY,
  tipo VARCHAR(20) NOT NULL,
  nome VARCHAR(100) NOT NULL,
  descricao TEXT,
  coordenadas JSONB,
  parametros JSONB,
  status VARCHAR(20) DEFAULT 'ativo',
  criado_em TIMESTAMPTZ DEFAULT NOW(),
  atualizado_em TIMESTAMPTZ DEFAULT NOW()
);

-- Dar permissões ao usuário aguada_user
GRANT ALL PRIVILEGES ON TABLE aguada.elementos TO aguada_user;

-- Inserir elementos (reservatórios)
INSERT INTO aguada.elementos (elemento_id, tipo, nome, descricao, parametros, status) VALUES
  ('RCON', 'reservatorio_consumo', 'Castelo Consumo', 'Água potável para consumo - reservatório elevado cilíndrico de concreto armado de 80m³', 
   '{"nivel_max_mm": 4500, "area_base_m2": 17.77, "volume_max_m3": 80, "forma": "cilindrico", "material": "concreto_armado", "tipo_instalacao": "elevado"}', 'ativo'),
  ('RCAV', 'reservatorio_incendio', 'Castelo de Incêndio', 'Água doce para rede de combate a incêndio - reservatório elevado cilíndrico de concreto armado de 80m³', 
   '{"nivel_max_mm": 4500, "area_base_m2": 17.77, "volume_max_m3": 80, "forma": "cilindrico", "material": "concreto_armado", "tipo_instalacao": "elevado"}', 'ativo'),
  ('RB03', 'reservatorio_bombas_3', 'Reservatório Bombas Nº3', 'Água doce para escorva das 2 bombas da casa de bombas', 
   '{"nivel_max_mm": 4500, "area_base_m2": 17.77, "volume_max_m3": 80, "forma": "cilindrico", "material": "concreto_armado", "tipo_instalacao": "elevado"}', 'ativo'),
  ('IE01', 'reservatorio', 'Cisterna Ilha do Engenho Nº1', 'Cisterna subterrânea IE01 - água doce para abastecimento', 
    '{"nivel_max_mm": 5000, "area_base_m2": 49, "volume_max_m3": 245, "forma": "retangular", "material": "concreto", "tipo_instalacao": "subterraneo"}', 'ativo'),
    ('IE02', 'reservatorio', 'Cisterna Ilha do Engenho Nº2', 'Cisterna subterrânea IE02 - água doce para abastecimento', 
    '{"nivel_max_mm": 5000, "area_base_m2": 49, "volume_max_m3": 245, "forma": "retangular", "material": "concreto", "tipo_instalacao": "subterraneo"}', 'ativo')
ON CONFLICT (elemento_id) DO UPDATE SET 
  tipo = EXCLUDED.tipo,
  nome = EXCLUDED.nome,
  descricao = EXCLUDED.descricao,
  parametros = EXCLUDED.parametros,
  atualizado_em = NOW();

-- Atualizar sensores com elemento_id correto
UPDATE aguada.sensores SET elemento_id = 'RCON' WHERE sensor_id = 'SEN_CON_01';
UPDATE aguada.sensores SET elemento_id = 'RCAV' WHERE sensor_id = 'SEN_CAV_01';

-- Verificar dados
SELECT elemento_id, tipo, nome, status FROM aguada.elementos;
SELECT sensor_id, elemento_id, node_mac, variavel FROM aguada.sensores;
