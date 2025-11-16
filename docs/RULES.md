# AGUADA - Regras e Padrões do Sistema

## 1. Visão Geral do Sistema

AGUADA é um sistema supervisório IoT para monitoramento e gestão de redes hídricas, composto por:
- **6 reservatórios** monitorados
- **5-10 bombas** de recalque
- **~20 válvulas** de controle
- **Sensores ultrassônicos** AJ-SR04M para medição de nível
- **Nodes ESP32-C3** para telemetria
- **Gateway MQTT** para comunicação
- **Backend PostgreSQL/TimescaleDB** para persistência
- **Dashboard Grafana/Web** para visualização

---

## 2. Topologia Hidráulica

### 2.1 Reservatórios Principais

#### Castelo de Consumo (CON)
- Capacidade: 80 m³
- Altura útil: 400 cm
- Diâmetro: 510 cm
- Sensor offset: 40 cm
- **Variação diária significativa** (foco principal de monitoramento)
- Válvulas: entrada (IE), saída AZ (Área Azul), saída AV (Área Vermelha)

#### Castelo de Incêndio (CAV)
- Capacidade: 80 m³
- Altura útil: 400 cm
- Diâmetro: 510 cm
- Sensor offset: 20 cm
- **Nível mínimo crítico: 70%**
- Rede independente com tomadas em Y
- Uso esporádico, esvaziamento rápido em eventos

#### Cisternas IE01 e IE02
- Capacidade: 254 m³ cada
- Altura útil: 240 cm
- Comprimento: 585 cm
- Largura: 1810 cm
- Sensor offset: 20 cm
- Formato retangular
- Válvulas: saída CON, saída CAV, entrada 01, entrada 02

---

## 3. Modelo de Dados

### 3.1 Estrutura de Elementos

Cada elemento hidráulico (reservatório, bomba, válvula) possui:

```json
{
  "id": "elemento_id",
  "tipo": "reservatorio|bomba|valvula|rede|consumidor",
  "nome": "string",
  "coordenadas": {"x": float, "y": float, "z": float},
  "estado": "ativo|inativo|alerta",
  "conexoes": {
    "entradas": ["porta_id"],
    "saidas": ["porta_id"]
  }
}
```

### 3.2 Portas e Conexões

Modelo de grafo hidráulico:

```
Elemento.porta_saida -> Conexão -> Elemento.porta_entrada
```

Exemplo:
```
res_cons.in01 = valv_conAV.out
```

Água só flui se:
- Válvulas intermediárias estão ABERTAS
- Bombas intermediárias estão ON
- Pressão é suficiente

### 3.3 Coordenadas Espaciais

Cada elemento tem coordenadas para visualização em:
- **Planta 2D** (x, y em metros ou pixels)
- **Mapa geográfico** (latitude, longitude) - opcional
- **Altitude** (z em metros)

---

## 4. Telemetria e Compressão de Dados

### 4.1 Estrutura Padronizada

```cpp
struct Telemetry {
    String mac;           // MAC do node
    uint32_t ts;          // Unix timestamp
    String data_label;    // ex: "ultra1_cm", "valve_in"
    float data_value;     // valor medido
    float battery;        // tensão bateria (mV)
    int rssi;             // força sinal WiFi
}
```

### 4.2 Regras de Transmissão

**Enviar SOMENTE quando houver mudança significativa:**

- Nível de água: variação > ±2 cm
- Pressão: variação > ±0.2 bar
- Estado de válvula/bomba: qualquer mudança
- Periodicidade máxima: 30 segundos (mesmo sem mudança)

### 4.3 Filtragem de Ruído

```cpp
// Aplicar mediana de N amostras
const int MEDIAN_SAMPLES = 11;

// Deadband para ignorar ruído
const float DEADBAND_CM = 2.0;

// Desvio padrão aceitável para "estável"
const float STABLE_STDDEV = 0.5;
```

### 4.4 Compressão no Banco

#### Tabela leituras_raw
- Armazena TODAS as leituras recebidas
- Índices: (sensor_id, datetime), (processed)
- Retention: 180 dias

#### Tabela leituras_processadas
- Armazena APENAS mudanças significativas
- Atualiza `data_fim` enquanto valor permanece estável
- Cria novo registro quando valor muda além do deadband

**Algoritmo:**
```python
if abs(valor_atual - valor_anterior) <= DEADBAND:
    UPDATE leituras_processadas SET data_fim = now()
else:
    INSERT INTO leituras_processadas (...)
```

---

## 5. Cálculos Físicos

### 5.1 Volume a partir do Nível

#### Reservatório Cilíndrico
```python
raio_m = diametro_cm / 200.0
volume_m3 = π * raio_m² * (nivel_cm / 100.0)
percentual = (volume_m3 / volume_max_m3) * 100
```

#### Reservatório Retangular
```python
volume_m3 = (comprimento_m * largura_m * nivel_cm) / 100.0
percentual = (volume_m3 / volume_max_m3) * 100
```

### 5.2 Pressão Estática

```python
# No fundo do reservatório
P_Pa = ρ * g * h
P_bar = P_Pa / 100000

# ρ água = 1000 kg/m³
# g = 9.81 m/s²
```

### 5.3 Vazão Estimada

```python
# Simplificado (orifício)
Q_m3s = C * sqrt(ΔP)

# Perda de carga
ΔP_loss = K_loss * Q
```

---

## 6. Detecção de Eventos

### 6.1 Abastecimento

**Condições:**
- ΔVolume > +50 L
- Bomba = ON
- Válvula entrada = ABERTA
- Duração > 5 minutos

**Ação:**
```sql
INSERT INTO eventos (tipo, reservatorio_id, volume_estimado_l, ...)
VALUES ('ABASTECIMENTO', 'res_cons', 520, ...)
```

### 6.2 Consumo

**Condições:**
- ΔVolume < 0 (queda gradual)
- Válvula saída = ABERTA
- Bomba consumo = ON (se aplicável)

**Métricas:**
```python
consumo_diario_L = sum(volumes_negativos) over 24h
consumo_por_periodo = consumo[00-06h, 06-12h, 12-18h, 18-24h]
```

### 6.3 Vazamento

**Condições:**
- Queda lenta e contínua > 1 hora
- Taxa: -15 L/h ou mais
- Todas válvulas saída = FECHADAS
- Bombas = OFF

**Ação:**
```sql
INSERT INTO anomalias (tipo, nivel_alerta, descricao, ...)
VALUES ('VAZAMENTO_SUSPEITO', 'MODERADO', 'queda 18L/h', ...)
```

### 6.4 Rede de Incêndio Crítica

**Condições:**
- Nível CAV < 70%
- Duração > 10 minutos

**Ação:**
- Alerta sonoro
- Notificação imediata
- Registro em eventos críticos

### 6.5 Falha de Sensor

**Condições:**
- Leitura travada (mesmo valor por > 1h)
- Valor fora de faixa física (< 0 ou > altura_max)
- Jitter > 3x desvio padrão normal
- Timeout de comunicação > 5 minutos

---

## 7. Auditoria e Rastreabilidade

### 7.1 Campos Obrigatórios

Toda leitura/modificação deve registrar:

```json
{
  "fonte": "sensor|usuario|sistema",
  "autor": "node_id | usuario_nome | processo",
  "datetime": "ISO8601 timestamp",
  "modo": "automatica|manual"
}
```

### 7.2 Leituras Manuais

Hidrômetros e conferências:

```sql
INSERT INTO leituras_raw (
  sensor_id, ativo_id, variavel, valor,
  fonte, autor, modo, observacao, datetime
) VALUES (
  'manual_001', 'res_cons', 'nivel_cm', 347,
  'usuario', 'luciano', 'manual', 
  'Conferência com régua', '2025-11-16T09:30:00'
)
```

### 7.3 Calibração

```sql
INSERT INTO calibracoes (
  sensor_id, valor_referencia, valor_sensor,
  ajuste_aplicado, responsavel_usuario_id
) VALUES (
  'SEN001', 347, 345, '+2cm', 1
)
```

---

## 8. Relatório Diário 06:00

### 8.1 Geração Automática

Cron job executa às 06:00 todos os dias:

```sql
INSERT INTO relatorio_diario (
  data, 
  volume_consumido_total_l,
  volume_abastecido_total_l,
  eventos_registrados,
  anomalias_detectadas,
  resumo
) SELECT ...
```

### 8.2 Conteúdo do Relatório

1. **Resumo de Volumes**
   - Volume inicial e final de cada reservatório
   - Consumo total do dia
   - Abastecimentos realizados

2. **Eventos Significativos**
   - Abastecimentos (horário, duração, volume)
   - Consumos anormais
   - Vazamentos detectados

3. **Status de Equipamentos**
   - Horas de operação de bombas
   - Ciclos de válvulas
   - Falhas ou alertas

4. **Métricas por Período**
   - Consumo 00-06h (residual)
   - Consumo 06-12h (pico matutino)
   - Consumo 12-18h (operacional)
   - Consumo 18-24h (redução)

5. **Alertas e Ações**
   - Anomalias em investigação
   - Recomendações de manutenção

---

## 9. Padrões de Código

### 9.1 Firmware (ESP32)

```cpp
// Configuração em JSON (SPIFFS/LittleFS)
// Cálculo de volume no node
// Transmissão lean (mudança > threshold)
// Buffer local para resiliência
// MQTT QoS 1 como padrão
```

### 9.2 Backend (Node.js/Python)

```javascript
// Validação rigorosa de payloads
// Pipeline: ingestão -> validação -> processamento -> eventos
// Funções puras para cálculos
// Logging estruturado (JSON)
// Testes unitários para regras de detecção
```

### 9.3 Database (PostgreSQL/TimescaleDB)

```sql
-- Hypertables para séries temporais
-- Índices em (elemento_id, datetime)
-- Triggers para compressão automática
-- Functions PL/pgSQL para eventos
-- Particionamento por tempo (se necessário)
```

---

## 10. Segurança e Confiabilidade

### 10.1 Comunicação

- MQTT com autenticação
- TLS/SSL em produção
- MAC address como identificador único
- Validação de checksum (opcional)

### 10.2 Persistência

- Backup diário automático
- Retenção configurable (raw: 180d, processadas: 2 anos)
- Reprocessamento possível (leituras_raw preservadas)

### 10.3 Resiliência

- Buffer local nos nodes (até 100 leituras)
- Fallback HTTP se MQTT falhar
- Reconexão automática com backoff exponencial
- Watchdog timer nos firmwares

---

## 11. Visualização e Interface

### 11.1 Dashboard Principal

- 6 cards de reservatórios (%, m³, tendência)
- Status de bombas e válvulas em tempo real
- Timeline de eventos do dia
- Alertas destacados (vermelho/amarelo/verde)

### 11.2 Planta Hidráulica

- Visualização com coordenadas x,y
- Cores por status (verde=OK, amarelo=alerta, vermelho=crítico)
- Linhas de conexão mostrando fluxo
- Click nos elementos para detalhes

### 11.3 Histórico

- Gráficos de 24h, 7d, 30d
- Comparação entre reservatórios
- Curvas de consumo por período
- Exportação de relatórios (PDF/CSV)

---

## 12. Próximas Evoluções

1. **Simulador hidráulico** (previsão de comportamento)
2. **IA para detecção preditiva** (vazamentos antes de se tornarem críticos)
3. **Controle automático** (ligar bombas conforme nível)
4. **Integração com sistema elétrico** (eficiência energética)
5. **App mobile** para operadores

---

## 13. Referências

- ESP32-C3 Datasheet
- AJ-SR04M Ultrasonic Sensor Spec
- PostgreSQL TimescaleDB Documentation
- MQTT 3.1.1 Protocol Specification
- Hidráulica básica (Bernoulli, perda de carga)

---

**Versão:** 1.0  
**Data:** 2025-11-16  
**Autor:** Luciano / Sistema AGUADA  
**Licença:** Uso interno
