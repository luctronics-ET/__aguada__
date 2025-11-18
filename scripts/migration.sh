#!/bin/bash

# =============================================================================
# AGUADA - MIGRATION & BACKUP SCRIPT
# Migração de dados entre instâncias
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
# FUNÇÃO: FAZER BACKUP
# =============================================================================

backup_database() {
    local BACKUP_DIR="${1:-.}"
    local BACKUP_FILE="$BACKUP_DIR/aguada_backup_$(date +%Y%m%d_%H%M%S).sql"
    
    log_info "Criando backup do banco de dados..."
    
    if command -v docker &> /dev/null && docker ps | grep -q aguada-postgres; then
        # Docker
        docker exec aguada-postgres pg_dump -U aguada_user -d aguada_db > "$BACKUP_FILE"
    else
        # Local PostgreSQL
        pg_dump -U aguada_user -d aguada_db > "$BACKUP_FILE"
    fi
    
    # Comprimir
    gzip "$BACKUP_FILE"
    BACKUP_FILE="${BACKUP_FILE}.gz"
    
    log_success "Backup criado: $BACKUP_FILE"
    echo "$BACKUP_FILE"
}

# =============================================================================
# FUNÇÃO: RESTAURAR BACKUP
# =============================================================================

restore_database() {
    local BACKUP_FILE="$1"
    
    if [ ! -f "$BACKUP_FILE" ]; then
        log_error "Arquivo de backup não encontrado: $BACKUP_FILE"
        return 1
    fi
    
    log_warning "Esta ação vai sobrescrever o banco de dados atual!"
    read -p "Tem certeza? (s/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Ss]$ ]]; then
        log_info "Operação cancelada"
        return 0
    fi
    
    log_info "Restaurando backup..."
    
    # Descomprimir se necessário
    if [[ "$BACKUP_FILE" == *.gz ]]; then
        TEMP_FILE=$(mktemp)
        gunzip -c "$BACKUP_FILE" > "$TEMP_FILE"
        BACKUP_FILE="$TEMP_FILE"
    fi
    
    if command -v docker &> /dev/null && docker ps | grep -q aguada-postgres; then
        # Docker
        docker exec -i aguada-postgres psql -U aguada_user -d aguada_db < "$BACKUP_FILE"
    else
        # Local PostgreSQL
        psql -U aguada_user -d aguada_db < "$BACKUP_FILE"
    fi
    
    log_success "Backup restaurado com sucesso!"
}

# =============================================================================
# FUNÇÃO: EXPORTAR DADOS
# =============================================================================

export_data() {
    local EXPORT_DIR="${1:-.}"
    local FORMAT="${2:-json}"  # json, csv
    
    log_info "Exportando dados em formato $FORMAT..."
    
    mkdir -p "$EXPORT_DIR"
    
    if [ "$FORMAT" = "json" ]; then
        # Exportar leituras bruas como JSON
        if command -v docker &> /dev/null && docker ps | grep -q aguada-postgres; then
            docker exec aguada-postgres psql -U aguada_user -d aguada_db \
                -c "SELECT row_to_json(t) FROM leituras_raw t LIMIT 1000;" \
                > "$EXPORT_DIR/leituras_raw.json"
        else
            psql -U aguada_user -d aguada_db \
                -c "SELECT row_to_json(t) FROM leituras_raw t LIMIT 1000;" \
                > "$EXPORT_DIR/leituras_raw.json"
        fi
        
        log_success "Dados exportados para $EXPORT_DIR/leituras_raw.json"
    fi
}

# =============================================================================
# FUNÇÃO: SINCRONIZAR ENTRE INSTÂNCIAS
# =============================================================================

sync_between_instances() {
    local SOURCE_HOST="$1"
    local SOURCE_USER="$2"
    local DEST_HOST="$3"
    local DEST_USER="$4"
    
    log_warning "Sincronização entre instâncias requer SSH acesso"
    
    log_info "Fazendo backup na origem: $SOURCE_HOST"
    BACKUP_FILE=$(ssh "$SOURCE_USER@$SOURCE_HOST" "cd /path/to/aguada && ./scripts/migration.sh backup")
    
    log_info "Transferindo arquivo: $BACKUP_FILE"
    scp "$SOURCE_USER@$SOURCE_HOST:$BACKUP_FILE" "/tmp/"
    
    log_info "Restaurando no destino: $DEST_HOST"
    ssh "$DEST_USER@$DEST_HOST" "cd /path/to/aguada && ./scripts/migration.sh restore /tmp/$(basename $BACKUP_FILE)"
    
    log_success "Sincronização completa!"
}

# =============================================================================
# FUNÇÃO: TRANSFERIR ARQUIVO PARA PENDRIVE
# =============================================================================

export_to_usb() {
    local USB_MOUNT="${1:/media/usb}"
    
    log_info "Preparando exportação para Pendrive..."
    log_info "Ensure pendrive está montado em: $USB_MOUNT"
    
    if [ ! -d "$USB_MOUNT" ]; then
        log_error "Diretório pendrive não encontrado: $USB_MOUNT"
        log_info "Montagens disponíveis:"
        mount | grep -E "media|mnt"
        return 1
    fi
    
    local EXPORT_DIR="$USB_MOUNT/aguada_export_$(date +%Y%m%d_%H%M%S)"
    mkdir -p "$EXPORT_DIR"
    
    # Backup do banco
    log_info "Fazendo backup..."
    backup_database "$EXPORT_DIR"
    
    # Copiar frontend
    log_info "Copiando frontend..."
    cp -r frontend "$EXPORT_DIR/"
    
    # Copiar backend (sem node_modules)
    log_info "Copiando backend..."
    rsync -av --exclude node_modules --exclude logs backend/ "$EXPORT_DIR/backend/"
    
    # Copiar scripts
    log_info "Copiando scripts..."
    cp scripts/deploy.sh "$EXPORT_DIR/"
    cp scripts/install-manual.sh "$EXPORT_DIR/"
    cp docker-compose.yml "$EXPORT_DIR/"
    cp backend/Dockerfile "$EXPORT_DIR/"
    
    # Criar README
    cat > "$EXPORT_DIR/README_RESTORE.md" << 'EOF'
# AGUADA - Restauração de Pendrive

## Passos para restaurar em novo computador:

1. **Copiar arquivos:**
   ```bash
   cp -r /media/usb/aguada_export_* ~/aguada
   cd ~/aguada
   ```

2. **Instalar (opção Docker - RECOMENDADO):**
   ```bash
   chmod +x scripts/deploy.sh
   ./scripts/deploy.sh
   ```

3. **OU Instalar (opção manual - Linux apenas):**
   ```bash
   chmod +x scripts/install-manual.sh
   sudo ./scripts/install-manual.sh
   ```

4. **Restaurar banco de dados:**
   ```bash
   chmod +x scripts/migration.sh
   ./scripts/migration.sh restore database/aguada_backup_*.sql.gz
   ```

5. **Configurar sensores:**
   - Editar firmware ESP32 com IP do novo computador
   - Recompilar e flashear sensores

6. **Acessar:**
   - Frontend: http://localhost
   - API: http://localhost:3000/api
EOF
    
    log_success "Exportação completa em: $EXPORT_DIR"
    log_info "Total de arquivos: $(find $EXPORT_DIR -type f | wc -l)"
    log_info "Tamanho total: $(du -sh $EXPORT_DIR)"
}

# =============================================================================
# MENU PRINCIPAL
# =============================================================================

show_menu() {
    echo ""
    echo "=========================================="
    echo "AGUADA - Migração e Backup"
    echo "=========================================="
    echo "1) Fazer backup"
    echo "2) Restaurar backup"
    echo "3) Exportar para Pendrive"
    echo "4) Exportar dados (JSON)"
    echo "5) Sair"
    echo ""
    read -p "Escolha uma opção: " choice
    
    case $choice in
        1)
            BACKUP_FILE=$(backup_database)
            log_success "Backup concluído!"
            ;;
        2)
            read -p "Caminho do arquivo de backup: " backup_path
            restore_database "$backup_path"
            ;;
        3)
            read -p "Ponto de montagem do Pendrive (padrão: /media/usb): " usb_mount
            export_to_usb "${usb_mount:=/media/usb}"
            ;;
        4)
            read -p "Diretório de exportação (padrão: ./export): " export_dir
            export_data "${export_dir:=./export}"
            ;;
        5)
            log_info "Saindo..."
            exit 0
            ;;
        *)
            log_error "Opção inválida"
            show_menu
            ;;
    esac
}

# =============================================================================
# SCRIPT PRINCIPAL
# =============================================================================

if [ "$1" = "backup" ]; then
    backup_database "${2:-.}"
elif [ "$1" = "restore" ]; then
    restore_database "$2"
elif [ "$1" = "export-usb" ]; then
    export_to_usb "${2:/media/usb}"
elif [ "$1" = "export-data" ]; then
    export_data "${2:-.}" "${3:-json}"
else
    # Menu interativo
    show_menu
fi
