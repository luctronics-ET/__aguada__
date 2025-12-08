#!/bin/bash

# Script para iniciar o frontend React e abrir no Simple Browser do VS Code
# AGUADA Project - Frontend React Starter

set -e

PROJECT_DIR="/home/luciano/Área de trabalho/aguada"
FRONTEND_DIR="$PROJECT_DIR/frontend-react"
PID_FILE="$FRONTEND_DIR/.vite.pid"
PORT=5173
URL="http://localhost:$PORT"

echo "🚀 Iniciando Frontend React - AGUADA..."

# 1. Verificar se Node.js está instalado
if ! command -v node &> /dev/null; then
    echo "❌ Node.js não está instalado!"
    echo "   Instale com: sudo apt install nodejs npm"
    exit 1
fi

echo "✅ Node.js $(node --version) detectado"

# 2. Ir para o diretório do frontend
cd "$FRONTEND_DIR"

# 3. Instalar dependências se necessário
if [ ! -d "node_modules" ]; then
    echo "📦 Instalando dependências..."
    npm install
else
    echo "✅ Dependências já instaladas"
fi

# 4. Parar servidor anterior se existir
if [ -f "$PID_FILE" ]; then
    OLD_PID=$(cat "$PID_FILE")
    if ps -p "$OLD_PID" > /dev/null 2>&1; then
        echo "⏹️  Parando servidor anterior (PID: $OLD_PID)..."
        kill "$OLD_PID" 2>/dev/null || true
        sleep 2
    fi
    rm -f "$PID_FILE"
fi

# 5. Iniciar servidor Vite em background
echo "🔄 Iniciando servidor Vite..."
npm run dev > /tmp/vite.log 2>&1 &
VITE_PID=$!
echo "$VITE_PID" > "$PID_FILE"

echo "✅ Servidor iniciado (PID: $VITE_PID)"

# 6. Aguardar servidor ficar pronto (máximo 30 segundos)
echo "⏳ Aguardando servidor em $URL..."
for i in {1..30}; do
    if curl -s "$URL" > /dev/null 2>&1; then
        echo "✅ Servidor pronto!"
        break
    fi
    sleep 1
    if [ $i -eq 30 ]; then
        echo "❌ Timeout aguardando servidor"
        echo "📋 Logs do Vite:"
        cat /tmp/vite.log
        exit 1
    fi
done

# 7. Abrir no Simple Browser do VS Code
echo "🌐 Abrindo no Simple Browser..."
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Frontend React: $URL"
echo "  PID: $VITE_PID"
echo "  Logs: /tmp/vite.log"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "Para parar o servidor:"
echo "  kill \$(cat $PID_FILE)"
echo ""

# Exibir logs em tempo real
echo "📋 Logs do servidor (Ctrl+C para sair):"
tail -f /tmp/vite.log
