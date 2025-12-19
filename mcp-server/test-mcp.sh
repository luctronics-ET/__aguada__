#!/bin/bash
# Test MCP servers configuration

echo "=== TESTE DOS MCP SERVERS ==="
echo ""

echo "1. Verificando servidor AGUADA (compilado):"
if [ -f "dist/index.js" ]; then
    echo "   ✓ dist/index.js existe"
    node -c dist/index.js && echo "   ✓ Sintaxe válida" || echo "   ✗ Erro de sintaxe"
else
    echo "   ✗ dist/index.js não encontrado"
fi

echo ""
echo "2. Verificando configuração MCP:"
if [ -f "../.cursor/mcp.json" ]; then
    echo "   ✓ .cursor/mcp.json existe"
    cat ../.cursor/mcp.json | jq '.mcpServers | keys' 2>/dev/null && echo "   ✓ JSON válido" || echo "   ✗ JSON inválido"
else
    echo "   ✗ .cursor/mcp.json não encontrado"
fi

echo ""
echo "3. Testando servidor AGUADA localmente:"
timeout 3 node dist/index.js 2>&1 | head -5

echo ""
echo "=== PARA USAR OS MCP SERVERS ==="
echo "1. Reinicie o VS Code/Cursor"
echo "2. Abra o GitHub Copilot Chat"
echo "3. Os servidores devem aparecer automaticamente"
echo ""
