const relativeTimeFormatter = new Intl.RelativeTimeFormat('pt-BR', {
  numeric: 'auto',
});

const RELATIVE_TIME_DIVISIONS: Array<{
  amount: number;
  unit: Intl.RelativeTimeFormatUnit;
}> = [
  { amount: 60, unit: 'second' },
  { amount: 60, unit: 'minute' },
  { amount: 24, unit: 'hour' },
  { amount: 7, unit: 'day' },
  { amount: 4.34524, unit: 'week' },
  { amount: 12, unit: 'month' },
  { amount: Infinity, unit: 'year' },
];

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
    if (Number.isNaN(dateObj.getTime())) {
      return '-';
    }

    let durationInSeconds = (dateObj.getTime() - Date.now()) / 1000;

    for (const division of RELATIVE_TIME_DIVISIONS) {
      if (Math.abs(durationInSeconds) < division.amount) {
        return relativeTimeFormatter.format(
          Math.round(durationInSeconds),
          division.unit,
        );
      }

      durationInSeconds /= division.amount;
    }
    return '-';
  } catch {
    return '-';
  }
}

/**
 * Formata uma data como string legível
 */
export function formatDate(date: string | Date, _formatStr = 'dd/MM/yyyy HH:mm:ss'): string {
  try {
    const dateObj = typeof date === 'string' ? new Date(date) : date;
    if (Number.isNaN(dateObj.getTime())) {
      return '-';
    }
    const pad = (value: number) => value.toString().padStart(2, '0');

    const day = pad(dateObj.getDate());
    const month = pad(dateObj.getMonth() + 1);
    const year = dateObj.getFullYear();
    const hours = pad(dateObj.getHours());
    const minutes = pad(dateObj.getMinutes());
    const seconds = pad(dateObj.getSeconds());

    return `${day}/${month}/${year} ${hours}:${minutes}:${seconds}`;
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
