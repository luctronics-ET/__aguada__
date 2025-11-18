#!/bin/bash

# =============================================================================
# AGUADA - DEPLOY SCRIPT PORTÁVEL
# Deploy completo em nova máquina (Linux/Mac/WSL)
# =============================================================================

set -e  # Exit on error

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Funções de logging
log_info() { echo -e "${BLUE}ℹ️  $1${NC}"; }
log_success() { echo -e "${GREEN}✅ $1${NC}"; }
log_warning() { echo -e "${YELLOW}⚠️  $1${NC}"; }
log_error() { echo -e "${RED}❌ $1${NC}"; }

# =============================================================================
# 1. VERIFICAÇÕES INICIAIS
# =============================================================================

log_info "Iniciando deploy do AGUADA..."

# Verificar se está no diretório correto
if [ ! -f "docker-compose.yml" ]; then
    log_error "Arquivo docker-compose.yml não encontrado!"
    log_info "Execute este script no diretório raiz do AGUADA"
    exit 1
fi

log_success "Diretório verificado ✓"

# =============================================================================
# 2. DETECTAR SISTEMA OPERACIONAL
# =============================================================================

OS_TYPE=$(uname -s)
log_info "Sistema detectado: $OS_TYPE"

# =============================================================================
# 3. VERIFICAR/INSTALAR DOCKER
# =============================================================================

log_info "Verificando Docker..."

if ! command -v docker &> /dev/null; then
    log_warning "Docker não encontrado. Instalando..."
    
    if [ "$OS_TYPE" = "Darwin" ]; then
        log_info "Baixar Docker Desktop: https://www.docker.com/products/docker-desktop"
        exit 1
    elif [ "$OS_TYPE" = "Linux" ]; then
        curl -fsSL https://get.docker.com -o get-docker.sh
        sudo sh get-docker.sh
        sudo usermod -aG docker $USER
        log_success "Docker instalado ✓"
        log_warning "Você precisa fazer logout e login novamente para usar docker sem sudo"
    fi
else
    log_success "Docker encontrado ✓"
fi

# =============================================================================
# 4. VERIFICAR/INSTALAR DOCKER-COMPOSE
# =============================================================================

log_info "Verificando Docker Compose..."

if ! command -v docker-compose &> /dev/null; then
    log_warning "Docker Compose não encontrado. Instalando..."
    sudo curl -L "https://github.com/docker/compose/releases/latest/download/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
    sudo chmod +x /usr/local/bin/docker-compose
    log_success "Docker Compose instalado ✓"
else
    log_success "Docker Compose encontrado ✓"
fi

# =============================================================================
# 5. CRIAR ARQUIVOS DE CONFIGURAÇÃO
# =============================================================================

log_info "Configurando ambiente..."

# .env para o backend
if [ ! -f "backend/.env" ]; then
    cat > backend/.env << 'EOF'
# Backend Configuration
NODE_ENV=production
PORT=3000
LOG_LEVEL=info

# Database
DB_HOST=postgres
DB_PORT=5432
DB_USER=aguada_user
DB_PASSWORD=aguada_pass_2025
DB_NAME=aguada_db

# Redis
REDIS_HOST=redis
REDIS_PORT=6379
REDIS_DB=0

# Security
CORS_ORIGIN=*
JWT_SECRET=$(openssl rand -base64 32)

# API
API_TIMEOUT=30000
MAX_REQUEST_SIZE=20mb

# Logging
LOG_FORMAT=json
LOG_DIR=./logs
EOF
    log_success ".env criado ✓"
else
    log_success ".env já existe ✓"
fi

# =============================================================================
# 6. CRIAR DIRETÓRIOS NECESSÁRIOS
# =============================================================================

log_info "Criando diretórios..."
mkdir -p backend/logs
mkdir -p docker/certs
mkdir -p database/backups
log_success "Diretórios criados ✓"

# =============================================================================
# 7. BUILD IMAGES
# =============================================================================

log_info "Building Docker images..."
docker-compose build
log_success "Images built ✓"

# =============================================================================
# 8. INICIAR CONTAINERS
# =============================================================================

log_info "Iniciando containers..."
docker-compose up -d

log_info "Aguardando serviços iniciarem..."
sleep 10

# =============================================================================
# 9. VERIFICAR SAÚDE DOS SERVIÇOS
# =============================================================================

log_info "Verificando saúde dos serviços..."

# PostgreSQL
if docker exec aguada-postgres pg_isready -U aguada_user -d aguada_db &>/dev/null; then
    log_success "PostgreSQL ✓"
else
    log_error "PostgreSQL não respondendo"
fi

# Redis
if docker exec aguada-redis redis-cli ping &>/dev/null | grep -q "PONG"; then
    log_success "Redis ✓"
else
    log_error "Redis não respondendo"
fi

# Backend
if curl -s http://localhost:3000/api/health &>/dev/null; then
    log_success "Backend ✓"
else
    log_warning "Backend ainda inicializando..."
fi

# =============================================================================
# 10. INFORMAÇÕES FINAIS
# =============================================================================

echo ""
log_success "=========================================="
log_success "AGUADA Deploy Completo! 🎉"
log_success "=========================================="
echo ""

log_info "Acessar o sistema:"
echo "  📱 Frontend:  http://localhost"
echo "  🔌 Backend:   http://localhost:3000/api"
echo "  🐘 PostgreSQL: localhost:5432"
echo "  💾 Redis:     localhost:6379"
echo ""

log_info "Logs e Monitoramento:"
echo "  Backend logs:    docker logs -f aguada-backend"
echo "  PostgreSQL logs: docker logs -f aguada-postgres"
echo "  Todos os logs:   docker-compose logs -f"
echo ""

log_info "Comandos úteis:"
echo "  Parar sistema:       docker-compose down"
echo "  Resetar banco:       docker-compose down -v"
echo "  Ver status:          docker-compose ps"
echo "  Acessar DB:          docker exec -it aguada-postgres psql -U aguada_user -d aguada_db"
echo ""

log_warning "PRÓXIMOS PASSOS:"
echo "  1. Configure os sensores para enviar para: http://[IP_DO_COMPUTADOR]:3000/api/telemetry"
echo "  2. Verifique que dados estão chegando:"
echo "     curl http://localhost:3000/api/readings/latest"
echo "  3. Acesse o dashboard:"
echo "     http://localhost/aguada/"
echo ""

log_success "Deploy pronto! Sensores podem começar a enviar dados."
