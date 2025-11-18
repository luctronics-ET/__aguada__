# 📚 Documentação do Sistema AGUADA

## Organização de Documentos

O sistema AGUADA possui três categorias principais de documentação:

### 📖 1. Instruções (`Documents/instrucoes/`)

Guias práticos de operação e manutenção:

| Arquivo | Propósito | Público |
|---------|----------|---------|
| **operacao.md** | Como iniciar, monitorar e usar o sistema | Operadores |
| **calibracao.md** | Procedimentos de calibração de sensores | Técnicos |
| **manutencao.md** | Checklists e rotinas de manutenção | Técnicos |
| **emergencias.md** | Como agir em situações críticas | Todos |

**Quando usar:**
- Operadores e técnicos consultam ao executar uma tarefa
- Procedures de emergência acessadas em momentos críticos
- Referência rápida para operações padrão

### 📋 2. Formulários (`Documents/formularios/`)

Modelos para impressão ou preenchimento digital:

| Arquivo | Propósito | Frequência | Obrigatório |
|---------|----------|-----------|------------|
| **CALIBRACAO.md** | Registro de calibração dos 5 sensores | Mensal | ✓ SIM |
| **MANUTENCAO.md** | Checklist de manutenção preventiva | Mensal | ✓ SIM |
| **INCIDENTE.md** | Registro de problemas e soluções | Conforme necessário | - |

**Como usar:**
- Imprima o formulário ou preencha em PDF
- Arquive em local seguro para auditoria
- Mantenha histórico por 12 meses

### 📊 3. Relatórios (`Documents/relatorios/`)

Modelos para análise de performance:

| Arquivo | Propósito | Frequência | Público |
|---------|----------|-----------|---------|
| **DIARIO.md** | Summary diário (opcional) | Diária | Supervisores |
| **MENSAL.md** | Análise completa do mês | Mensal | Gerência |

**Métricas incluídas:**
- Disponibilidade do sistema
- Performance de cada sensor
- Consumo e tendências
- Incidentes e alertas
- Recomendações

## Fluxo de Trabalho

### Startup Diário
1. ✅ Verificar **operacao.md** - Procedimento de inicialização
2. ✅ Confirmar todos os 5 sensores online
3. ✅ Notar hora e nível inicial em **MANUTENCAO.md**

### Manutenção Mensal
1. 📋 Preencher **CALIBRACAO.md** - Calibrar cada sensor
2. 📋 Preencher **MANUTENCAO.md** - Checklist completo
3. 📊 Gerar **MENSAL.md** - Análise de performance

### Situação de Emergência
1. 🚨 Consultar **emergencias.md** - Procedimento imediato
2. 📋 Documentar em **INCIDENTE.md** - Registro do ocorrido
3. 🔧 Implementar solução conforme guia

### Auditoria/Rastreabilidade
1. 📋 Coletar todos os formulários preenchidos do mês
2. 📊 Revisar relatório mensal
3. 🗂️ Arquivar em pasta com data (e.g., `2025-11/`)

## Checklist de Conformidade

### Semanal
- [ ] Verificar se todos os 5 sensores estão online
- [ ] Confirmar que dados estão sendo coletados
- [ ] Revisar alertas críticos pendentes

### Mensal (OBRIGATÓRIO)
- [ ] Preencher **CALIBRACAO.md** - Calibrar sensores
- [ ] Preencher **MANUTENCAO.md** - Checklist completo
- [ ] Gerar **MENSAL.md** - Análise e recomendações
- [ ] Arquivar formulários preenchidos
- [ ] Validar integridade do banco de dados

### Trimestral
- [ ] Revisão de todos os formulários do trimestre
- [ ] Análise de tendências de problemas
- [ ] Plano de ações corretivas
- [ ] Atualização de procedimentos se necessário

## Acesso aos Documentos

### Via Dashboard
- Página **Documentação** (documentacao.html)
- Todos os arquivos com descrição e links
- Busca integrada

### Via Filesystem
```bash
# Instruções
cat Documents/instrucoes/operacao.md

# Formulários
cat Documents/formularios/CALIBRACAO.md

# Relatórios
cat Documents/relatorios/MENSAL.md
```

### Via Git
```bash
# Clonar repositório
git clone https://github.com/...aguada.git

# Acessar documentação
cd aguada/Documents
```

## Retenção de Documentos

| Tipo | Retenção | Local |
|------|----------|-------|
| Formulários mensais | 12 meses | Documents/ (backup mensal) |
| Relatórios mensais | 24 meses | Documents/relatorios/ |
| Incidentes críticos | 24 meses | Documents/formularios/ |
| Logs do sistema | 90 dias | Backend (auto-limpeza) |

## Estrutura de Pastas

```
Documents/
├── instrucoes/           # Guias de procedimento
│   ├── README.md
│   ├── operacao.md
│   ├── calibracao.md
│   ├── manutencao.md
│   └── emergencias.md
├── formularios/          # Modelos para preenchimento
│   ├── README.md
│   ├── CALIBRACAO.md
│   ├── MANUTENCAO.md
│   └── INCIDENTE.md
└── relatorios/           # Modelos de análise
    ├── README.md
    ├── DIARIO.md
    └── MENSAL.md
```

## Integração com Sistema

A página **documentacao.html** no frontend fornece:
- Acesso rápido a todos os 9 documentos
- Descrição de cada um
- Indicadores de obrigatoriedade
- Busca integrada
- Links para download/visualização

## Próximas Ações

- [ ] Imprimir e arquivar formulários em local seguro
- [ ] Treinar equipe sobre procedimentos
- [ ] Configurar agenda mensal para preenchimento de formulários
- [ ] Implementar backup automático de documentos
- [ ] Criar histórico de versões para cada documento

---

**Última atualização:** 17 de novembro de 2025  
**Versão:** 1.0  
**Responsável:** Operações
