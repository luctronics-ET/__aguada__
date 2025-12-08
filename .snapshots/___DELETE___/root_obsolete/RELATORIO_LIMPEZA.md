# Relatório de Limpeza - AGUADA
**Data:** 2025-01-27

## Arquivos e Diretórios Não Utilizados Identificados

### 1. Frontend React (Não Existe)
- **Script:** `start-frontend-react.sh` - Referencia `frontend-react/` que não existe
- **Status:** Script não funcional, diretório não existe
- **Ação:** Manter script comentado ou remover se não for usado

### 2. Arquivos de Log Antigos
- `backend/backend.log` - Log antigo do backend
- `backend/logs/aguada.log` - Log antigo
- **Ação:** Manter estrutura de logs, mas arquivos antigos podem ser limpos

### 3. Arquivos de Configuração Antigos
- `firmware/gateway_esp_idf/sdkconfig.old` - Configuração antiga do SDK
- **Ação:** Pode ser removido se não for necessário

### 4. Diretórios de Documentação Redundantes
- `doc/` - Documentação antiga (pode ser redundante com `docs/`)
  - `doc-filelist.js`
  - `doc-script.js`
  - `doc-style.css`
- `frontend/doc/` - Diretório vazio
- **Ação:** Verificar se `doc/` é usado antes de remover

### 5. Script PHP de Inicialização
- `init.php` - Script PHP de inicialização
- **Status:** Não referenciado em scripts principais (apenas em DEBUG_FRONTEND.md)
- **Ação:** Verificar se é necessário ou se pode ser removido

### 6. Arquivos de Configuração Duplicados
- `config/` e `frontend/config/` - Ambos contêm `reservoirs.json` e `sensors.json`
- **Ação:** Verificar qual é usado e consolidar

## Duplicações de Código

### Controllers Duplicados
- `reading.controller.js` tem `getSensorsStatus()` mas não é usado
- `sensors.controller.js` tem `getSensorsStatus()` e é o usado nas rotas
- **Ação:** Remover função duplicada de `reading.controller.js`

## Arquivos Mantidos (Importantes)

- `backend/` - Backend Node.js (ativo)
- `frontend/` - Frontend HTML/JS (ativo)
- `firmware/` - Firmware ESP32 (ativo)
- `docs/` - Documentação principal
- `docker/` - Configurações Docker
- `scripts/` - Scripts de instalação e deploy

## Recomendações

1. **Remover:** `firmware/gateway_esp_idf/sdkconfig.old`
2. **Verificar:** `init.php` - se não for usado, remover
3. **Verificar:** `doc/` - se não for usado, mover para `___DELETE___` ou remover
4. **Limpar:** Logs antigos em `backend/logs/`
5. **Consolidar:** Configurações duplicadas entre `config/` e `frontend/config/`
6. **Remover função duplicada:** `getSensorsStatus()` de `reading.controller.js`

