# Relatório: Duplicações e Arquivos Não Usados - AGUADA

**Data:** $(date)
**Status:** ✅ Verificação Completa

---

## 📋 Resumo Executivo

Este relatório identifica duplicações de arquivos backend/frontend e arquivos não utilizados no projeto AGUADA.

---

## 🔍 Duplicações Identificadas

### 1. Docker Compose Files

**Arquivos:**
- `/docker-compose.yml` (raiz do projeto) ✅ **PRINCIPAL**
- `/docker/docker-compose.yml` ⚠️ **DUPLICADO**

**Recomendação:**
- Manter apenas o arquivo na raiz (`/docker-compose.yml`)
- O arquivo em `/docker/docker-compose.yml` parece ser uma versão antiga ou alternativa
- **Ação:** Verificar se `/docker/docker-compose.yml` está sendo usado. Se não, pode ser removido.

### 2. Dockerfile Backend

**Arquivos:**
- `/backend/Dockerfile` ✅ **PRINCIPAL** (Node 22-alpine, mais recente)
- `/docker/Dockerfile.backend` ⚠️ **DUPLICADO** (Node 18-alpine, versão antiga)

**Recomendação:**
- O `docker-compose.yml` na raiz referencia `/backend/Dockerfile`, então este é o principal
- `/docker/Dockerfile.backend` parece ser uma versão antiga
- **Ação:** Verificar se `/docker/Dockerfile.backend` está sendo usado. Se não, pode ser removido.

### 3. Configurações de Sensores

**Arquivos:**
- `/config/sensors.json` ✅ **PRINCIPAL** (configuração do backend)
- `/frontend/config/sensors.json` ✅ **NECESSÁRIO** (configuração do frontend)

**Status:** ✅ **NÃO É DUPLICAÇÃO** - São arquivos diferentes para propósitos diferentes:
- Backend usa para configuração de sensores no banco de dados
- Frontend usa para exibição e mapeamento na interface

---

## 📁 Arquivos Potencialmente Não Usados

### 1. Scripts PHP

**Arquivo:** `/init.php`

**Status:** ⚠️ **POSSIVELMENTE NÃO USADO**

**Análise:**
- O projeto é baseado em Node.js/JavaScript
- Não há referências a PHP no código
- O script parece ser um script de inicialização antigo

**Recomendação:**
- Verificar se há documentação que referencia este arquivo
- Se não for usado, pode ser removido ou movido para `/docs/legacy/`

### 2. Arquivos de Documentação Duplicados

**Arquivos:**
- `/Documents/API_INTEGRATION.md`
- `/docs/FRONTEND_SPEC.md`
- `/docs/FRONTEND_IMPLEMENTATION.md`
- `/docs/FRONTEND_SUMMARY.md`
- `/docs/FRONTEND_IMPROVEMENTS.md`

**Status:** ⚠️ **VERIFICAR CONTEÚDO**

**Recomendação:**
- Consolidar documentação duplicada
- Manter apenas a versão mais atualizada
- Mover versões antigas para `/docs/archive/`

### 3. Scripts de Inicialização

**Arquivos:**
- `/start-frontend-react.sh` ⚠️ **VERIFICAR** (projeto não usa React)
- `/start-real-data.sh` ✅ **PODE SER ÚTIL**
- `/setup.sh` ✅ **PODE SER ÚTIL**

**Recomendação:**
- Verificar se `start-frontend-react.sh` é usado. Se não, remover.
- Manter scripts de setup e inicialização que são úteis.

---

## ✅ Arquivos Corretos (Não São Duplicações)

### Backend
- `/backend/` - Estrutura correta do backend Node.js
- `/backend/src/` - Código fonte do backend
- `/backend/Dockerfile` - Dockerfile principal

### Frontend
- `/frontend/` - Estrutura correta do frontend HTML/JS
- `/frontend/assets/` - Assets do frontend
- `/frontend/config/` - Configurações do frontend

### Docker
- `/docker/` - Configurações Docker adicionais
- `/docker/mosquitto/` - Configuração MQTT
- `/docker/nginx.conf` - Configuração Nginx

### Configuração
- `/config/` - Configurações do sistema
- `/database/` - Scripts de banco de dados

---

## 🎯 Recomendações de Ação

### Prioridade Alta
1. ✅ **Verificar uso de `/docker/docker-compose.yml`**
   - Se não usado, remover ou renomear para `docker-compose.yml.old`

2. ✅ **Verificar uso de `/docker/Dockerfile.backend`**
   - Se não usado, remover ou mover para `/docs/legacy/`

3. ✅ **Verificar uso de `/init.php`**
   - Se não usado, remover ou mover para `/docs/legacy/`

### Prioridade Média
4. ⚠️ **Consolidar documentação duplicada**
   - Revisar arquivos em `/docs/` e `/Documents/`
   - Manter apenas versões atualizadas

5. ⚠️ **Verificar scripts de inicialização**
   - Confirmar se `start-frontend-react.sh` é necessário
   - Documentar propósito de cada script

### Prioridade Baixa
6. 📝 **Organizar arquivos legados**
   - Criar diretório `/docs/legacy/` para arquivos antigos
   - Mover arquivos não usados para lá antes de deletar

---

## 📊 Estatísticas

- **Duplicações Identificadas:** 2 (docker-compose.yml, Dockerfile.backend)
- **Arquivos Potencialmente Não Usados:** 3 (init.php, start-frontend-react.sh, docs duplicados)
- **Arquivos Corretos:** ✅ Todos os outros arquivos estão organizados corretamente

---

## ✅ Conclusão

O projeto está bem organizado, com apenas algumas duplicações menores que podem ser limpas. As principais duplicações são:
1. Arquivos Docker antigos em `/docker/`
2. Script PHP de inicialização que pode não ser mais usado
3. Alguma documentação duplicada que pode ser consolidada

**Recomendação Final:** Fazer uma limpeza cuidadosa dos arquivos identificados, mantendo backups antes de remover qualquer arquivo.

