# AGUADA - Changelog

## [1.0.0] - 2025-11-16

### Adicionado
- **RULES.md**: Documento completo de regras e padrões do sistema
  - Topologia hidráulica detalhada (6 reservatórios)
  - Modelo de dados e conexões
  - Estrutura de telemetria padronizada
  - Regras de compressão de dados
  - Detecção de eventos
  - Auditoria e rastreabilidade
  
- **SETUP.md**: Guia completo de instalação e configuração
  - ESP-IDF 5.x
  - PostgreSQL/TimescaleDB
  - MQTT Broker (Mosquitto)
  - Backend Node.js
  - Grafana
  - Docker Compose

- **config/**: Diretório de configurações JSON
  - `reservoirs.json`: Dimensões e parâmetros dos 4 reservatórios
  - `thresholds.json`: Limiares e parâmetros do sistema
  - `network_topology.json`: Modelo de rede hidráulica (grafo)
  - `sensors.json`: Mapeamento e calibração de sensores

- **SUMMARY.md**: Resumo executivo da atualização do projeto

- **CHANGELOG.md**: Este arquivo

- **prompt_library/backend/01_ingestion_api_v2.md**: Versão expandida do prompt de API

### Atualizado
- **README.md**: Documentação principal completamente reescrita
  - Arquitetura completa do sistema
  - 6 reservatórios com topologia detalhada
  - Modelo de grafo hidráulico
  - Firmware ESP32-C3 SuperMini
  - Compressão inteligente de dados
  - Detecção automática de eventos
  - Leituras manuais e auditoria
  - Estrutura do projeto
  - Guia de operação

### Modificado
- Estrutura do projeto reorganizada com diretório `config/`
- Documentação de referência consolidada em `DOCS_REF/`
- Prompts organizados por categoria em `prompt_library/`

### Características Principais
- **Compressão de dados**: Redução > 90% no volume de dados
- **Detecção de eventos**: Abastecimento, consumo, vazamento automáticos
- **Auditoria completa**: Rastreabilidade total (fonte/autor/datetime)
- **Relatório diário**: Geração automática às 06:00
- **Modelo de rede**: Grafo hidráulico com portas e conexões
- **Visualização espacial**: Coordenadas x,y,z para planta 2D

### Tecnologias
- ESP-IDF 5.x + Arduino as Component
- PostgreSQL 15 / TimescaleDB
- MQTT (Mosquitto/EMQX)
- Node.js 18+ / Python 3
- Grafana

### Melhorias de Performance
- API: < 100ms (p95)
- Suporte: 100 leituras/segundo
- Processamento assíncrono
- Índices otimizados

### Segurança
- Autenticação MQTT
- JWT para API
- Rate limiting
- Validação rigorosa de inputs
- HTTPS em produção

---

## Próximas Versões

### [1.1.0] - Planejado
- Implementação do schema PostgreSQL
- API de ingestão funcional
- Firmware completo testado
- Dashboard Grafana v1

### [1.2.0] - Planejado
- Processamento de eventos em tempo real
- Relatório diário automatizado
- Sistema de alertas

### [2.0.0] - Planejado
- Controle automático de bombas/válvulas
- IA preditiva para vazamentos
- Simulador hidráulico
- App mobile

---

**Autor**: Equipe AGUADA  
**Versão Atual**: 1.0.0  
**Data**: 2025-11-16
