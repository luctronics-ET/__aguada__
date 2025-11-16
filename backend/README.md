# AGUADA Backend API

Backend Node.js para o sistema AGUADA de monitoramento hidráulico.

## 🚀 Tecnologias

- **Node.js 18+** com ES Modules
- **Express** - API REST
- **PostgreSQL 15** com **TimescaleDB** - Banco de dados time-series
- **Redis** - Fila de processamento assíncrono
- **Zod** - Validação de schemas
- **Winston** - Logging estruturado
- **MQTT** - Comunicação com nodes ESP32

## 📦 Instalação

```bash
# Instalar dependências
npm install

# Copiar arquivo de ambiente
cp .env.example .env

# Editar variáveis de ambiente
nano .env
```

## ⚙️ Configuração

Edite o arquivo `.env` com suas credenciais:

```env
DB_HOST=localhost
DB_PORT=5432
DB_NAME=aguada
DB_USER=aguada_user
DB_PASSWORD=sua_senha

REDIS_HOST=localhost
REDIS_PORT=6379

PORT=3000
```

## 🗄️ Banco de Dados

```bash
# Criar banco e aplicar schema
psql -U postgres -c "CREATE DATABASE aguada;"
psql -U aguada_user -d aguada -f ../database/schema.sql
```

## ▶️ Executar

```bash
# Desenvolvimento (com auto-reload)
npm run dev

# Produção
npm start

# Testes
npm test
```

## 📡 Endpoints

### POST /api/telemetry
Recebe telemetria dos nodes ESP32.

**Request:**
```json
{
  "node_mac": "AA:BB:CC:DD:EE:01",
  "datetime": "2025-11-16T14:30:00Z",
  "data": [
    {
      "label": "nivel_cm",
      "value": 245.5,
      "unit": "cm"
    }
  ],
  "meta": {
    "battery": 3.8,
    "rssi": -65,
    "uptime": 3600
  }
}
```

**Response:**
```json
{
  "success": true,
  "message": "Telemetria recebida com sucesso",
  "processed": 1
}
```

### POST /api/manual-reading
Registra leitura manual.

**Request:**
```json
{
  "sensor_id": "SEN_CON_01",
  "value": 250.0,
  "variable": "nivel_cm",
  "usuario": "operador@aguada.local",
  "observacao": "Leitura com régua graduada"
}
```

### POST /api/calibration
Registra calibração de sensor.

**Request:**
```json
{
  "sensor_id": "SEN_CON_01",
  "valor_referencia": 250.0,
  "valor_sensor": 248.5,
  "responsavel_usuario_id": 1,
  "tipo": "manual",
  "observacao": "Calibração com régua"
}
```

**Response:**
```json
{
  "success": true,
  "calibracao_id": 42,
  "ajuste_aplicado": 1.5
}
```

## 🔄 Processamento de Dados

### 1. Validação
- Schema validation com **Zod**
- Verificação de MAC address
- Range de valores físicos

### 2. Mapeamento
- Identifica sensor pelo MAC
- Associa ao elemento hidráulico

### 3. Cálculo de Volume
- Fórmulas geométricas (cilindro/retângulo)
- Aplicação de offset de calibração

### 4. Compressão (Deadband)
- Deadband: 2cm
- Window size: 11 amostras
- Redução: >90% de dados

### 5. Detecção de Eventos
- **ABASTECIMENTO**: ΔV > +50L
- **VAZAMENTO**: Taxa < -15L/h por >1h
- **NIVEL_CRITICO_CAV**: <70% por >10min

## 📊 Estrutura de Diretórios

```
backend/
├── src/
│   ├── config/          # Configurações (DB, Redis, Logger)
│   ├── controllers/     # Controladores de rotas
│   ├── routes/          # Definição de rotas
│   ├── schemas/         # Validação Zod
│   ├── services/        # Lógica de negócio
│   │   ├── sensor.service.js
│   │   ├── reading.service.js
│   │   ├── volume.service.js
│   │   ├── compression.service.js
│   │   └── event.service.js
│   └── server.js        # Entry point
├── logs/                # Arquivos de log
├── package.json
└── .env.example
```

## 🧪 Testes

```bash
# Rodar todos os testes
npm test

# Modo watch
npm run test:watch

# Coverage
npm test -- --coverage
```

## 📝 Logs

Logs são salvos em:
- **Console**: Saída colorida para desenvolvimento
- **Arquivo**: `logs/aguada.log` (rotação automática)

Formato:
```
2025-11-16 14:30:00 [info]: Telemetria recebida {"node_mac":"AA:BB:CC:DD:EE:01","readings":1}
```

## 🔒 Segurança

- **Helmet**: Headers de segurança
- **Rate Limiting**: 60 req/min por IP
- **CORS**: Configurável
- **Input Validation**: Zod schemas
- **SQL Injection**: Prepared statements

## 🚀 Performance

- **Target**: <100ms p95 latency
- **Throughput**: 100 leituras/segundo
- **Connection Pool**: 20 conexões PostgreSQL
- **Redis Queue**: Processamento assíncrono

## 📚 Referências

- [RULES.md](../RULES.md) - Regras do sistema
- [SETUP.md](../SETUP.md) - Guia de instalação
- [database/schema.sql](../database/schema.sql) - Schema do banco
