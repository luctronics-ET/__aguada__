# MCP Servers - Configuração Automática

## ✅ Status: CONFIGURADO

Os MCP (Model Context Protocol) servers estão **configurados para iniciar automaticamente** quando você abre este workspace no VS Code/Cursor.

## 📁 Arquivos de Configuração

### VS Code/Cursor

- **Arquivo**: `.cursor/mcp.json` ou `.vscode/mcp.json`
- **Localização**: Raiz do projeto

## 🚀 Servidores Ativos

### 1. **aguada** (Customizado)

- **Path**: `mcp-server/dist/index.js`
- **Status**: ✅ Compilado
- **Função**: Acesso aos dados e configurações do projeto AGUADA
- **Tools disponíveis**:
  - `read_config` - Ler arquivos de configuração JSON
  - `read_schema` - Ler schema do banco de dados
  - `list_reservoirs` - Listar reservatórios cadastrados
  - `list_sensors` - Listar sensores do sistema

### 2. **filesystem**

- **Package**: `@modelcontextprotocol/server-filesystem`
- **Status**: ✅ Ativo
- **Função**: Acesso ao sistema de arquivos do projeto
- **Escopo**: `/home/luciano/Área de trabalho/aguada`

### 3. **postgres**

- **Package**: `@modelcontextprotocol/server-postgres`
- **Status**: ✅ Ativo
- **Função**: Consultas SQL diretas ao banco
- **Conexão**: `postgresql://aguada_user:aguada123@localhost:5433/aguada_db`
- **Schema**: `aguada`

### 4. **huggingface**

- **Package**: `@llmindset/hf-mcp-server`
- **Status**: ✅ Ativo
- **Função**: Acesso a modelos e datasets do Hugging Face

### 5. **github**

- **Type**: HTTP
- **URL**: `https://api.githubcopilot.com/mcp/`
- **Status**: ✅ Ativo
- **Função**: Integração com GitHub

### 6. **figma** & **figma-desktop**

- **Type**: HTTP
- **Status**: ✅ Ativo
- **Função**: Integração com Figma (web e desktop)

### 7. **markitdown**

- **Command**: `uvx markitdown-mcp`
- **Status**: ✅ Ativo
- **Função**: Conversão de documentos para Markdown

## 🔧 Como Usar

### 1. Reiniciar o Editor

```bash
# Feche e reabra o VS Code/Cursor
# ou use: Ctrl+Shift+P → "Developer: Reload Window"
```

### 2. Verificar no Copilot Chat

- Abra o GitHub Copilot Chat (Ctrl+Shift+I)
- Os servidores MCP devem aparecer automaticamente
- Você pode usar comandos como:
  ```
  @workspace usando o servidor postgres, mostre os últimos 5 registros
  @workspace usando o servidor aguada, liste os reservatórios
  ```

### 3. Testar Conexão

```bash
cd mcp-server
./test-mcp.sh
```

## 🔍 Troubleshooting

### MCP Servers não aparecem

1. Verifique se o arquivo `.cursor/mcp.json` existe
2. Reinicie completamente o VS Code/Cursor
3. Verifique os logs: `View → Output → GitHub Copilot`

### Servidor AGUADA não funciona

```bash
cd mcp-server
npm install
npm run build
```

### Erro de conexão Postgres

- Verifique se o PostgreSQL está rodando: `ps aux | grep postgres`
- Teste a conexão: `psql -U aguada_user -d aguada_db -h localhost -p 5433`
- Senha correta: `aguada123`

## 📦 Recompilar Servidor AGUADA

Se modificar o código TypeScript:

```bash
cd mcp-server
npm run build
# Reinicie o VS Code/Cursor
```

## 🎯 Próximos Passos

1. ✅ MCP servers configurados
2. ⚠️ Registrar sensor no banco de dados
3. ⚠️ Testar telemetria (POST /api/telemetry)
4. ⚠️ Verificar dados salvos

**Execute:**

```bash
# 1. Registrar sensor
psql -U aguada_user -d aguada_db -h localhost -p 5433 -f REGISTER_SENSOR.sql

# 2. Testar telemetria
./test-telemetry.sh

# 3. Verificar sistema
./check-system.sh
```

## 📚 Documentação

- [MCP Protocol](https://modelcontextprotocol.io/)
- [MCP TypeScript SDK](https://github.com/modelcontextprotocol/typescript-sdk)
- [VS Code MCP Extension](https://marketplace.visualstudio.com/items?itemName=modelcontextprotocol.mcp)

---

**Última atualização**: 2025-12-10
