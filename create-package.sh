#!/bin/bash

################################################################################
# AGUADA - Package Creation Script
# Create a deployable package for distribution
# Usage: bash create-package.sh
################################################################################

set -e

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

# Configuration
PROJECT_NAME="aguada"
PROJECT_DIR="$PWD"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
PACKAGE_DIR="/tmp/aguada-package-$TIMESTAMP"
PACKAGE_FILE="$PROJECT_DIR/aguada-v2.1.0-$TIMESTAMP.tar.gz"

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[✓]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[⚠]${NC} $1"; }
log_error() { echo -e "${RED}[✗]${NC} $1"; }

################################################################################
# Package Creation
################################################################################

create_package() {
    echo -e "${BLUE}"
    echo "╔════════════════════════════════════════╗"
    echo "║ AGUADA Package Creator v2.1.0         ║"
    echo "║ Hydraulic Monitoring System           ║"
    echo "╚════════════════════════════════════════╝"
    echo -e "${NC}"
    echo ""
    
    log_info "Criando pacote..."
    mkdir -p "$PACKAGE_DIR"
    
    # Copy source excluding unnecessary files
    log_info "Copiando arquivos..."
    
    rsync -av \
        --exclude='.git' \
        --exclude='.github' \
        --exclude='node_modules' \
        --exclude='build' \
        --exclude='.env' \
        --exclude='.env.local' \
        --exclude='*.log' \
        --exclude='logs/*' \
        --exclude='.DS_Store' \
        --exclude='*.tmp' \
        --exclude='backups/*' \
        --exclude='.playwright-mcp' \
        "$PROJECT_DIR/" "$PACKAGE_DIR/$PROJECT_NAME/"
    
    log_success "Arquivos copiados"
    
    # Create README for package
    log_info "Criando documentação de deploy..."
    cat > "$PACKAGE_DIR/$PROJECT_NAME/QUICKSTART_DEPLOY.md" << 'EOF'
# 🚀 AGUADA v2.1.0 - Quick Deploy Guide

## Sistema Pronto para Produção

Este pacote contém o sistema AGUADA completo e testado. Siga os passos abaixo para deploy.

### 📋 Pré-requisitos

- Docker 20.10+
- Docker Compose 2.0+
- Git (opcional, para versionamento)
- 5GB espaço livre em disco

### 🚀 Deploy Rápido (3 passos)

```bash
# 1. Extrair e navegar
tar xzf aguada-v2.1.0-*.tar.gz
cd aguada

# 2. Executar deploy automático
bash deploy-automatic.sh

# 3. Acessar sistema
# Dashboard:  http://localhost
# API:        http://localhost:3000/api
# Grafana:    http://localhost:3001
```

### 🔒 Deploy Seguro (com SSL)

```bash
bash deploy-automatic.sh --secure --backup
```

### ✅ Validar Deploy

```bash
# Executar suite de testes
./test-sistema.sh

# Todos os 8 testes devem passar ✓
```

### 📖 Documentação Completa

- `DEPLOYMENT.md` - Guia detalhado de deploy
- `docs/RULES.md` - Especificação técnica do sistema
- `backend/README.md` - API documentation
- `firmware/*/README.md` - Firmware ESP32 notes

### 🔧 Troubleshooting

**Erro: "Docker daemon não está rodando"**
```bash
sudo systemctl start docker
sudo systemctl enable docker
```

**Erro: "Port already in use"**
```bash
# Liberar porta
sudo lsof -i :3000
sudo kill -9 <PID>
```

**Erro: "Database connection failed"**
```bash
# Aguardar mais tempo
docker-compose logs postgres
docker-compose ps
```

### 📊 Dados Iniciais

O sistema inicia com dados de exemplo. Para usar dados reais:

```bash
# Restaurar backup
docker-compose exec postgres psql -U aguada -d aguada < seu_backup.sql

# Ou conectar sensores ESP32 via USB
# Verificar em: http://localhost/aguada
```

### 🔐 Segurança

**IMPORTANTE:** Altere credenciais padrão em PRODUÇÃO

```bash
# Grafana (padrão: admin/admin)
http://localhost:3001 → Admin → Change Password

# Database (padrão: aguada_user)
docker-compose exec postgres psql -U aguada -c \
  "ALTER USER aguada_user PASSWORD 'nova_senha';"
```

### 📞 Suporte

Para problemas, consulte:
- `DEPLOYMENT.md` - Guia completo
- `docs/SETUP.md` - Guia de configuração avançada
- Logs: `docker-compose logs -f backend`

---

**Sistema AGUADA v2.1.0**  
Validado e pronto para produção ✅
EOF
    
    log_success "Documentação de deploy criada"
    
    # Create installation checklist
    log_info "Criando checklist..."
    cat > "$PACKAGE_DIR/$PROJECT_NAME/INSTALLATION_CHECKLIST.md" << 'EOF'
# ✅ Installation Checklist - AGUADA v2.1.0

## Pré-Instalação
- [ ] Docker 20.10+ instalado: `docker --version`
- [ ] Docker Compose 2.0+ instalado: `docker-compose --version`
- [ ] Mínimo 5GB disco disponível: `df -h`
- [ ] Portas 80, 443, 3000, 3001, 5432, 6379 livres
- [ ] Conexão internet funcional

## Instalação
- [ ] Pacote extraído com sucesso
- [ ] Diretório `aguada/` criado
- [ ] Permissões corretas: `chmod +x deploy-automatic.sh`
- [ ] Script de deploy executado: `bash deploy-automatic.sh`
- [ ] Todos 5 containers iniciados: `docker-compose ps`

## Validação
- [ ] Health check passou: `curl http://localhost:3000/api/health`
- [ ] 8/8 testes passaram: `./test-sistema.sh`
- [ ] Dashboard carrega: http://localhost
- [ ] API responde: http://localhost:3000/api/sensors
- [ ] Grafana acessa: http://localhost:3001

## Pós-Instalação
- [ ] Senha Grafana alterada (default: admin/admin)
- [ ] Senha Database alterada (se necessário)
- [ ] Backup database realizado
- [ ] SSL/HTTPS configurado (produção)
- [ ] Firewall configurado

## Integração Sensores
- [ ] ESP32 sensor nodes identificados
- [ ] Gateway USB conectado (se aplicável)
- [ ] Firmware flasheado nos devices
- [ ] Primeira leitura recebida em `/api/readings/latest`
- [ ] Dados visíveis no dashboard

## Operação
- [ ] Sistema rodando por 1+ hora sem erros
- [ ] Alertas configurados (se aplicável)
- [ ] Monitoramento ativo (Grafana dashboards)
- [ ] Backups automáticos configurados (opcional)
- [ ] Logs sendo mantidos

## Segurança (Produção)
- [ ] Senhas default alteradas
- [ ] Credenciais removidas de `.env` versionado
- [ ] SSL/HTTPS ativo
- [ ] Firewall restringindo acesso
- [ ] VPN/Proxy reverso em frente (recomendado)

---

Data de Conclusão: _______  
Responsável: _______  
Observações: _______
EOF
    
    log_success "Checklist criado"
    
    # Create environment template
    log_info "Criando template de ambiente..."
    cat > "$PACKAGE_DIR/$PROJECT_NAME/backend/.env.example.production" << 'EOF'
# ========================================
# AGUADA Production Configuration
# ========================================

# Application
NODE_ENV=production
PORT=3000
LOG_LEVEL=warn

# Database (PostgreSQL 16 + TimescaleDB)
DB_HOST=postgres
DB_PORT=5432
DB_NAME=aguada
DB_USER=aguada_user
DB_PASSWORD=CHANGE_ME_STRONG_PASSWORD

# Cache & Queue
REDIS_HOST=redis
REDIS_PORT=6379
REDIS_PASSWORD=CHANGE_ME_REDIS_PASSWORD

# API
API_BASE_URL=https://seu-dominio.com
API_TIMEOUT=30000
CORS_ORIGIN=https://seu-dominio.com

# Serial/Gateway
SERIAL_PORT=/dev/ttyACM0
SERIAL_BAUDRATE=115200
SERIAL_TIMEOUT=5000

# MQTT (se ativado)
MQTT_BROKER=localhost
MQTT_PORT=1883
MQTT_USER=aguada
MQTT_PASSWORD=CHANGE_ME

# Features
ENABLE_MQTT=false
ENABLE_WEBSOCKET=true
ENABLE_COMPRESSION=true
COMPRESSION_THRESHOLD=1024

# Security
JWT_SECRET=CHANGE_ME_JWT_SECRET
SESSION_SECRET=CHANGE_ME_SESSION_SECRET
RATE_LIMIT=100

# Logging
LOG_FILE=/var/log/aguada/aguada.log
LOG_MAX_SIZE=100M
LOG_RETENTION_DAYS=30

# Notifications (opcional)
EMAIL_SERVICE=smtp
EMAIL_HOST=smtp.gmail.com
EMAIL_PORT=587
EMAIL_USER=seu-email@gmail.com
EMAIL_PASSWORD=CHANGE_ME

# Alerting
ALERT_EMAIL_TO=admin@sua-empresa.com
ALERT_SLACK_WEBHOOK=https://hooks.slack.com/...
ALERT_TELEGRAM_BOT_TOKEN=
ALERT_TELEGRAM_CHAT_ID=
EOF
    
    log_success "Template de ambiente criado"
    
    # Create version file
    echo "2.1.0" > "$PACKAGE_DIR/$PROJECT_NAME/VERSION"
    echo $(date -u +"%Y-%m-%dT%H:%M:%SZ") > "$PACKAGE_DIR/$PROJECT_NAME/BUILD_DATE"
    
    log_success "Informações de versão adicionadas"
    
    # Create package
    log_info "Compactando pacote..."
    cd /tmp
    tar czf "$PACKAGE_FILE" "aguada-package-$TIMESTAMP"
    
    local size=$(du -h "$PACKAGE_FILE" | cut -f1)
    log_success "Pacote criado: $PACKAGE_FILE ($size)"
    
    # Cleanup
    rm -rf "$PACKAGE_DIR"
    
    # Create checksum
    log_info "Gerando checksum..."
    cd "$PROJECT_DIR"
    sha256sum "$PACKAGE_FILE" > "${PACKAGE_FILE}.sha256"
    
    log_success "Checksum criado"
    
    # Summary
    echo ""
    echo -e "${GREEN}════════════════════════════════════════${NC}"
    echo -e "${GREEN}✓ PACKAGE PRONTO PARA DISTRIBUIÇÃO${NC}"
    echo -e "${GREEN}════════════════════════════════════════${NC}"
    echo ""
    echo "📦 Arquivo: $(basename $PACKAGE_FILE)"
    echo "📍 Localização: $PACKAGE_FILE"
    echo "📊 Tamanho: $size"
    echo ""
    echo "🔐 Checksum SHA-256:"
    cat "${PACKAGE_FILE}.sha256"
    echo ""
    echo "📋 Instruções para o outro computador:"
    echo ""
    echo "  1. Copiar arquivo:"
    echo "     $PACKAGE_FILE"
    echo ""
    echo "  2. Extrair:"
    echo "     tar xzf aguada-v2.1.0-*.tar.gz"
    echo "     cd aguada"
    echo ""
    echo "  3. Executar deploy:"
    echo "     bash deploy-automatic.sh"
    echo ""
    echo "  4. Acessar:"
    echo "     Dashboard: http://localhost"
    echo "     API: http://localhost:3000/api"
    echo ""
}

main() {
    create_package
}

main "$@"
