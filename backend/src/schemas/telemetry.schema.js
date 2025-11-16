import { z } from 'zod';

// MAC address regex: AA:BB:CC:DD:EE:FF
const macRegex = /^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$/;

// ISO8601 datetime validation
const iso8601Regex = /^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(\.\d{3})?Z?$/;

export const telemetrySchema = z.object({
  node_mac: z.string().regex(macRegex, 'MAC inválido (formato: AA:BB:CC:DD:EE:FF)'),
  
  datetime: z.string().regex(iso8601Regex, 'Datetime deve estar em ISO8601')
    .refine((val) => {
      const date = new Date(val);
      const now = new Date();
      const diff = Math.abs(now - date);
      return diff < 3600000; // Max 1h de diferença
    }, 'Datetime não pode ter mais de 1h de diferença do servidor'),
  
  data: z.array(
    z.object({
      label: z.enum(['nivel_cm', 'pressao_bar', 'vazao_lpm', 'temperatura_c']),
      value: z.number()
        .refine((val) => !isNaN(val), 'Valor deve ser numérico')
        .refine((val) => isFinite(val), 'Valor deve ser finito'),
      unit: z.string().optional(),
    })
  ).min(1, 'Deve conter ao menos 1 leitura'),
  
  meta: z.object({
    battery: z.number().min(0).max(5).optional(),
    rssi: z.number().min(-120).max(0).optional(),
    uptime: z.number().nonnegative().optional(),
    firmware_version: z.string().optional(),
  }).optional(),
});

export const manualReadingSchema = z.object({
  sensor_id: z.string().min(1, 'sensor_id obrigatório'),
  
  value: z.number()
    .refine((val) => !isNaN(val), 'Valor deve ser numérico'),
  
  variable: z.enum(['nivel_cm', 'pressao_bar', 'vazao_lpm']),
  
  datetime: z.string().regex(iso8601Regex).optional(),
  
  usuario: z.string().min(1, 'Usuário obrigatório'),
  
  observacao: z.string().max(500).optional(),
});

export const calibrationSchema = z.object({
  sensor_id: z.string().min(1, 'sensor_id obrigatório'),
  
  valor_referencia: z.number()
    .refine((val) => !isNaN(val), 'Valor de referência deve ser numérico'),
  
  valor_sensor: z.number()
    .refine((val) => !isNaN(val), 'Valor do sensor deve ser numérico'),
  
  responsavel_usuario_id: z.number().int().positive(),
  
  tipo: z.enum(['manual', 'automatica']).default('manual'),
  
  observacao: z.string().max(500).optional(),
});

export function validateTelemetry(data) {
  return telemetrySchema.safeParse(data);
}

export function validateManualReading(data) {
  return manualReadingSchema.safeParse(data);
}

export function validateCalibration(data) {
  return calibrationSchema.safeParse(data);
}
