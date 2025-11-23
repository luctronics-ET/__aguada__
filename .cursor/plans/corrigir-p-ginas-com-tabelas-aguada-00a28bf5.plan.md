<!-- 00a28bf5-fb0f-4411-9541-af5ecf8a5e99 08bc4854-26cf-4895-80ab-8c52928374a6 -->
# Plano: Corrigir Páginas com Tabelas - AGUADA

## Objetivo

Conectar todas as páginas HTML que contêm tabelas aos dados reais da API, garantindo que exibam informações atualizadas e funcionais.

## Páginas Identificadas com Tabelas

1. **dados.html** - Tabela de leituras brutas (raw readings)
2. **system.html** - Tabela de status dos sensores
3. **alerts.html** - Tabela de eventos/alertas
4. **history.html** - Tabela de histórico de leituras

## Ordem de Execução (Prioridade: Tabelas Primeiro)

### Fase 1: dados.html (Tabela de Leituras Brutas)

**Arquivo:** `frontend/dados.html`
**Endpoint API:** `GET /api/readings/raw`

**Tarefas:**

- Verificar se o código de carregamento de dados está completo
- Garantir que a paginação funciona corretamente
- Verificar se os filtros (sensor, variável, timestamp) funcionam
- Corrigir formatação de valores (distance_cm, válvulas, etc.)
- Adicionar tratamento de erros e estados de loading
- Verificar se o auto-refresh está funcionando
- Garantir que o campo `meta` está sendo parseado corretamente
- Adicionar formatação adequada para MAC address, bateria, RSSI, uptime

**Verificações:**

- Função `loadData()` está chamando o endpoint correto
- Função `updateTable()` está renderizando corretamente
- Funções de paginação (`firstPage`, `prevPage`, `nextPage`, `lastPage`) funcionam
- Funções de filtro e ordenação funcionam

---

### Fase 2: system.html (Tabela de Status dos Sensores)

**Arquivo:** `frontend/system.html`
**Endpoint API:** `GET /api/sensors/status`

**Tarefas:**

- Remover dados estáticos hardcoded da tabela
- Conectar à API `/api/sensors/status`
- Renderizar dinamicamente os dados dos sensores
- Exibir status online/offline baseado em `ultima_leitura`
- Calcular e exibir "há X tempo" da última leitura
- Exibir bateria, RSSI e uptime dos sensores
- Adicionar indicadores visuais de status (badges)
- Atualizar contador de "nós ativos" dinamicamente
- Adicionar polling automático para atualizar status

**Estrutura da tabela:**

- Sensor (elemento_id)
- MAC Address (node_mac)
- Status (online/offline/warning)
- Última Leitura (tempo relativo)
- Bateria (formato: X.XX V)
- RSSI (formato: -XX dBm)
- Uptime (formato: Xd Xh)

---

### Fase 3: alerts.html (Tabela de Eventos)

**Arquivo:** `frontend/alerts.html`
**Endpoint API:** `GET /api/alerts`

**Tarefas:**

- Verificar se a função `renderEventsTable()` está funcionando
- Garantir que os filtros (crítico, aviso, info) funcionam
- Verificar formatação de timestamps
- Garantir que botões de ação (Resolver) funcionam
- Adicionar paginação se necessário (para muitos alertas)
- Melhorar formatação de badges de nível (crítico, aviso, info)

**Verificações:**

- Tabela está sendo populada corretamente
- Filtros estão funcionando
- Botão "Resolver" está chamando a API corretamente

---

### Fase 4: history.html (Tabela de Histórico)

**Arquivo:** `frontend/history.html`
**Endpoint API:** `GET /api/readings/history/:sensor_id`

**Tarefas:**

- Verificar se a função `loadReadings()` está funcionando corretamente
- Garantir que os filtros (sensor, tipo, período) funcionam
- Verificar formatação de valores na tabela
- Garantir que a paginação funciona (se implementada)
- Adicionar ordenação por colunas
- Melhorar tratamento de dados ausentes
- Verificar se as estatísticas (máximo, mínimo, média, desvio) estão corretas

**Verificações:**

- Dados estão sendo buscados corretamente
- Filtros estão aplicando corretamente
- Formatação de timestamps está correta
- Estatísticas estão sendo calculadas corretamente

---

## Padrões a Seguir

### 1. Estrutura de Código

- Sempre verificar se `window.apiService` está disponível
- Usar `apiService.getSensorsStatus()`, `apiService.getAlerts()`, etc.
- Adicionar tratamento de erros com try/catch
- Mostrar estados de loading enquanto busca dados
- Mostrar mensagens de erro amigáveis

### 2. Formatação de Dados

- Timestamps: usar `formatDateTime()` ou `formatTime()` de `app.js`
- Bateria: converter mV para V (dividir por 1000)
- RSSI: exibir com unidade "dBm"
- Uptime: formatar como "Xd Xh" ou "Xh Xm"
- MAC Address: exibir em formato monospace

### 3. Atualização Automática

- Adicionar polling periódico (10-30 segundos)
- Usar `setInterval()` para atualizações automáticas
- Limpar intervalos ao sair da página (`beforeunload`)

### 4. Estados Visuais

- Loading: mostrar spinner ou "Carregando..."
- Vazio: mostrar "Nenhum dado encontrado"
- Erro: mostrar mensagem de erro com opção de recarregar
- Sucesso: exibir dados formatados

---

## Checklist de Validação

Para cada página, verificar:

- [ ] Dados estão sendo buscados da API
- [ ] Tabela está sendo populada corretamente
- [ ] Filtros funcionam
- [ ] Ordenação funciona (se aplicável)
- [ ] Paginação funciona (se aplicável)
- [ ] Formatação de dados está correta
- [ ] Estados de loading/erro estão implementados
- [ ] Atualização automática está funcionando
- [ ] Código está limpo e bem estruturado

---

## Notas Técnicas

- Todas as páginas devem usar `api-service.js` para chamadas à API
- Usar funções de formatação de `app.js` quando disponíveis
- Manter consistência com outras páginas já corrigidas
- Adicionar logs no console para debug (`console.log`, `console.error`)
- Garantir que não há dados hardcoded nas tabelas

### To-dos

- [ ] Corrigir tabela de leituras brutas em dados.html - verificar carregamento, paginação, filtros e formatação
- [ ] Conectar tabela de status dos sensores em system.html à API /api/sensors/status - remover dados estáticos
- [ ] Verificar e corrigir tabela de eventos em alerts.html - garantir que renderEventsTable funciona corretamente
- [ ] Verificar e corrigir tabela de histórico em history.html - garantir filtros, formatação e estatísticas