# AGUADA MCP Server - Quick Start Guide

## ✅ MCP Server Configurado com Sucesso!

O servidor MCP do AGUADA está instalado e pronto para uso.

### 📁 Estrutura Criada

```
mcp-server/
├── src/
│   └── index.ts          # Implementação do servidor
├── dist/
│   └── index.js          # Código compilado
├── package.json          # Dependências
├── tsconfig.json         # Configuração TypeScript
├── README.md             # Documentação completa
└── test.sh               # Suite de testes
```

### 🔧 Configuração do VS Code

O arquivo `.vscode/settings.json` foi atualizado com:

```json
{
  "mcp.servers": {
    "aguada": {
      "command": "node",
      "args": [
        "/home/luciano/Área de trabalho/__aguada__/mcp-server/dist/index.js"
      ]
    }
  }
}
```

### 🚀 Como Usar

#### 1. Recarregar VS Code
```
Ctrl+Shift+P > Developer: Reload Window
```

#### 2. Ferramentas Disponíveis

O servidor MCP fornece 5 ferramentas:

**`get_telemetry`**
- Buscar dados de telemetria dos sensores
- Parâmetros: `node_id`, `start_time`, `end_time`

**`get_reservoir_status`**
- Status atual de um reservatório
- Parâmetros: `reservoir_id` (CAV, CAM, CAS, CI)

**`get_system_overview`**
- Visão geral completa do sistema
- Sem parâmetros

**`analyze_consumption`**
- Análise de padrões de consumo
- Parâmetros: `period` (daily, weekly, monthly)

**`check_events`**
- Verificar eventos hidráulicos
- Parâmetros: `event_type`, `hours`

#### 3. Recursos Disponíveis

**Configurações:**
- `aguada://config/reservoirs` - JSON dos reservatórios
- `aguada://config/sensors` - JSON dos sensores
- `aguada://config/topology` - Topologia da rede

**Documentação:**
- `aguada://docs/schema` - Schema do banco de dados
- `aguada://docs/api` - Documentação da API

### 🧪 Testar o Servidor

#### Opção 1: MCP Inspector (Recomendado)
```bash
cd mcp-server
npm run inspector
```

Abre interface gráfica para testar todas as ferramentas e recursos.

#### Opção 2: Linha de Comando
```bash
cd mcp-server
npm start
```

O servidor aguarda comandos via stdio (usado pelo VS Code/Claude).

#### Opção 3: Suite de Testes
```bash
cd mcp-server
./test.sh
```

### 📊 Exemplo de Uso no Copilot

No VS Code, você pode usar o Copilot para interagir com o MCP:

```
"Use o MCP server aguada para me mostrar o status do reservatório CAV"

"Liste todos os sensores configurados no sistema AGUADA"

"Analise o consumo de água do último mês"

"Verifique se houve eventos de vazamento nas últimas 24 horas"
```

### 🔗 Integração com o Sistema

O MCP server **lê** os arquivos de configuração do projeto:
- `/config/reservoirs.json` ✅
- `/config/sensors.json` ✅
- `/config/network_topology.json` ✅
- `/database/schema.sql` ✅

Em **produção**, conecte ao PostgreSQL/TimescaleDB para dados em tempo real.

### 🛠️ Desenvolvimento

#### Recompilar após mudanças:
```bash
cd mcp-server
npm run build
```

#### Modo watch (recompila automaticamente):
```bash
cd mcp-server
npm run dev
```

### 📝 Próximos Passos

1. ✅ **MCP Server Instalado e Funcionando**
2. ⏳ **Recarregar VS Code** para ativar
3. ⏳ **Testar ferramentas** via Copilot ou MCP Inspector
4. ⏳ **Conectar ao banco de dados** (substituir dados mock)
5. ⏳ **Adicionar autenticação** para produção

### 🐛 Troubleshooting

**Servidor não aparece no VS Code?**
- Recarregue a janela: `Ctrl+Shift+P > Reload Window`
- Verifique o caminho em `.vscode/settings.json`

**Erro ao compilar?**
```bash
cd mcp-server
rm -rf node_modules dist
npm install
npm run build
```

**Testar conexão básica:**
```bash
cd mcp-server
timeout 2 node dist/index.js
# Deve exibir: "AGUADA MCP Server running on stdio"
```

---

## 🎉 Servidor MCP Configurado!

O AGUADA MCP Server está pronto para fornecer contexto inteligente sobre o sistema de monitoramento hidráulico diretamente no VS Code.
