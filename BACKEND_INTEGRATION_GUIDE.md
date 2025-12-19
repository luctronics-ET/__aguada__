# Backend Integration Guide - Firmware v1.0.0 (Litros)

## 📋 Mudanças Necessárias no Backend

### 1. Schema de Validação (Zod)

**Arquivo:** `backend/src/schemas/telemetry.schema.js`

#### ❌ Antes:

```javascript
const telemetrySchema = z.object({
  mac: z.string().regex(/^[0-9A-F]{2}(:[0-9A-F]{2}){5}$/),
  type: z.enum(["distance_cm", "valve_in", "valve_out", "sound_in"]),
  value: z.number().int(),
  battery: z.number().int().min(0),
  uptime: z.number().int().min(0),
  rssi: z.number().int().max(0),
  // Sem campo alias
});
```

#### ✅ Depois:

```javascript
const telemetrySchema = z.object({
  mac: z.string().regex(/^[0-9A-F]{2}(:[0-9A-F]{2}){5}$/),
  type: z.enum(["distance_cm", "valve_in", "valve_out", "sound_in"]),
  value: z.number().int(),
  battery: z.number().int().min(0),
  uptime: z.number().int().min(0),
  rssi: z.number().int().max(0),
  alias: z.string().max(16).optional(), // ✅ Novo
});
```

### 2. Controlador de Telemetria

**Arquivo:** `backend/src/controllers/telemetry.controller.js`

#### ❌ Antes (mL):

```javascript
export const receiveTelemetry = async (req, res) => {
  const { mac, type, value, battery, uptime, rssi } = req.body;

  if (type === "distance_cm") {
    // Valor vinha em cm × 100 (ex: 24480 = 244.80 cm)
    const distanceCm = value / 100;

    // Volume calculado em mL
    const volumeMl = calculateVolume(distanceCm, 245000000);

    // Armazenar em mL
    await db.query(
      "INSERT INTO aguada.leituras_raw (sensor_id, valor, unidade) VALUES ($1, $2, $3)",
      ["RCON", volumeMl, "mL"]
    );
  }
};
```

#### ✅ Depois (L):

```javascript
export const receiveTelemetry = async (req, res) => {
  const { mac, type, value, battery, uptime, rssi, alias } = req.body;

  if (type === "distance_cm") {
    // Valor vem em cm × 100 (ex: 24480 = 244.80 cm)
    const distanceCm = value / 100;

    // Volume já é retornado em L (não em mL!)
    const volumeL = calculateVolumeInLiters(distanceCm, 245000); // 245.000 L max

    // Percentual inteiro (0-100)
    const percentInteger = Math.round((volumeL / 245000) * 100);

    // Armazenar em L
    await db.query(
      "INSERT INTO aguada.leituras_raw (sensor_id, valor, unidade, alias) VALUES ($1, $2, $3, $4)",
      ["RCON", volumeL, "L", alias || "UNKNOWN"]
    );
  }
};

// Helper atualizado
function calculateVolumeInLiters(distanceCm, maxCapacityL) {
  const levelCm = 400 - distanceCm; // Ou 450, depende do reservatório
  const percent = Math.max(0, Math.min(100, (levelCm / 400) * 100));
  return Math.round((percent / 100) * maxCapacityL);
}
```

### 3. Função Helper de Cálculo

**Arquivo:** `backend/src/utils/volume.utils.js` (novo)

```javascript
/**
 * Calcula volume em litros a partir da altura em cm
 * @param {number} heightCm - Altura medida do fundo (em cm)
 * @param {number} maxHeightCm - Altura máxima do reservatório (ex: 400cm)
 * @param {number} maxVolumeL - Capacidade máxima em litros
 * @returns {number} Volume em litros (inteiro)
 */
export function calculateVolumeInLiters(heightCm, maxHeightCm, maxVolumeL) {
  if (heightCm < 0 || heightCm > maxHeightCm) {
    return 0;
  }

  const percent = (heightCm / maxHeightCm) * 100;
  const volumeL = (percent / 100) * maxVolumeL;

  // Retornar como inteiro (sem decimais)
  return Math.round(volumeL);
}

/**
 * Calcula percentual inteiro
 * @param {number} volumeL - Volume em litros
 * @param {number} maxVolumeL - Capacidade máxima em litros
 * @returns {number} Percentual (0-100, inteiro)
 */
export function calculatePercentage(volumeL, maxVolumeL) {
  const percent = (volumeL / maxVolumeL) * 100;
  return Math.round(Math.max(0, Math.min(100, percent)));
}
```

### 4. Modelo de Banco de Dados

**Arquivo:** `database/schema.sql`

#### Alterar tabela `leituras_raw`:

```sql
-- ❌ Remover
ALTER TABLE aguada.leituras_raw DROP COLUMN IF EXISTS valor_ml;
ALTER TABLE aguada.leituras_raw DROP COLUMN IF EXISTS valor_max_ml;

-- ✅ Adicionar (se não existirem)
ALTER TABLE aguada.leituras_raw
ADD COLUMN IF NOT EXISTS valor_l INTEGER,  -- Volume em litros
ADD COLUMN IF NOT EXISTS valor_max_l INTEGER,  -- Capacidade em litros
ADD COLUMN IF NOT EXISTS percentual_inteiro SMALLINT,  -- 0-100
ADD COLUMN IF NOT EXISTS alias VARCHAR(16);  -- Identificador do dispositivo

-- ✅ Índices para melhor performance
CREATE INDEX IF NOT EXISTS idx_leituras_alias
ON aguada.leituras_raw(alias);

CREATE INDEX IF NOT EXISTS idx_leituras_valor_l
ON aguada.leituras_raw(valor_l)
WHERE valor_l IS NOT NULL;
```

#### Exemplos de Queries:

```sql
-- Última leitura de cada sensor (em litros)
SELECT
  sensor_id,
  alias,
  valor_l,
  valor_max_l,
  ROUND((valor_l::FLOAT / valor_max_l::FLOAT) * 100) as percent,
  datetime
FROM aguada.leituras_raw
WHERE datetime = (
  SELECT MAX(datetime) FROM aguada.leituras_raw lr2
  WHERE lr2.sensor_id = aguada.leituras_raw.sensor_id
)
ORDER BY sensor_id;

-- Volume acumulado em um dia (em litros)
SELECT
  alias,
  DATE(datetime),
  MAX(valor_l) as volume_maximo_l,
  MIN(valor_l) as volume_minimo_l,
  (MAX(valor_l) - MIN(valor_l)) as consumo_l
FROM aguada.leituras_raw
WHERE DATE(datetime) = CURRENT_DATE
GROUP BY alias, DATE(datetime);

-- Sensores offline (sem dados há mais de 5 minutos)
SELECT
  alias,
  MAX(datetime) as ultimo_dado,
  NOW() - MAX(datetime) as tempo_sem_dados
FROM aguada.leituras_raw
WHERE datetime > NOW() - INTERVAL '1 hour'
GROUP BY alias
HAVING (NOW() - MAX(datetime)) > INTERVAL '5 minutes'
ORDER BY tempo_sem_dados DESC;
```

### 5. Endpoint de API

**Arquivo:** `backend/src/routes/api.routes.js`

#### Exemplo de resposta atualizada:

```javascript
// GET /api/readings/latest
router.get("/readings/latest", async (req, res) => {
  const result = await db.query(`
    SELECT DISTINCT ON (sensor_id)
      sensor_id,
      alias,
      valor_l,
      valor_max_l,
      ROUND((valor_l::FLOAT / valor_max_l::FLOAT) * 100)::INT as percentual,
      datetime,
      battery,
      uptime,
      rssi
    FROM aguada.leituras_raw
    WHERE tipo = 'distance_cm'
    ORDER BY sensor_id, datetime DESC
  `);

  return res.json({
    success: true,
    data: result.rows.map((row) => ({
      sensor_id: row.sensor_id,
      alias: row.alias,
      volume_L: row.valor_l, // ✅ Em litros
      max_capacity_L: row.valor_max_l,
      percentage: row.percentual, // ✅ Inteiro (0-100)
      battery_mV: row.battery,
      uptime_sec: row.uptime,
      signal_dBm: row.rssi,
      timestamp: row.datetime,
    })),
  });
});
```

### 6. Resposta JSON Esperada do Firmware

```json
{
  "mac": "20:6E:F1:6B:77:58",
  "type": "distance_cm",
  "value": 24480,
  "battery": 5000,
  "uptime": 3600,
  "rssi": -50,
  "alias": "RB03"
}
```

**Interpretação:**

- `value: 24480` = 244.80 cm de altura
- Com reservatório de 245.000L e 400cm de altura
- Volume = (244.80 / 400) × 245.000 = **150.000L**
- Percentual = (150.000 / 245.000) × 100 = **61%** (inteiro)

### 7. Testes de API

```bash
# Enviar telemetria de teste
curl -X POST http://localhost:3000/api/telemetry \
  -H "Content-Type: application/json" \
  -d '{
    "mac": "20:6E:F1:6B:77:58",
    "type": "distance_cm",
    "value": 24480,
    "battery": 5000,
    "uptime": 3600,
    "rssi": -50,
    "alias": "RB03"
  }'

# Obter últimas leituras
curl http://localhost:3000/api/readings/latest | jq '.data[] | {alias, volume_L, percentage}'

# Resposta esperada:
# {
#   "alias": "RB03",
#   "volume_L": 150000,
#   "percentage": 61
# }
```

---

## 📊 Cheat Sheet - Conversão de Tipos

| Campo          | Tipo Antigo     | Tipo Novo   | Exemplo               |
| -------------- | --------------- | ----------- | --------------------- |
| Volume         | mL (uint16)     | L (uint32)  | 245.000.000 → 245.000 |
| Percentual     | float (0-100.5) | int (0-100) | 100.5 → 100           |
| Max Capacity   | mL              | L           | 245.000.000 → 245.000 |
| Alias          | ❌ Não tinha    | string(16)  | "RB03"                |
| Distance × 100 | ✅ Mantém       | ✅ Mantém   | 24480 = 244.80cm      |

---

## ⚠️ Cuidados na Migração

1. **Backup de dados antigos (mL)**

   ```bash
   pg_dump -h localhost -U aguada aguada > backup_ml.sql
   ```

2. **Limpeza de NVS no ESP32**

   - Sem `erase-flash`, valores antigos (em mL) permanecerão
   - Novo firmware tentará ler 245.000.000 como se fosse 245.000 = ❌ 1000x errado

3. **Atualizar documentação**
   - Mudar exemplos de JSON em README
   - Atualizar postman collection
   - Avisar time de operações sobre unidade nova

---

## ✅ Checklist de Implementação

- [ ] Schema Zod atualizado
- [ ] Controlador de telemetria atualizado
- [ ] Utils de volume criado
- [ ] Schema SQL migrado
- [ ] Testes de API confirmados
- [ ] Frontend mostra volumes em L (não mL)
- [ ] Documentação atualizada
- [ ] Dados históricos em mL documentados (para auditoria)

---

_Guia preparado para: firmware/node_ultra_12dez25/_
_Data: 12 de dezembro de 2025_
_Status: Aguardando compilação do firmware_
