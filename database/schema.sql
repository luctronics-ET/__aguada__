-- AGUADA - Schema PostgreSQL/TimescaleDB
-- Versão: 1.0
-- Data: 2025-11-16

-- =============================================================================
-- 1. EXTENSÕES
-- =============================================================================

CREATE EXTENSION IF NOT EXISTS timescaledb;
-- Tentar criar extensão postgis apenas se estiver disponível no sistema
DO $$
BEGIN
  IF EXISTS (SELECT 1 FROM pg_available_extensions WHERE name = 'postgis') THEN
    CREATE EXTENSION IF NOT EXISTS postgis;
  ELSE
    RAISE NOTICE 'postgis extension not available on this server — skipping';
  END IF;
END
$$;

-- =============================================================================
-- 2. SCHEMA E USUÁRIOS
-- =============================================================================

CREATE SCHEMA IF NOT EXISTS aguada;
SET search_path = aguada, public;

-- =============================================================================
-- 3. TABELAS DE CONFIGURAÇÃO
-- =============================================================================

-- Usuários do sistema
CREATE TABLE usuarios (
  usuario_id SERIAL PRIMARY KEY,
  nome VARCHAR(100) NOT NULL,
  email VARCHAR(100) UNIQUE,
  role VARCHAR(20) NOT NULL,  -- 'admin', 'operator', 'viewer'
  ativo BOOLEAN DEFAULT true,
  criado_em TIMESTAMPTZ DEFAULT NOW(),
  ultimo_login TIMESTAMPTZ
);

-- Elementos hidráulicos (reservatórios, bombas, válvulas)
CREATE TABLE elementos (
  elemento_id VARCHAR(50) PRIMARY KEY,
  tipo VARCHAR(20) NOT NULL,  -- 'reservatorio', 'bomba', 'valvula', 'rede'
  nome VARCHAR(100) NOT NULL,
  descricao TEXT,
  coordenadas JSONB,  -- {x, y, z, lat, lon}
  parametros JSONB,   -- Parâmetros específicos do tipo
  status VARCHAR(20) DEFAULT 'ativo',
  criado_em TIMESTAMPTZ DEFAULT NOW(),
  atualizado_em TIMESTAMPTZ DEFAULT NOW()
);

-- Portas dos elementos (entradas/saídas)
CREATE TABLE portas (
  porta_id SERIAL PRIMARY KEY,
  elemento_id VARCHAR(50) REFERENCES elementos(elemento_id) ON DELETE CASCADE,
  nome VARCHAR(50) NOT NULL,  -- 'in01', 'out_az', etc.
  tipo VARCHAR(10) NOT NULL,  -- 'entrada', 'saida'
  descricao TEXT,
  UNIQUE(elemento_id, nome)
);

-- Conexões entre portas (grafo hidráulico)
CREATE TABLE conexoes (
  conexao_id SERIAL PRIMARY KEY,
  porta_origem_id INTEGER REFERENCES portas(porta_id),
  porta_destino_id INTEGER REFERENCES portas(porta_id),
  ativo BOOLEAN DEFAULT true,
  descricao TEXT,
  criado_em TIMESTAMPTZ DEFAULT NOW()
);

-- Sensores
CREATE TABLE sensores (
  sensor_id VARCHAR(50) PRIMARY KEY,
  elemento_id VARCHAR(50) REFERENCES elementos(elemento_id),
  node_mac VARCHAR(17) NOT NULL,  -- MAC address do ESP32
  tipo VARCHAR(20) NOT NULL,  -- 'ultrassonico', 'pressao', 'vazao'
  modelo VARCHAR(50),
  variavel VARCHAR(20) NOT NULL,  -- 'nivel_cm', 'pressao_bar'
  unidade VARCHAR(10),
  gpio_config JSONB,  -- {trig: 1, echo: 0}
  precisao NUMERIC(5,2),
  range_min NUMERIC(10,2),
  range_max NUMERIC(10,2),
  frequencia_leitura_sec INTEGER DEFAULT 10,
  ultima_calibracao TIMESTAMPTZ,
  ajuste_offset NUMERIC(10,2) DEFAULT 0,
  status VARCHAR(20) DEFAULT 'ativo',
  criado_em TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX idx_sensores_elemento ON sensores(elemento_id);
CREATE INDEX idx_sensores_mac ON sensores(node_mac);

-- Configurações por elemento (deadband, window_size, etc.)
CREATE TABLE elemento_configs (
  elemento_id VARCHAR(50) PRIMARY KEY REFERENCES elementos(elemento_id),
  deadband NUMERIC(10,2) DEFAULT 2.0,
  window_size INTEGER DEFAULT 11,
  stability_stddev NUMERIC(5,2) DEFAULT 0.5,
  nivel_critico_percent NUMERIC(5,2),
  nivel_alerta_percent NUMERIC(5,2),
  parametros_extras JSONB,
  atualizado_em TIMESTAMPTZ DEFAULT NOW()
);

-- =============================================================================
-- 4. TABELAS DE TELEMETRIA
-- =============================================================================

-- Leituras brutas (TODAS as leituras recebidas)
CREATE TABLE leituras_raw (
  leitura_id BIGSERIAL,
  sensor_id VARCHAR(50) NOT NULL REFERENCES sensores(sensor_id),
  elemento_id VARCHAR(50) NOT NULL REFERENCES elementos(elemento_id),
  variavel VARCHAR(20) NOT NULL,
  valor NUMERIC(10,2) NOT NULL,
  unidade VARCHAR(10),
  meta JSONB,  -- {battery, rssi, temperature, etc.}
  fonte VARCHAR(20) NOT NULL,  -- 'sensor', 'usuario', 'sistema'
  autor VARCHAR(100),  -- node_mac, username, process_name
  modo VARCHAR(20),  -- 'automatica', 'manual'
  observacao TEXT,
  datetime TIMESTAMPTZ NOT NULL,
  processed BOOLEAN DEFAULT FALSE,
  criado_em TIMESTAMPTZ DEFAULT NOW()
);

-- Converter para hypertable (TimescaleDB)
SELECT create_hypertable('leituras_raw', 'datetime', if_not_exists => TRUE);

-- Índices
CREATE INDEX idx_leituras_raw_sensor_datetime ON leituras_raw(sensor_id, datetime DESC);
CREATE INDEX idx_leituras_raw_elemento_datetime ON leituras_raw(elemento_id, datetime DESC);
CREATE INDEX idx_leituras_raw_processed ON leituras_raw(processed) WHERE NOT processed;

-- Índices otimizados para queries frequentes (Fase 1 - Melhorias)
-- Índice composto para queries recentes (sem função NOW() no predicado)
CREATE INDEX IF NOT EXISTS idx_leituras_raw_latest 
ON leituras_raw(sensor_id, variavel, datetime DESC);

-- Índice composto para queries de leituras não processadas
CREATE INDEX IF NOT EXISTS idx_leituras_raw_processed_datetime 
ON leituras_raw(processed, datetime) 
WHERE processed = false;

-- Índice para queries por sensor e variável (otimiza histórico)
CREATE INDEX IF NOT EXISTS idx_leituras_raw_sensor_variavel_datetime 
ON leituras_raw(sensor_id, variavel, datetime DESC);

-- Leituras processadas (APENAS mudanças significativas)
CREATE TABLE leituras_processadas (
  proc_id BIGSERIAL,
  elemento_id VARCHAR(50) NOT NULL REFERENCES elementos(elemento_id),
  variavel VARCHAR(20) NOT NULL,
  valor NUMERIC(10,2) NOT NULL,
  unidade VARCHAR(10),
  volume_m3 NUMERIC(10,3),  -- Calculado se aplicável
  percentual NUMERIC(5,2),  -- Percentual de ocupação
  criterio VARCHAR(100),  -- Ex: 'estavel stddev<0.5'
  variacao NUMERIC(10,2),  -- Delta em relação ao anterior
  data_inicio TIMESTAMPTZ NOT NULL,
  data_fim TIMESTAMPTZ NOT NULL,
  fonte VARCHAR(20),
  autor VARCHAR(100),
  meta JSONB,  -- {min, max, stddev, count}
  criado_em TIMESTAMPTZ DEFAULT NOW()
);

SELECT create_hypertable('leituras_processadas', 'data_fim', if_not_exists => TRUE);

CREATE INDEX idx_leituras_proc_elemento ON leituras_processadas(elemento_id, data_fim DESC);

-- Estados de bombas e válvulas
CREATE TABLE estados_equipamentos (
  estado_id BIGSERIAL,
  elemento_id VARCHAR(50) NOT NULL REFERENCES elementos(elemento_id),
  tipo VARCHAR(10) NOT NULL,  -- 'bomba', 'valvula'
  estado VARCHAR(10) NOT NULL,  -- 'ON'/'OFF', 'ABERTA'/'FECHADA'
  fonte VARCHAR(20),
  autor VARCHAR(100),
  datetime TIMESTAMPTZ NOT NULL,
  criado_em TIMESTAMPTZ DEFAULT NOW()
);

SELECT create_hypertable('estados_equipamentos', 'datetime', if_not_exists => TRUE);

CREATE INDEX idx_estados_elemento ON estados_equipamentos(elemento_id, datetime DESC);

-- =============================================================================
-- 5. TABELAS DE EVENTOS E ALERTAS
-- =============================================================================

-- Eventos detectados (abastecimento, consumo, vazamento)
CREATE TABLE eventos (
  evento_id BIGSERIAL PRIMARY KEY,
  tipo VARCHAR(50) NOT NULL,
  elemento_id VARCHAR(50) REFERENCES elementos(elemento_id),
  detalhe JSONB,  -- Volume, duração, elementos envolvidos
  causa_provavel TEXT,
  nivel_confianca NUMERIC(3,2),  -- 0.0 a 1.0
  detectado_por VARCHAR(50),
  datetime_inicio TIMESTAMPTZ NOT NULL,
  datetime_fim TIMESTAMPTZ,
  criado_em TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX idx_eventos_elemento_tipo ON eventos(elemento_id, tipo);
CREATE INDEX idx_eventos_datetime ON eventos(datetime_inicio DESC);

-- Anomalias em investigação
CREATE TABLE anomalias (
  anomalia_id BIGSERIAL PRIMARY KEY,
  tipo VARCHAR(50) NOT NULL,
  elemento_id VARCHAR(50) REFERENCES elementos(elemento_id),
  descricao TEXT,
  nivel_alerta VARCHAR(20),  -- 'LEVE', 'MODERADO', 'CRITICO'
  inicio TIMESTAMPTZ NOT NULL,
  fim TIMESTAMPTZ,
  status VARCHAR(20) DEFAULT 'ativo',  -- 'ativo', 'resolvido', 'falso_positivo'
  detectado_por VARCHAR(50),
  resolvido_por VARCHAR(100),
  observacoes TEXT,
  criado_em TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX idx_anomalias_status ON anomalias(status, inicio DESC);

-- =============================================================================
-- 6. CALIBRAÇÃO E AUDITORIA
-- =============================================================================

-- Calibrações de sensores
CREATE TABLE calibracoes (
  calibracao_id SERIAL PRIMARY KEY,
  sensor_id VARCHAR(50) REFERENCES sensores(sensor_id),
  elemento_id VARCHAR(50) REFERENCES elementos(elemento_id),
  responsavel_usuario_id INTEGER REFERENCES usuarios(usuario_id),
  valor_referencia NUMERIC(10,2) NOT NULL,
  valor_sensor NUMERIC(10,2) NOT NULL,
  ajuste_aplicado NUMERIC(10,2),
  tipo VARCHAR(20),  -- 'manual', 'automatica'
  observacao TEXT,
  datetime TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX idx_calibracoes_sensor ON calibracoes(sensor_id, datetime DESC);

-- Log de auditoria
CREATE TABLE auditoria (
  log_id BIGSERIAL,
  tabela VARCHAR(50),
  operacao VARCHAR(10),  -- 'INSERT', 'UPDATE', 'DELETE'
  registro_id VARCHAR(50),
  usuario VARCHAR(100),
  ip VARCHAR(45),
  dados_anteriores JSONB,
  dados_novos JSONB,
  datetime TIMESTAMPTZ DEFAULT NOW()
);

SELECT create_hypertable('auditoria', 'datetime', if_not_exists => TRUE);

CREATE INDEX idx_auditoria_tabela ON auditoria(tabela, datetime DESC);

-- =============================================================================
-- 7. RELATÓRIOS E CONSUMO
-- =============================================================================

-- Relatórios diários gerados às 06:00
CREATE TABLE relatorios_diarios (
  relatorio_id SERIAL PRIMARY KEY,
  data DATE UNIQUE NOT NULL,
  volume_consumido_total_l NUMERIC(10,2),
  volume_abastecido_total_l NUMERIC(10,2),
  eventos_registrados INTEGER,
  anomalias_detectadas INTEGER,
  consumo_por_periodo JSONB,  -- {00-06h, 06-12h, 12-18h, 18-24h}
  resumo TEXT,
  arquivo_pdf TEXT,  -- Caminho do PDF gerado
  gerado_em TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX idx_relatorios_data ON relatorios_diarios(data DESC);

-- Consumo diário por reservatório
CREATE TABLE consumo_diario (
  consumo_id SERIAL PRIMARY KEY,
  data DATE NOT NULL,
  elemento_id VARCHAR(50) REFERENCES elementos(elemento_id),
  volume_inicial_m3 NUMERIC(10,3),
  volume_final_m3 NUMERIC(10,3),
  consumo_total_l NUMERIC(10,2),
  abastecimento_total_l NUMERIC(10,2),
  consumo_por_periodo JSONB,
  UNIQUE(data, elemento_id)
);

CREATE INDEX idx_consumo_data_elemento ON consumo_diario(data DESC, elemento_id);

-- =============================================================================
-- 8. FUNÇÕES E TRIGGERS
-- =============================================================================

-- Função para atualizar timestamp
CREATE OR REPLACE FUNCTION atualizar_timestamp()
RETURNS TRIGGER AS $$
BEGIN
  NEW.atualizado_em = NOW();
  RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- Trigger para elementos
CREATE TRIGGER trigger_elementos_timestamp
BEFORE UPDATE ON elementos
FOR EACH ROW EXECUTE FUNCTION atualizar_timestamp();

-- Função para processar leituras e comprimir dados
CREATE OR REPLACE FUNCTION processar_leitura(
  p_sensor_id VARCHAR,
  p_elemento_id VARCHAR,
  p_variavel VARCHAR,
  p_valor NUMERIC,
  p_datetime TIMESTAMPTZ
)
RETURNS VOID AS $$
DECLARE
  v_config RECORD;
  v_ultimo RECORD;
  v_delta NUMERIC;
BEGIN
  -- Buscar configuração do elemento
  SELECT * INTO v_config FROM elemento_configs WHERE elemento_id = p_elemento_id;
  
  IF NOT FOUND THEN
    v_config.deadband := 2.0;
  END IF;
  
  -- Buscar última leitura processada
  SELECT * INTO v_ultimo FROM leituras_processadas
  WHERE elemento_id = p_elemento_id AND variavel = p_variavel
  ORDER BY data_fim DESC LIMIT 1;
  
  IF FOUND THEN
    v_delta := ABS(p_valor - v_ultimo.valor);
    
    -- Se dentro do deadband, estender período
    IF v_delta <= v_config.deadband THEN
      UPDATE leituras_processadas
      SET data_fim = p_datetime
      WHERE proc_id = v_ultimo.proc_id;
      RETURN;
    END IF;
  END IF;
  
  -- Se mudança significativa ou primeira leitura, inserir novo registro
  INSERT INTO leituras_processadas (
    elemento_id, variavel, valor, variacao,
    data_inicio, data_fim, fonte, autor
  ) VALUES (
    p_elemento_id, p_variavel, p_valor, v_delta,
    p_datetime, p_datetime, 'sistema', 'processar_leitura'
  );
END;
$$ LANGUAGE plpgsql;

-- =============================================================================
-- 9. VIEWS ÚTEIS
-- =============================================================================

-- View de últimas leituras por elemento
CREATE OR REPLACE VIEW v_ultimas_leituras AS
SELECT DISTINCT ON (l.elemento_id, l.variavel)
  l.elemento_id,
  e.nome as elemento_nome,
  l.variavel,
  l.valor,
  l.unidade,
  l.meta,
  l.datetime,
  l.fonte
FROM leituras_raw l
JOIN elementos e ON l.elemento_id = e.elemento_id
ORDER BY l.elemento_id, l.variavel, l.datetime DESC;

-- View de status atual dos reservatórios
CREATE OR REPLACE VIEW v_status_reservatorios AS
SELECT
  e.elemento_id,
  e.nome,
  l.valor as nivel_cm,
  l.meta->>'volume_m3' as volume_m3,
  l.meta->>'percentual' as percentual,
  l.datetime as ultima_leitura,
  CASE
    WHEN (l.meta->>'percentual')::numeric < 10 THEN 'CRITICO'
    WHEN (l.meta->>'percentual')::numeric < 20 THEN 'ALERTA'
    ELSE 'NORMAL'
  END as status
FROM elementos e
JOIN LATERAL (
  SELECT * FROM leituras_raw
  WHERE elemento_id = e.elemento_id AND variavel = 'nivel_cm'
  ORDER BY datetime DESC LIMIT 1
) l ON true
WHERE e.tipo = 'reservatorio'
ORDER BY e.elemento_id;

-- =============================================================================
-- 10. POLÍTICAS DE RETENÇÃO (TimescaleDB)
-- =============================================================================

-- Retenção de 180 dias para leituras_raw
SELECT add_retention_policy('leituras_raw', INTERVAL '180 days');

-- Retenção de 2 anos para leituras_processadas
SELECT add_retention_policy('leituras_processadas', INTERVAL '730 days');

-- Habilitar compressão nas hypertables
ALTER TABLE leituras_raw SET (
  timescaledb.compress,
  timescaledb.compress_segmentby = 'sensor_id,variavel',
  timescaledb.compress_orderby = 'datetime DESC'
);

ALTER TABLE leituras_processadas SET (
  timescaledb.compress,
  timescaledb.compress_segmentby = 'elemento_id,variavel',
  timescaledb.compress_orderby = 'data_inicio DESC'
);

-- Compressão automática após 7 dias
SELECT add_compression_policy('leituras_raw', INTERVAL '7 days');
SELECT add_compression_policy('leituras_processadas', INTERVAL '7 days');

-- =============================================================================
-- 11. DADOS INICIAIS (EXEMPLO)
-- =============================================================================

-- Inserir usuário admin
INSERT INTO usuarios (nome, email, role) VALUES
('Admin', 'admin@aguada.local', 'admin');

-- Comentários
COMMENT ON TABLE elementos IS 'Elementos da rede hidráulica (reservatórios, bombas, válvulas)';
COMMENT ON TABLE leituras_raw IS 'Todas as leituras recebidas dos sensores (sem filtro)';
COMMENT ON TABLE leituras_processadas IS 'Leituras comprimidas (apenas mudanças significativas)';
COMMENT ON TABLE eventos IS 'Eventos detectados automaticamente (abastecimento, consumo, vazamento)';
COMMENT ON TABLE anomalias IS 'Anomalias em investigação';

-- =============================================================================
-- FIM DO SCHEMA
-- =============================================================================
