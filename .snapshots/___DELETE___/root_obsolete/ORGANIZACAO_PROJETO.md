# 📁 Organização do Projeto AGUADA

**Data:** 2025-11-20  
**Status:** ✅ Concluído

## Resumo da Organização

Foi criada a pasta `___DELETE___` para armazenar arquivos obsoletos, redundantes ou não utilizados no projeto.

## Arquivos Movidos

### 📦 Backups
- `firmware/node_sensor_10/main/main.c.backup` - Backup do main.c
- `firmware/gateway_esp_idf/sdkconfig.old` - Configuração antiga do SDK

### 🔧 Firmware Obsoleto
- `firmware/gateway_00/` - Versão antiga do gateway (substituída por `gateway_esp_idf`)
- `firmware/node_10/` - Versão antiga do sensor node (substituída por `node_sensor_10` e `node_sensor_20`)

### 🧪 Testes Antigos
- `backend/server-test-realdata.js` - Teste antigo do servidor
- `backend/test-serial-monitor.js` - Teste antigo de monitor serial

### 📄 Documentação Redundante
- `REVISAO_CONCLUIDA.txt` - Resumo de revisão antigo
- `REORGANIZACAO_RESUMO.md` - Resumo de reorganização antigo
- `REVIEW_SUMMARY.md` - Resumo de revisão
- `IMPLEMENTATION_SUMMARY.md` - Resumo de implementação
- `CHANGES.md` - Mudanças (redundante com CHANGELOG)
- `SUMMARY.sh` - Script de resumo antigo
- `doc/` - Documentação antiga (substituída por `docs/`)

### 🗂️ Workspace Antigo
- `firmware/gateway_00_arduino/__aguada__.code-workspace` - Workspace do IDE
- `firmware/gateway_00_arduino/doc/` - Documentação do gateway Arduino
- `dashboard/` - Pasta de dashboard não utilizada

### 📋 Logs
- `backend/backend.log` - Log antigo
- `backend/logs/` - Diretório de logs antigos

## Estrutura Final do Projeto

```
aguada/
├── backend/              # Backend Node.js
├── frontend/             # Frontend HTML/JS
├── frontend-react/       # Frontend React (alternativo)
├── firmware/             # Firmware ESP32
│   ├── gateway_esp_idf/  # Gateway ESP-IDF (ativo)
│   ├── gateway_00_arduino/ # Gateway Arduino (legado)
│   ├── node_sensor_10/   # Sensor Node 10 (ativo)
│   └── node_sensor_20/   # Sensor Node 20 (ativo)
├── config/              # Configurações do sistema
├── database/            # Scripts SQL
├── docs/                # Documentação principal
├── Documents/           # Documentos operacionais
├── scripts/             # Scripts de deploy/manutenção
├── docker/              # Configurações Docker
├── mcp-server/          # Servidor MCP
└── ___DELETE___/        # Arquivos obsoletos (para revisão)
```

## Próximos Passos

1. ✅ Revisar arquivos em `___DELETE___` antes de deletar permanentemente
2. ✅ Manter apenas firmware ativo (`gateway_esp_idf`, `node_sensor_10`, `node_sensor_20`)
3. ✅ Consolidar documentação em `docs/`
4. ✅ Manter estrutura organizada e limpa

## Observações

- A pasta `___DELETE___` está no `.gitignore` para não ser versionada
- Arquivos podem ser recuperados se necessário
- Após confirmação, os arquivos podem ser deletados permanentemente

