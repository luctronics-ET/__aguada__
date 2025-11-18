#!/bin/bash

# =============================================================================
# AGUADA - MANUAL INSTALLATION SCRIPT (sem Docker)
# Para Linux Debian/Ubuntu com acesso sudo
# =============================================================================

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info() { echo -e "${BLUE}ℹ️  $1${NC}"; }
log_success() { echo -e "${GREEN}✅ $1${NC}"; }
log_warning() { echo -e "${YELLOW}⚠️  $1${NC}"; }
log_error() { echo -e "${RED}❌ $1${NC}"; }

# =============================================================================
# 1. VERIFICAÇÕES INICIAIS
# =============================================================================

log_info "Iniciando instalação manual do AGUADA..."

if [ "$EUID" -ne 0 ]; then
   log_error "Este script deve ser executado com sudo"
   exit 1
fi

# =============================================================================
# 2. ATUALIZAR SISTEMA
# =============================================================================

log_info "Atualizando repositórios..."
apt-get update -qq
log_success "Repositórios atualizados ✓"

# =============================================================================
# 3. INSTALAR DEPENDÊNCIAS SISTEMA
# =============================================================================

log_info "Instalando dependências do sistema..."

PACKAGES=(
    "build-essential"
    "curl"
    "wget"
    "git"
    "nodejs"
    "npm"
    "postgresql"
    "postgresql-contrib"
    "redis-server"
    "nginx"
    "supervisor"
)

for package in "${PACKAGES[@]}"; do
    if ! dpkg -l | grep -q "^ii.*$package"; then
        log_info "Instalando $package..."
        apt-get install -y "$package" > /dev/null 2>&1
        log_success "$package ✓"
    else
        log_success "$package já instalado ✓"
    fi
done

# =============================================================================
# 4. INSTALAR TIMESCALEDB
# =============================================================================

log_info "Instalando TimescaleDB para PostgreSQL..."

if ! dpkg -l | grep -q "postgresql-15-timescaledb"; then
    # Adicionar repositório TimescaleDB
    apt install -y postgresql-15-timescaledb-2-loader-deb
    
    # Editar postgresql.conf
    PG_CONFIG="/etc/postgresql/15/main/postgresql.conf"
    if ! grep -q "timescaledb" "$PG_CONFIG"; then
        echo "shared_preload_libraries = 'timescaledb'" >> "$PG_CONFIG"
    fi
    
    # Reiniciar PostgreSQL
    systemctl restart postgresql
    log_success "TimescaleDB instalado ✓"
else
    log_success "TimescaleDB já instalado ✓"
fi

# =============================================================================
# 5. CRIAR BANCO DE DADOS
# =============================================================================

log_info "Configurando PostgreSQL..."

# Iniciar PostgreSQL
systemctl start postgresql
systemctl enable postgresql

# Criar usuario e banco
sudo -u postgres psql << EOF
-- Criar usuário
CREATE USER aguada_user WITH PASSWORD 'aguada_pass_2025' CREATEDB;

-- Criar banco de dados
CREATE DATABASE aguada_db OWNER aguada_user;

-- Conectar ao banco e criar extensões
\c aguada_db;
CREATE EXTENSION IF NOT EXISTS timescaledb CASCADE;
CREATE EXTENSION IF NOT EXISTS postgis;

-- Criar schema
CREATE SCHEMA aguada;
GRANT ALL ON SCHEMA aguada TO aguada_user;
EOF

log_success "PostgreSQL configurado ✓"

# =============================================================================
# 6. IMPORTAR SCHEMA
# =============================================================================

log_info "Importando schema do banco de dados..."

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
AGUADA_DIR="$(dirname "$SCRIPT_DIR")"

if [ -f "$AGUADA_DIR/database/init.sql" ]; then
    sudo -u postgres psql -d aguada_db -f "$AGUADA_DIR/database/init.sql" > /dev/null 2>&1
    log_success "Schema importado ✓"
else
    log_error "Arquivo init.sql não encontrado em $AGUADA_DIR/database/"
    exit 1
fi

# =============================================================================
# 7. CONFIGURAR REDIS
# =============================================================================

log_info "Configurando Redis..."

systemctl start redis-server
systemctl enable redis-server

log_success "Redis configurado ✓"

# =============================================================================
# 8. INSTALAR DEPENDÊNCIAS NODE.JS
# =============================================================================

log_info "Instalando dependências Node.js..."

cd "$AGUADA_DIR/backend"
npm ci --only=production > /dev/null 2>&1

log_success "Dependências Node.js instaladas ✓"

# =============================================================================
# 9. CONFIGURAR BACKEND COM SUPERVISOR
# =============================================================================

log_info "Configurando supervisor para gerenciar Backend..."

cat > /etc/supervisor/conf.d/aguada-backend.conf << EOF
[program:aguada-backend]
command=node $AGUADA_DIR/backend/src/server.js
directory=$AGUADA_DIR/backend
user=nobody
autostart=true
autorestart=true
stderr_logfile=$AGUADA_DIR/backend/logs/backend.err.log
stdout_logfile=$AGUADA_DIR/backend/logs/backend.out.log

environment=
    NODE_ENV=production,
    PORT=3000,
    DB_HOST=localhost,
    DB_PORT=5432,
    DB_USER=aguada_user,
    DB_PASSWORD=aguada_pass_2025,
    DB_NAME=aguada_db,
    REDIS_HOST=localhost,
    REDIS_PORT=6379
EOF

systemctl start supervisor
systemctl enable supervisor
supervisorctl reread
supervisorctl update

log_success "Supervisor configurado ✓"

# =============================================================================
# 10. CONFIGURAR NGINX
# =============================================================================

log_info "Configurando Nginx..."

cat > /etc/nginx/sites-available/aguada << 'EOF'
server {
    listen 80 default_server;
    server_name _;
    
    client_max_body_size 20M;
    
    # Frontend estático
    location / {
        root __AGUADA_DIR__/frontend;
        try_files $uri $uri/ /index.html;
        expires 1h;
    }
    
    # API backend
    location /api/ {
        proxy_pass http://127.0.0.1:3000;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
        proxy_connect_timeout 60s;
        proxy_send_timeout 60s;
        proxy_read_timeout 60s;
    }
}
EOF

# Substituir placeholder
sed -i "s|__AGUADA_DIR__|$AGUADA_DIR|g" /etc/nginx/sites-available/aguada

# Ativar site
ln -sf /etc/nginx/sites-available/aguada /etc/nginx/sites-enabled/
rm -f /etc/nginx/sites-enabled/default

# Testar config
nginx -t > /dev/null 2>&1

# Iniciar Nginx
systemctl start nginx
systemctl enable nginx

log_success "Nginx configurado ✓"

# =============================================================================
# 11. CRIAR DIRETÓRIOS NECESSÁRIOS
# =============================================================================

log_info "Criando diretórios..."
mkdir -p "$AGUADA_DIR/backend/logs"
mkdir -p "$AGUADA_DIR/database/backups"
chmod -R 755 "$AGUADA_DIR"
log_success "Diretórios criados ✓"

# =============================================================================
# 12. CRIAR SCRIPT DE BACKUP
# =============================================================================

log_info "Criando script de backup automático..."

cat > /usr/local/bin/aguada-backup << 'EOF'
#!/bin/bash
BACKUP_DIR="__AGUADA_DIR__/database/backups"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
BACKUP_FILE="$BACKUP_DIR/aguada_$TIMESTAMP.sql.gz"

mkdir -p "$BACKUP_DIR"

sudo -u postgres pg_dump -d aguada_db | gzip > "$BACKUP_FILE"

# Manter apenas últimos 10 backups
ls -t "$BACKUP_DIR"/aguada_*.sql.gz | tail -n +11 | xargs rm -f

echo "✅ Backup criado: $BACKUP_FILE"
EOF

sed -i "s|__AGUADA_DIR__|$AGUADA_DIR|g" /usr/local/bin/aguada-backup
chmod +x /usr/local/bin/aguada-backup

log_success "Script de backup criado ✓"

# =============================================================================
# 13. AGENDAR BACKUPS DIÁRIOS
# =============================================================================

log_info "Agendando backups diários..."

# Adicionar ao crontab root
(crontab -l 2>/dev/null | grep -v aguada-backup; echo "0 2 * * * /usr/local/bin/aguada-backup") | crontab -

log_success "Backups agendados ✓"

# =============================================================================
# 14. OBTER IP E INFORMAÇÕES
# =============================================================================

IP_ADDRESS=$(hostname -I | awk '{print $1}')

# =============================================================================
# 15. VERIFICAÇÕES FINAIS
# =============================================================================

echo ""
log_success "=========================================="
log_success "AGUADA Instalado com Sucesso! 🎉"
log_success "=========================================="
echo ""

log_success "Serviços em execução:"
systemctl is-active --quiet postgresql && echo "  ✅ PostgreSQL" || echo "  ❌ PostgreSQL"
systemctl is-active --quiet redis-server && echo "  ✅ Redis" || echo "  ❌ Redis"
systemctl is-active --quiet nginx && echo "  ✅ Nginx" || echo "  ❌ Nginx"

echo ""
log_info "Acessar o sistema:"
echo "  📱 Frontend:  http://$IP_ADDRESS"
echo "  🔌 Backend:   http://$IP_ADDRESS:3000/api"
echo "  🐘 PostgreSQL: localhost:5432"
echo "  💾 Redis:     localhost:6379"
echo ""

log_info "Diretórios importantes:"
echo "  App:          $AGUADA_DIR"
echo "  Backend:      $AGUADA_DIR/backend"
echo "  Frontend:     $AGUADA_DIR/frontend"
echo "  Logs:         $AGUADA_DIR/backend/logs"
echo "  Backups:      $AGUADA_DIR/database/backups"
echo ""

log_info "Comandos úteis:"
echo "  Ver logs:           tail -f $AGUADA_DIR/backend/logs/backend.out.log"
echo "  Reiniciar backend:  supervisorctl restart aguada-backend"
echo "  Status:             supervisorctl status"
echo "  Acessar DB:         psql -U aguada_user -d aguada_db"
echo "  Fazer backup:       /usr/local/bin/aguada-backup"
echo ""

log_warning "PRÓXIMOS PASSOS:"
echo "  1. Configure os sensores para enviar para: http://$IP_ADDRESS:3000/api/telemetry"
echo "  2. Verifique dados:"
echo "     curl http://localhost:3000/api/readings/latest"
echo "  3. Acesse o dashboard:"
echo "     http://$IP_ADDRESS/"
echo ""

log_success "Instalação concluída! Sistema pronto para receber dados dos sensores."
