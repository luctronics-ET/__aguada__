#!/bin/bash

# AGUADA - Script de Backup do Banco de Dados

set -e

BACKUP_DIR="./backups"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
DB_NAME="aguada"
BACKUP_FILE="${BACKUP_DIR}/aguada_backup_${TIMESTAMP}.sql"

echo "================================"
echo "  AGUADA - Backup Database"
echo "================================"

# Criar diretório de backups
mkdir -p "$BACKUP_DIR"

# Fazer backup
echo "[INFO] Fazendo backup do banco '$DB_NAME'..."
pg_dump -U aguada_user -d "$DB_NAME" -F p -f "$BACKUP_FILE"

# Comprimir
echo "[INFO] Comprimindo backup..."
gzip "$BACKUP_FILE"

echo "[INFO] ✓ Backup completo: ${BACKUP_FILE}.gz"

# Limpar backups antigos (manter últimos 7 dias)
echo "[INFO] Limpando backups antigos..."
find "$BACKUP_DIR" -name "aguada_backup_*.sql.gz" -mtime +7 -delete

echo "[INFO] ✓ Processo concluído!"
