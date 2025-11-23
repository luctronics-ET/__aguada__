#!/bin/bash

# AGUADA - Script de Setup Automático
# Este script configura o ambiente do zero

set -e

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  AGUADA - Setup Automático do Ambiente                     ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Cores
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

# Diretório do projeto
PROJECT_DIR="/home/luciano/Área de trabalho/aguada"
cd "$PROJECT_DIR"

echo -e "${GREEN}📦 Passo 1: Verificando pré-requisitos...${NC}"

# Verificar Node.js
if ! command -v node &> /dev/null; then
    echo -e "${RED}❌ Node.js não encontrado. Instalando...${NC}"
    curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
    sudo apt install -y nodejs
fi

# Verificar PostgreSQL
if ! command -v psql &> /dev/null; then
    echo -e "${RED}❌ PostgreSQL não encontrado. Instalando...${NC}"
    sudo apt update
    sudo apt install -y postgresql postgresql-contrib
fi

echo -e "${GREEN}✅ Pré-requisitos OK${NC}"
echo ""

echo -e "${GREEN}🗄️  Passo 2: Configurando banco de dados...${NC}"

# Criar usuário e banco (se não existir)
sudo -u postgres psql << 'EOF' || true
DO $$
BEGIN
    IF NOT EXISTS (SELECT FROM pg_user WHERE usename = 'aguada_user') THEN
        CREATE USER aguada_user WITH PASSWORD 'aguada_pass_2025';
    END IF;
END
$$;

SELECT 'CREATE DATABASE aguada_db OWNER aguada_user'
WHERE NOT EXISTS (SELECT FROM pg_database WHERE datname = 'aguada_db')\gexec
EOF

# Criar schema básico
sudo -u postgres psql -d aguada_db << 'EOF'
CREATE SCHEMA IF NOT EXISTS aguada;

CREATE TABLE IF NOT EXISTS aguada.sensores (
    sensor_id VARCHAR(50) PRIMARY KEY,
    elemento_id VARCHAR(50) NOT NULL,
    node_mac VARCHAR(17),
    variavel VARCHAR(50) NOT NULL,
    tipo VARCHAR(50),
    status VARCHAR(20) DEFAULT 'ativo',
    ajuste_offset NUMERIC(10,2) DEFAULT 0,
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS aguada.leituras_raw (
    leitura_id SERIAL PRIMARY KEY,
    sensor_id VARCHAR(50) NOT NULL,
    elemento_id VARCHAR(50) NOT NULL,
    variavel VARCHAR(50) NOT NULL,
    valor NUMERIC(10,2) NOT NULL,
    unidade VARCHAR(10) DEFAULT 'cm',
    datetime TIMESTAMP DEFAULT NOW(),
    meta JSONB,
    fonte VARCHAR(20) DEFAULT 'sensor',
    autor VARCHAR(50),
    modo VARCHAR(20) DEFAULT 'automatica',
    observacao TEXT,
    FOREIGN KEY (sensor_id) REFERENCES aguada.sensores(sensor_id)
);

CREATE INDEX IF NOT EXISTS idx_leituras_datetime ON aguada.leituras_raw(datetime DESC);
CREATE INDEX IF NOT EXISTS idx_leituras_sensor ON aguada.leituras_raw(sensor_id, datetime DESC);

GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA aguada TO aguada_user;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA aguada TO aguada_user;

-- Inserir sensores de exemplo
INSERT INTO aguada.sensores (sensor_id, elemento_id, node_mac, variavel, tipo) VALUES
('SEN_CON_01', 'RCON', '20:6e:f1:6b:77:58', 'distance_cm', 'ultrassonico'),
('SEN_CAV_01', 'RCAV', 'dc:06:75:67:6a:cc', 'distance_cm', 'ultrassonico')
ON CONFLICT (sensor_id) DO NOTHING;
EOF

echo -e "${GREEN}✅ Banco de dados configurado${NC}"
echo ""

echo -e "${GREEN}🖥️  Passo 3: Configurando backend...${NC}"

cd "$PROJECT_DIR/backend"

# Criar .env se não existir
if [ ! -f .env ]; then
    cat > .env << 'EOF'
PORT=3000
NODE_ENV=development
DB_HOST=localhost
DB_PORT=5432
DB_USER=aguada_user
DB_PASSWORD=aguada_pass_2025
DB_NAME=aguada_db
SERIAL_PORT=/dev/ttyACM0
SERIAL_BAUD=115200
CORS_ORIGIN=*
EOF
    echo -e "${GREEN}✅ Arquivo .env criado${NC}"
fi

# Instalar dependências
if [ ! -d "node_modules" ]; then
    echo "Instalando dependências do backend..."
    npm install
    echo -e "${GREEN}✅ Dependências instaladas${NC}"
else
    echo -e "${YELLOW}⚠️  node_modules já existe. Pulando instalação.${NC}"
fi

echo -e "${GREEN}✅ Backend configurado${NC}"
echo ""

echo -e "${GREEN}📡 Passo 4: Verificando gateway USB...${NC}"

if [ -e /dev/ttyACM0 ]; then
    echo -e "${GREEN}✅ Gateway encontrado: /dev/ttyACM0${NC}"
    
    # Verificar permissões
    if [ ! -r /dev/ttyACM0 ] || [ ! -w /dev/ttyACM0 ]; then
        echo -e "${YELLOW}⚠️  Ajustando permissões...${NC}"
        sudo chmod 666 /dev/ttyACM0 || echo -e "${YELLOW}⚠️  Execute: sudo usermod -a -G dialout $USER${NC}"
    fi
else
    echo -e "${YELLOW}⚠️  Gateway não encontrado em /dev/ttyACM0${NC}"
    echo "   Conecte o gateway ESP32 via USB"
fi

echo ""
echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  ✅ SETUP CONCLUÍDO!                                       ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo "Próximos passos:"
echo "1. Iniciar backend: cd backend && npm start"
echo "2. Testar API: curl http://localhost:3000/api/health"
echo "3. Abrir frontend: python3 -m http.server 8080 (na pasta frontend-simple)"
echo ""
echo "Ver SETUP_COMPLETO.md para mais detalhes."

