#!/bin/bash

################################################################################
# AGUADA - Automatic Deployment Script
# Deploy AGUADA system to a new machine with Docker Compose
# Usage: bash deploy-automatic.sh [--secure] [--backup]
################################################################################

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
PROJECT_NAME="aguada"
DOCKER_COMPOSE_FILE="docker-compose.yml"
BACKEND_ENV_FILE="backend/.env"
ADMIN_EMAIL="admin@aguada.local"
DEPLOY_DIR="$PWD"
BACKUP_DIR="$DEPLOY_DIR/backups"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Flags
SECURE_MODE=false
CREATE_BACKUP=false

################################################################################
# Helper Functions
################################################################################

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[✓]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[⚠]${NC} $1"
}

log_error() {
    echo -e "${RED}[✗]${NC} $1"
}

check_command() {
    if ! command -v "$1" &> /dev/null; then
        log_error "$1 não está instalado"
        log_info "Instale com: sudo apt install $1"
        exit 1
    fi
}

################################################################################
# Pre-flight Checks
################################################################################

check_prerequisites() {
    log_info "Verificando pré-requisitos..."
    
    check_command "docker"
    check_command "docker-compose"
    
    # Check Docker daemon
    if ! docker ps &> /dev/null; then
        log_error "Docker daemon não está rodando"
        log_info "Inicie com: sudo systemctl start docker"
        exit 1
    fi
    
    log_success "Docker e Docker Compose OK"
}

check_ports() {
    log_info "Verificando disponibilidade de portas..."
    
    local ports=(80 443 3000 3001 5432 6379)
    local busy_ports=()
    
    for port in "${ports[@]}"; do
        if nc -z localhost "$port" 2>/dev/null; then
            busy_ports+=($port)
        fi
    done
    
    if [ ${#busy_ports[@]} -gt 0 ]; then
        log_warning "Portas ocupadas: ${busy_ports[*]}"
        log_info "Use 'sudo lsof -i :PORT' para identificar processo"
        read -p "Continuar mesmo assim? (s/n): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Ss]$ ]]; then
            log_info "Abortado"
            exit 1
        fi
    fi
    
    log_success "Portas disponíveis"
}

check_disk_space() {
    log_info "Verificando espaço em disco..."
    
    local available=$(df "$DEPLOY_DIR" | awk 'NR==2 {print $4}')
    local required=$((5 * 1024 * 1024))  # 5GB em KB
    
    if [ "$available" -lt "$required" ]; then
        log_error "Espaço insuficiente: ${available}KB disponível, ${required}KB requerido"
        exit 1
    fi
    
    log_success "Espaço em disco adequado"
}

################################################################################
# Configuration
################################################################################

setup_env_file() {
    log_info "Configurando arquivo .env..."
    
    if [ -f "$BACKEND_ENV_FILE" ]; then
        log_warning "Arquivo .env já existe"
        read -p "Sobrescrever? (s/n): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Ss]$ ]]; then
            log_info "Mantendo .env existente"
            return
        fi
    fi
    
    local db_password
    if [ "$SECURE_MODE" = true ]; then
        db_password=$(openssl rand -base64 32)
        log_info "Senha gerada: $db_password (salvar em local seguro!)"
    else
        db_password="aguada_pass_2025"
        log_warning "Usando senha default (MUDE EM PRODUÇÃO!)"
    fi
    
    cat > "$BACKEND_ENV_FILE" << EOF
# AGUADA Backend Configuration
NODE_ENV=production
PORT=3000
LOG_LEVEL=info

# Database
DB_HOST=postgres
DB_PORT=5432
DB_NAME=aguada
DB_USER=aguada_user
DB_PASSWORD=$db_password

# Redis
REDIS_HOST=redis
REDIS_PORT=6379

# API Configuration
API_BASE_URL=http://localhost
API_TIMEOUT=30000

# Serial/Gateway (USB)
SERIAL_PORT=/dev/ttyACM0
SERIAL_BAUDRATE=115200

# Features
ENABLE_MQTT=false
ENABLE_WEBSOCKET=true
ENABLE_COMPRESSION=true

# Logging
LOG_FILE=logs/aguada.log
EOF
    
    log_success "Arquivo .env criado"
}

################################################################################
# Docker Operations
################################################################################

start_services() {
    log_info "Iniciando serviços Docker..."
    
    cd "$DEPLOY_DIR"
    
    # Stop existing if any
    docker-compose down 2>/dev/null || true
    sleep 2
    
    # Start services
    docker-compose up -d
    
    # Wait for services to be ready
    log_info "Aguardando serviços ficarem prontos..."
    sleep 10
    
    # Check container status
    local containers=$(docker-compose ps --services --filter "status=running")
    local expected=5
    local running=$(echo "$containers" | wc -l)
    
    if [ "$running" -lt "$expected" ]; then
        log_error "Apenas $running/$expected containers rodando"
        docker-compose ps
        exit 1
    fi
    
    log_success "Todos os containers iniciados"
}

wait_for_database() {
    log_info "Aguardando database ficar pronto..."
    
    local max_attempts=30
    local attempt=1
    
    while [ $attempt -le $max_attempts ]; do
        if docker-compose exec -T postgres psql -U aguada -d aguada -c "SELECT 1;" &>/dev/null; then
            log_success "Database pronto"
            return 0
        fi
        
        echo -ne "\rTentativa $attempt/$max_attempts..."
        sleep 2
        ((attempt++))
    done
    
    log_error "Database timeout"
    docker-compose logs postgres
    exit 1
}

initialize_database() {
    log_info "Inicializando database..."
    
    # Check if tables exist
    local table_count=$(docker-compose exec -T postgres psql -U aguada -d aguada -c \
        "SELECT count(*) FROM information_schema.tables WHERE table_schema = 'aguada';" \
        2>/dev/null | tail -1 | xargs)
    
    if [ "$table_count" -gt 10 ]; then
        log_info "Database já inicializado ($table_count tabelas encontradas)"
        return
    fi
    
    log_info "Aplicando schema..."
    cat "$DEPLOY_DIR/database/schema.sql" | \
        docker-compose exec -T postgres psql -U aguada -d aguada
    
    log_success "Schema aplicado"
    
    if [ -f "$DEPLOY_DIR/database/sample-data.sql" ]; then
        log_info "Carregando dados de exemplo..."
        cat "$DEPLOY_DIR/database/sample-data.sql" | \
            docker-compose exec -T postgres psql -U aguada -d aguada
        log_success "Dados de exemplo carregados"
    fi
}

################################################################################
# Validation
################################################################################

run_health_checks() {
    log_info "Executando health checks..."
    
    local endpoints=(
        "http://localhost:3000/api/health"
        "http://localhost:3000/api/sensors"
        "http://localhost:3000/api/readings/latest"
    )
    
    local passed=0
    
    for endpoint in "${endpoints[@]}"; do
        local response=$(curl -s -w "\n%{http_code}" "$endpoint" 2>/dev/null || echo "000")
        local http_code=$(echo "$response" | tail -1)
        
        if [ "$http_code" = "200" ]; then
            log_success "✓ $endpoint"
            ((passed++))
        else
            log_warning "✗ $endpoint (HTTP $http_code)"
        fi
    done
    
    log_info "Health checks: $passed/${#endpoints[@]} passed"
}

run_full_test_suite() {
    log_info "Executando suite de testes completa..."
    
    if [ -f "./test-sistema.sh" ]; then
        chmod +x ./test-sistema.sh
        ./test-sistema.sh
    else
        log_warning "Script de testes não encontrado"
    fi
}

################################################################################
# Backup
################################################################################

create_database_backup() {
    log_info "Criando backup do database..."
    
    mkdir -p "$BACKUP_DIR"
    
    local backup_file="$BACKUP_DIR/aguada_backup_$TIMESTAMP.sql"
    
    docker-compose exec -T postgres pg_dump -U aguada aguada > "$backup_file"
    
    # Compress
    gzip "$backup_file"
    backup_file="${backup_file}.gz"
    
    local size=$(du -h "$backup_file" | cut -f1)
    log_success "Backup criado: $backup_file ($size)"
}

################################################################################
# SSL/HTTPS Setup (Optional)
################################################################################

setup_ssl() {
    log_info "Configurando SSL/HTTPS..."
    
    local cert_dir="$DEPLOY_DIR/docker/certs"
    mkdir -p "$cert_dir"
    
    if [ -f "$cert_dir/aguada.crt" ]; then
        log_warning "Certificado SSL já existe"
        return
    fi
    
    log_info "Gerando certificado auto-assinado (válido por 365 dias)..."
    
    openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
        -keyout "$cert_dir/aguada.key" \
        -out "$cert_dir/aguada.crt" \
        -subj "/C=BR/ST=SP/L=Campinas/O=AGUADA/CN=localhost"
    
    log_success "Certificado SSL criado"
    log_info "Nota: Use certificado válido em produção (Let's Encrypt, etc)"
}

################################################################################
# Firewall Setup (Optional)
################################################################################

setup_firewall() {
    log_info "Configurando firewall..."
    
    if ! command -v ufw &> /dev/null; then
        log_warning "UFW não instalado, pulando"
        return
    fi
    
    if sudo ufw status | grep -q "Status: inactive"; then
        log_warning "UFW inativo"
        return
    fi
    
    log_info "Abrindo portas..."
    sudo ufw allow 80/tcp
    sudo ufw allow 443/tcp
    sudo ufw allow 22/tcp
    
    log_success "Firewall configurado"
}

################################################################################
# Post-Deployment
################################################################################

show_credentials() {
    log_info "Credenciais de acesso:"
    echo ""
    echo "  Dashboard:  http://localhost"
    echo "  API:        http://localhost:3000/api"
    echo "  Grafana:    http://localhost:3001"
    echo ""
    echo "  Grafana User:     admin"
    echo "  Grafana Password: admin (MUDE NA PRIMEIRA EXECUÇÃO)"
    echo ""
    
    if grep -q "DB_PASSWORD" "$BACKEND_ENV_FILE"; then
        echo "  Database:   postgres / aguada_user"
        echo "  Banco:      aguada"
        echo ""
    fi
}

show_next_steps() {
    echo ""
    echo -e "${GREEN}=== DEPLOYMENT COMPLETO ===${NC}"
    echo ""
    echo "Próximas ações recomendadas:"
    echo "  1. Alterar senha Grafana: http://localhost:3001"
    echo "  2. Conectar ESP32 sensor nodes via USB"
    echo "  3. Verificar leituras: http://localhost/aguada"
    echo "  4. Ativar MQTT se necessário (docker-compose.yml)"
    echo "  5. Configurar SSL em produção"
    echo ""
    echo "Documentação completa em: DEPLOYMENT.md"
    echo ""
}

################################################################################
# Main
################################################################################

main() {
    echo -e "${BLUE}"
    echo "╔══════════════════════════════════════════╗"
    echo "║  🌊 AGUADA - Automatic Deployment v2.1.0  ║"
    echo "║   Hydraulic Monitoring System            ║"
    echo "╚══════════════════════════════════════════╝"
    echo -e "${NC}"
    echo ""
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --secure) SECURE_MODE=true; shift ;;
            --backup) CREATE_BACKUP=true; shift ;;
            *) log_error "Argumento desconhecido: $1"; exit 1 ;;
        esac
    done
    
    # Execute deployment steps
    check_prerequisites
    check_ports
    check_disk_space
    setup_env_file
    
    if [ "$SECURE_MODE" = true ]; then
        setup_ssl
        setup_firewall
    fi
    
    start_services
    wait_for_database
    initialize_database
    
    sleep 5
    run_health_checks
    run_full_test_suite
    
    if [ "$CREATE_BACKUP" = true ]; then
        create_database_backup
    fi
    
    show_credentials
    show_next_steps
    
    log_success "Sistema AGUADA pronto para uso!"
}

# Run
main "$@"
