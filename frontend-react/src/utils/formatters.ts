import { format, formatDistanceToNow } from 'date-fns';
import { ptBR } from 'date-fns/locale';

/**
 * Formata um número como volume em litros
 */
export function formatLiters(liters: number): string {
  if (liters >= 1000) {
    return `${(liters / 1000).toFixed(1)} m³`;
  }
  return `${liters.toFixed(0)} L`;
}

/**
 * Formata um número como porcentagem
 */
export function formatPercent(value: number): string {
  return `${value.toFixed(0)}%`;
}

/**
 * Formata uma data como timestamp relativo (ex: "há 2 minutos")
 */
export function formatRelativeTime(date: string | Date): string {
  try {
    const dateObj = typeof date === 'string' ? new Date(date) : date;
    return formatDistanceToNow(dateObj, { addSuffix: true, locale: ptBR });
  } catch {
    return '-';
  }
}

/**
 * Formata uma data como string legível
 */
export function formatDate(date: string | Date, formatStr = 'dd/MM/yyyy HH:mm:ss'): string {
  try {
    const dateObj = typeof date === 'string' ? new Date(date) : date;
    return format(dateObj, formatStr, { locale: ptBR });
  } catch {
    return '-';
  }
}

/**
 * Formata RSSI em dBm com indicador visual
 */
export function formatRSSI(rssi: number): string {
  const icon = rssi > -50 ? '📶' : rssi > -70 ? '📡' : '📉';
  return `${icon} ${rssi} dBm`;
}

/**
 * Formata bateria em volts
 */
export function formatBattery(millivolts: number): string {
  const volts = millivolts / 1000;
  const icon = volts >= 4.5 ? '🔋' : volts >= 4.0 ? '🪫' : '⚡';
  return `${icon} ${volts.toFixed(1)}V`;
}
