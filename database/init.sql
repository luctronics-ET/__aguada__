#!/bin/bash

# AGUADA - Database Initialization Script
# Executa este script DENTRO do container PostgreSQL ou em um PostgreSQL local
# Usage: psql -U aguada_user -d aguada_db -f database/init.sql

-- =============================================================================
-- 1. CRIAR SCHEMA E EXTENSÕES
-- =============================================================================

CREATE SCHEMA IF NOT EXISTS aguada;
SET search_path = aguada, public;

CREATE EXTENSION IF NOT EXISTS timescaledb CASCADE;
CREATE EXTENSION IF NOT EXISTS postgis;

-- =============================================================================
-- 2. TABELAS DE CONFIGURAÇÃO
-- =============================================================================

CREATE TABLE IF NOT EXISTS sensores (
  sensor_id VARCHAR(20) PRIMARY KEY,
  mac_address VARCHAR(17) NOT NULL UNIQUE,
  nome VARCHAR(100) NOT NULL,
  tipo VARCHAR(50),
  altura_cm INT,
  diametro_cm INT,
  capacidade_m3 FLOAT,
  coordenadas JSONB,
  ativo BOOLEAN DEFAULT true,
  criado_em TIMESTAMPTZ DEFAULT NOW(),
  atualizado_em TIMESTAMPTZ DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS usuarios (
  usuario_id SERIAL PRIMARY KEY,
  nome VARCHAR(100) NOT NULL,
  email VARCHAR(100) UNIQUE NOT NULL,
  password_hash VARCHAR(255),
  role VARCHAR(20) DEFAULT 'viewer', -- 'admin', 'operator', 'viewer'
  ativo BOOLEAN DEFAULT true,
  criado_em TIMESTAMPTZ DEFAULT NOW(),
  ultimo_login TIMESTAMPTZ
);

-- =============================================================================
-- 3. TABELAS DE TELEMETRIA (HYPERTABLES)
-- =============================================================================

CREATE TABLE IF NOT EXISTS leituras_raw (
  tempo TIMESTAMPTZ NOT NULL,
  sensor_id VARCHAR(20) NOT NULL,
  mac_address VARCHAR(17),
  tipo_variavel VARCHAR(50) NOT NULL,
  valor_int INTEGER,
  bateria_mv INTEGER,
  uptime_seg INTEGER,
  rssi_dbm INTEGER,
  recebido_em TIMESTAMPTZ DEFAULT NOW()
);

-- Converter para hypertable (TimescaleDB)
SELECT create_hypertable('leituras_raw', 'tempo', if_not_exists => TRUE);

-- Índices para hypertable
CREATE INDEX IF NOT EXISTS idx_leituras_raw_sensor_tempo 
  ON leituras_raw (sensor_id, tempo DESC);
CREATE INDEX IF NOT EXISTS idx_leituras_raw_tipo_tempo 
  ON leituras_raw (tipo_variavel, tempo DESC);
CREATE INDEX IF NOT EXISTS idx_leituras_raw_mac 
  ON leituras_raw (mac_address, tempo DESC);

-- Política de compressão (após 7 dias, comprimir dados)
ALTER TABLE leituras_raw SET (
  timescaledb.compress,
  timescaledb.compress_interval = '7 days'
);

SELECT add_compression_policy('leituras_raw', INTERVAL '7 days', if_not_exists => TRUE);

-- =============================================================================
-- 4. TABELA DE LEITURAS PROCESSADAS
-- =============================================================================

CREATE TABLE IF NOT EXISTS leituras_processadas (
  tempo TIMESTAMPTZ NOT NULL,
  sensor_id VARCHAR(20) NOT NULL,
  tipo_variavel VARCHAR(50) NOT NULL,
  valor_medio FLOAT,
  valor_min FLOAT,
  valor_max FLOAT,
  quantidade_amostra INT,
  processado_em TIMESTAMPTZ DEFAULT NOW()
);

SELECT create_hypertable('leituras_processadas', 'tempo', if_not_exists => TRUE);

CREATE INDEX IF NOT EXISTS idx_leituras_processadas_sensor_tempo 
  ON leituras_processadas (sensor_id, tempo DESC);

-- =============================================================================
-- 5. TABELA DE EVENTOS/ALERTAS
-- =============================================================================

CREATE TABLE IF NOT EXISTS eventos (
  evento_id BIGSERIAL PRIMARY KEY,
  tempo TIMESTAMPTZ NOT NULL,
  sensor_id VARCHAR(20) NOT NULL,
  tipo_evento VARCHAR(50),
  severidade VARCHAR(20), -- 'info', 'warning', 'critical'
  mensagem TEXT,
  dados_json JSONB,
  reconhecido BOOLEAN DEFAULT false,
  reconhecido_em TIMESTAMPTZ,
  reconhecido_por VARCHAR(100)
);

SELECT create_hypertable('eventos', 'tempo', if_not_exists => TRUE);

CREATE INDEX IF NOT EXISTS idx_eventos_sensor_tempo 
  ON eventos (sensor_id, tempo DESC);
CREATE INDEX IF NOT EXISTS idx_eventos_severidade 
  ON eventos (severidade, tempo DESC);

-- =============================================================================
-- 6. TABELA DE CONFIGURAÇÕES
-- =============================================================================

CREATE TABLE IF NOT EXISTS configuracoes (
  config_id SERIAL PRIMARY KEY,
  chave VARCHAR(100) UNIQUE NOT NULL,
  valor TEXT,
  descricao TEXT,
  tipo VARCHAR(20), -- 'string', 'int', 'float', 'boolean', 'json'
  atualizado_em TIMESTAMPTZ DEFAULT NOW(),
  atualizado_por VARCHAR(100)
);

-- =============================================================================
-- 7. INSERIR SENSORES INICIAIS
-- =============================================================================

INSERT INTO sensores (sensor_id, mac_address, nome, tipo, altura_cm, capacidade_m3)
VALUES
  ('RCON', '20:6E:F1:6B:77:58', 'Reservatório RCON', 'cilindrico', 400, 80),
  ('RCAV', 'DC:06:75:67:6A:CC', 'Reservatório RCAV', 'cilindrico', 400, 80),
  ('RB03', 'AA:BB:CC:DD:EE:01', 'Reservatório RB03', 'cilindrico', 400, 80),
  ('IE01', 'AA:BB:CC:DD:EE:02', 'Reservatório IE01', 'cilindrico', 400, 80),
  ('IE02', 'AA:BB:CC:DD:EE:03', 'Reservatório IE02', 'cilindrico', 400, 80)
ON CONFLICT (sensor_id) DO NOTHING;

-- =============================================================================
-- 8. INSERIR USUÁRIOS INICIAIS
-- =============================================================================

INSERT INTO usuarios (nome, email, role)
VALUES
  ('Admin AGUADA', 'admin@aguada.local', 'admin'),
  ('Operador', 'operador@aguada.local', 'operator'),
  ('Visualizador', 'viewer@aguada.local', 'viewer')
ON CONFLICT (email) DO NOTHING;

-- =============================================================================
-- 9. INSERIR CONFIGURAÇÕES INICIAIS
-- =============================================================================

INSERT INTO configuracoes (chave, valor, descricao, tipo)
VALUES
  ('heartbeat_interval_sec', '30', 'Intervalo de heartbeat dos sensores', 'int'),
  ('deadband_cm', '2', 'Deadband para mudanças de distância', 'int'),
  ('http_timeout_sec', '5', 'Timeout para requisições HTTP', 'int'),
  ('data_retention_days', '90', 'Dias de retenção de dados brutos', 'int'),
  ('timezone', 'America/Sao_Paulo', 'Timezone do sistema', 'string'),
  ('gateway_mac', '80:F1:B2:50:2E:C4', 'MAC address do gateway', 'string'),
  ('espnow_channel', '1', 'Canal WiFi para ESP-NOW', 'int')
ON CONFLICT (chave) DO NOTHING;

-- =============================================================================
-- 10. VIEWS ÚTEIS
-- =============================================================================

CREATE OR REPLACE VIEW ultima_leitura_por_sensor AS
SELECT DISTINCT ON (sensor_id)
  sensor_id,
  tempo,
  tipo_variavel,
  valor_int,
  bateria_mv,
  rssi_dbm,
  EXTRACT(EPOCH FROM (NOW() - tempo)) as segundos_atras
FROM leituras_raw
ORDER BY sensor_id, tempo DESC;

CREATE OR REPLACE VIEW sensores_online AS
SELECT
  s.sensor_id,
  s.nome,
  CASE WHEN lr.tempo > NOW() - INTERVAL '2 minutes' THEN 'online' ELSE 'offline' END as status,
  EXTRACT(EPOCH FROM (NOW() - lr.tempo)) as segundos_offline
FROM sensores s
LEFT JOIN LATERAL (
  SELECT tempo FROM leituras_raw 
  WHERE leituras_raw.sensor_id = s.sensor_id 
  ORDER BY tempo DESC LIMIT 1
) lr ON true;

-- =============================================================================
-- 11. PERMISSÕES
-- =============================================================================

GRANT USAGE ON SCHEMA aguada TO aguada_user;
GRANT SELECT, INSERT, UPDATE ON ALL TABLES IN SCHEMA aguada TO aguada_user;
GRANT SELECT ON ALL TABLES IN SCHEMA public TO aguada_user;

-- =============================================================================
-- FIM DO SCRIPT
-- =============================================================================

\echo '✅ Database AGUADA inicializado com sucesso!'
\echo '📊 Sensores: 5 (RCON, RCAV, RB03, IE01, IE02)'
\echo '👤 Usuários: 3 (admin, operator, viewer)'
\echo '🚀 Pronto para receber telemetria!'
