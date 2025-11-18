# AGUADA - Resumo da Atualização do Projeto

**Data:** 16 de novembro de 2025  
**Versão:** 1.0  
**Status:** ✅ Completo

---

## 📋 Tarefas Concluídas

### 1. ✅ RULES.md - Regras e Padrões do Sistema
**Arquivo:** `/RULES.md`

Criado documento completo com:
- Visão geral do sistema (6 reservatórios, bombas, válvulas)
- Topologia hidráulica detalhada (CON, CAV, IE01, IE02, B03)
- Modelo de dados (elementos, portas, conexões)
- Estrutura de telemetria padronizada
- Regras de compressão de dados (deadband, mediana, temporal)
- Cálculos físicos (volume, pressão, vazão)
- Detecção de eventos (abastecimento, consumo, vazamento)
- Auditoria e rastreabilidade (fonte, autor, datetime)
- Relatório diário às 06:00
- Padrões de código (firmware, backend, database)
- Segurança e confiabilidade

---

### 2. ✅ README.md - Documentação Principal Atualizada
**Arquivo:** `/readme.md`

Atualizado com:
- Sistema completo de 6 reservatórios
- Topologia detalhada da rede hídrica
- Modelo de dados e conexões (grafo hidráulico)
- Firmware Node Tipo 10 (ESP32-C3)
- Pinout completo
- Estrutura de telemetria lean
- Comunicação MQTT + HTTP fallback
- Backend pipeline
- Detecção inteligente de eventos
- Compressão de dados (redução > 90%)
- Relatório diário às 06:00
- Leituras manuais e auditoria
- Requisitos e instalação
- Estrutura do projeto
- Troubleshooting
- Próximas evoluções

---

### 3. ✅ SETUP.md - Guia de Configuração do Ambiente
**Arquivo:** `/SETUP.md`

Guia completo de instalação:
- Requisitos de hardware e sistema operacional
- Instalação do ESP-IDF 5.x
- Configuração VSCode + extensão ESP-IDF
- Arduino as Component
- PostgreSQL 15 / TimescaleDB
- MQTT Broker (Mosquitto)
- Backend Node.js 18+
- Grafana
- Docker Compose (alternativa simplificada)
- Configuração dos nodes ESP32
- Testes e verificação
- Troubleshooting

---

### 4. ✅ Configurações do Sistema
**Diretório:** `/config/`

#### 4.1 reservoirs.json
Dimensões e parâmetros dos 5 reservatórios:
- RCON - Castelo de Consumo - 81.7 m³ cilíndrico - hsensor 40cm
- RCAV - Castelo de Incêndio - 81.7 m³ cilíndrico - hsensor 20cm (nível crítico: 70%)
- RB03 - Reservatório Casa Bombas - 80 m³ cilíndrico - Casa de Bombas N03
- IE01 - Cisterna IE 01 - 254 m³ retangular - hsensor 20cm
- IE02 - Cisterna IE 02 - 254 m³ retangular - hsensor 20cm (mesmo ESP que IE01)

Inclui:
- Dimensões físicas
- Cálculos de volume
- Níveis de alerta
- Coordenadas espaciais (x, y, z)
- Conexões hidráulicas
- Fórmulas de cálculo

#### 4.2 thresholds.json
Limiares e parâmetros do sistema:
- Compressão de dados (deadband: 2cm, window: 11 amostras)
- Eventos (abastecimento, consumo, vazamento)
- Qualidade de dados (ranges físicos)
- Alertas (níveis críticos, sensores offline)
- Telemetria (intervalos, retries)
- Constantes físicas

#### 4.3 network_topology.json
Modelo de rede hidráulica:
- Elementos (reservatórios, bombas, válvulas)
- Portas (entradas/saídas)
- Conexões entre elementos
- Regras de fluxo
- Condições de operação

#### 4.4 sensors.json
Mapeamento e calibração de sensores:
- 5 sensores ultrassônicos AJ-SR04M
- 4 nodes ESP32-C3 SuperMini (1 deles com 2 sensores)
- Mapeamento MAC → sensor_type → reservatório (feito no backend)
- Tipos: TYPE_SINGLE_ULTRA (3x) e TYPE_DUAL_ULTRA (1x)
- Parâmetros de calibração por sensor
- Procedimento de calibração padrão trimestral
- Status, localização e GPIO mapping

---

### 5. ✅ Prompts Atualizados
**Diretório:** `/prompt_library/`

#### Backend
- **01_ingestion_api_v2.md**: API REST completa com validação, auditoria, pipeline de eventos
- Endpoints: telemetria automática, leituras manuais, calibração
- Processamento: volume, compressão temporal, detecção de eventos

#### (Outros prompts mantidos para referência)

---

## 🎯 Principais Melhorias Implementadas

### 1. Modelo de Rede Hidráulica Completo
- **Grafo de conexões** entre elementos
- **Portas de entrada/saída** explícitas
- **Regras de fluxo** baseadas em estados
- **Coordenadas espaciais** para visualização

### 2. Compressão Inteligente de Dados
- **Redução > 90%** no volume de dados
- **Tabela dupla**: leituras_raw (todas) + leituras_processadas (mudanças)
- **Deadband temporal**: atualiza data_fim enquanto valor estável
- **Mediana de 11 amostras** para filtrar ruído

### 3. Detecção Automática de Eventos
- **Abastecimento**: ΔV > 50L + bomba ON + válvula ABERTA
- **Consumo**: cálculo por período (00-06h, 06-12h, 12-18h, 18-24h)
- **Vazamento**: queda lenta > 1h sem bombeamento
- **Nível crítico CAV**: < 70% → alerta urgente

### 4. Auditoria Completa
- **Campos obrigatórios**: fonte, autor, datetime, modo
- **Leituras manuais**: usuários podem inserir hidrômetros
- **Calibração**: registro de ajustes e responsáveis
- **Rastreabilidade**: todo dado tem origem identificada

### 5. Relatório Diário às 06:00
- **Resumo de volumes** (inicial/final)
- **Consumo por período** (4 faixas horárias)
- **Eventos significativos**
- **Status de equipamentos**
- **Alertas e recomendações**

---

## 📊 Estrutura do Projeto Atualizada

```
aguada/
├── RULES.md                     ✅ NOVO
├── SETUP.md                     ✅ NOVO
├── readme.md                    ✅ ATUALIZADO
├── DOCS_REF/
│   ├── AGUADA REF DEF RULES.txt
│   ├── chatgptAguada30OUT.txt
│   └── ESP32_C3_SUPER_MINI_PINOUT.md
├── config/                      ✅ NOVO
│   ├── reservoirs.json          ✅ NOVO
│   ├── thresholds.json          ✅ NOVO
│   ├── network_topology.json    ✅ NOVO
│   └── sensors.json             ✅ NOVO
├── prompt_library/
│   ├── backend/
│   │   ├── 01_ingestion_api.md
│   │   ├── 01_ingestion_api_v2.md  ✅ NOVO
│   │   ├── 02_volume_consumption_engine.md
│   │   └── 03_daily_report_06h.md
│   ├── database/
│   │   └── 01_hydraulic_schema.sql.md
│   ├── firmware/
│   │   ├── 01_telemetry_struct.md
│   │   ├── 02_generic_firmware.md
│   │   ├── 03_sensor_fault_detection.md
│   │   └── firmware_node10/
│   └── frontend/
│       └── 01_dashboard_ui.md
├── backend/                     (a implementar)
├── database/                    (a implementar)
└── dashboard/                   (a implementar)
```

---

## 🚀 Próximos Passos Recomendados

### Curto Prazo (1-2 semanas)
1. Implementar schema PostgreSQL conforme `config/` e RULES.md
2. Criar API de ingestão seguindo `01_ingestion_api_v2.md`
3. Configurar MQTT broker e testar comunicação
4. Flash firmware no primeiro ESP32-C3 (node_04 - CON)
5. Configurar dashboard Grafana básico

### Médio Prazo (1 mês)
1. Implementar processamento de eventos
2. Configurar relatório diário às 06:00
3. Deploy completo dos 4 nodes (CON, CAV, IE01, IE02)
4. Calibrar todos os sensores
5. Testes de vazamento simulado

### Longo Prazo (3-6 meses)
1. Controle automático de bombas/válvulas
2. IA preditiva para vazamentos
3. Simulador hidráulico
4. App mobile para operadores
5. Integração geoespacial (mapa real)

---

## 📝 Observações Importantes

### 1. Configurações Críticas
- **CAV (Incêndio)**: nível mínimo **70%** - alerta urgente se abaixo
- **Deadband**: **2 cm** - ajustar conforme ruído real dos sensores
- **Window size**: **11 amostras** - mediana para filtrar ruído
- **Relatório**: **06:00** diariamente - configurar cron job

### 2. Calibração de Sensores
- **Frequência**: trimestral
- **Procedimento**: definido em `config/sensors.json`
- **Condições**: sem bombeamento, sem vento forte
- **Registro**: sempre documentar no banco de dados

### 3. Auditoria
- **Toda modificação** deve ter: fonte, autor, datetime
- **Leituras manuais**: require autenticação de usuário
- **Logs estruturados**: JSON com todas as operações

### 4. Performance
- **API**: < 100ms (p95)
- **Suportar**: 100 leituras/segundo
- **Processamento**: assíncrono (não bloquear API)
- **Índices**: (sensor_id, datetime), (processed)

---

## ✅ Checklist de Validação

- [x] RULES.md criado e completo
- [x] README.md atualizado com arquitetura completa
- [x] SETUP.md com guia de instalação detalhado
- [x] config/reservoirs.json com 4 reservatórios
- [x] config/thresholds.json com todos os limiares
- [x] config/network_topology.json com grafo hidráulico
- [x] config/sensors.json com mapeamento completo
- [x] Prompt backend atualizado (ingestion API v2)
- [ ] Schema PostgreSQL implementado
- [ ] API backend implementada
- [ ] MQTT broker configurado
- [ ] Firmware testado em hardware
- [ ] Dashboard Grafana configurado
- [ ] Testes end-to-end realizados

---

## 📧 Suporte

Para dúvidas sobre:
- **Instalação**: consulte `SETUP.md`
- **Arquitetura**: consulte `RULES.md`
- **Configurações**: consulte `config/*.json`
- **Desenvolvimento**: consulte `prompt_library/`

---

**Conclusão:** O projeto AGUADA está com documentação completa, arquitetura definida, configurações prontas e guias de instalação detalhados. Pronto para implementação do código backend, firmware e dashboard.

**Autor:** Equipe AGUADA  
**Versão:** 1.0  
**Data:** 2025-11-16
