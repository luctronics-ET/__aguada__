#!/bin/bash
# ==============================================================================
# AGUADA - Script de Setup Completo do Sistema
# ==============================================================================

set -e

echo "=============================================================="
echo "🚀 AGUADA - Setup Completo do Sistema"
echo "=============================================================="

PROJECT_ROOT=$(cd "$(dirname "$0")" && pwd)
cd "$PROJECT_ROOT"

echo "📁 Projeto: $PROJECT_ROOT"

# ==============================================================================
# 1. VERIFICAR PRÉ-REQUISITOS
# ==============================================================================
echo ""
echo "✓ Verificando pré-requisitos..."

# Verificar Node.js
if ! command -v node &> /dev/null; then
    echo "❌ Node.js não instalado"
    exit 1
fi
echo "  ✓ Node.js $(node --version)"

# Verificar npm
if ! command -v npm &> /dev/null; then
    echo "❌ npm não instalado"
    exit 1
fi
echo "  ✓ npm $(npm --version)"

# Verificar PostgreSQL
if ! command -v psql &> /dev/null; then
    echo "⚠️  PostgreSQL não encontrado no PATH (pode estar em container Docker)"
else
    echo "  ✓ PostgreSQL $(psql --version | head -1)"
fi

# ==============================================================================
# 2. INSTALAR DEPENDÊNCIAS
# ==============================================================================
echo ""
echo "📦 Instalando dependências..."

if [ -d "backend/node_modules" ]; then
    echo "  ✓ node_modules existe, pulando npm install"
else
    cd backend
    npm install --omit=dev
    cd "$PROJECT_ROOT"
    echo "  ✓ Dependências instaladas"
fi

# ==============================================================================
# 3. VERIFICAR BANCO DE DADOS
# ==============================================================================
echo ""
echo "🗄️  Verificando banco de dados..."

# Carregar variáveis do .env
if [ -f "backend/.env" ]; then
    export $(cat backend/.env | grep -v '^#' | xargs)
    echo "  ✓ Variáveis de ambiente carregadas"
else
    echo "❌ Arquivo backend/.env não encontrado"
    exit 1
fi

DB_HOST=${DB_HOST:-localhost}
DB_PORT=${DB_PORT:-5432}
DB_NAME=${DB_NAME:-aguada_db}
DB_USER=${DB_USER:-aguada_user}

echo "  Banco: $DB_HOST:$DB_PORT/$DB_NAME"
echo "  Usuário: $DB_USER"

# Tentar conectar (com timeout)
if command -v psql &> /dev/null; then
    echo "  Testando conexão..."
    if timeout 3 psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -c "SELECT 1" > /dev/null 2>&1; then
        echo "  ✓ Conexão com banco bem-sucedida"
    else
        echo "  ⚠️  Não foi possível conectar ao banco"
        echo "    Verifique se PostgreSQL está rodando em $DB_HOST:$DB_PORT"
    fi
else
    echo "  ⚠️  psql não disponível, pulando verificação"
fi

# ==============================================================================
# 4. VERIFICAR REDIS
# ==============================================================================
echo ""
echo "🔴 Verificando Redis..."

REDIS_HOST=${REDIS_HOST:-localhost}
REDIS_PORT=${REDIS_PORT:-6379}

echo "  Redis: $REDIS_HOST:$REDIS_PORT"

if command -v redis-cli &> /dev/null; then
    if timeout 2 redis-cli -h "$REDIS_HOST" -p "$REDIS_PORT" PING > /dev/null 2>&1; then
        echo "  ✓ Redis está rodando"
    else
        echo "  ⚠️  Redis não respondeu em $REDIS_HOST:$REDIS_PORT"
    fi
else
    echo "  ⚠️  redis-cli não disponível"
fi

# ==============================================================================
# 5. TESTAR BACKEND
# ==============================================================================
echo ""
echo "🔬 Testando backend..."

cd backend

# Tentar iniciar com timeout
if timeout 5 PORT=9999 node src/server.js > /tmp/backend-test.log 2>&1 &
then
    BACKEND_PID=$!
    sleep 2
    
    if ps -p $BACKEND_PID > /dev/null 2>&1; then
        echo "  ✓ Backend iniciou com sucesso (PID: $BACKEND_PID)"
        
        # Testar health endpoint
        if command -v curl &> /dev/null; then
            HEALTH=$(curl -s http://localhost:9999/api/health 2>/dev/null || echo "FAILED")
            if echo "$HEALTH" | grep -q "running"; then
                echo "  ✓ Health endpoint respondendo"
            else
                echo "  ⚠️  Health endpoint não respondeu corretamente"
            fi
        fi
        
        kill $BACKEND_PID 2>/dev/null || true
    else
        echo "  ❌ Backend falhou ao iniciar"
        cat /tmp/backend-test.log | tail -10
    fi
else
    echo "  ⚠️  Erro ao testar backend"
    cat /tmp/backend-test.log | tail -5
fi

cd "$PROJECT_ROOT"

# ==============================================================================
# 6. FRONTEND
# ==============================================================================
echo ""
echo "🌐 Verificando frontend..."

if [ -f "frontend/index.html" ]; then
    echo "  ✓ Frontend encontrado (frontend/index.html)"
else
    echo "  ❌ Frontend não encontrado"
fi

# ==============================================================================
# RESUMO
# ==============================================================================
echo ""
echo "=============================================================="
echo "✅ Setup Verificação Completa!"
echo "=============================================================="
echo ""
echo "Para iniciar o sistema:"
echo ""
echo "  1. Terminal 1 - Backend:"
echo "     cd backend && PORT=5000 npm run dev"
echo ""
echo "  2. Terminal 2 - Frontend (com live reload):"
echo "     cd frontend && python3 -m http.server 8000"
echo ""
echo "  3. Navegador:"
echo "     http://localhost:8000"
echo ""
echo "Ou use Docker Compose:"
echo "  docker compose up -d"
echo ""
echo "=============================================================="
