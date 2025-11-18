// Tipos para telemetria e leituras de sensores
export interface TelemetryReading {
  sensor_id: string;
  elemento_id: string;
  variavel: 'distance_cm' | 'valve_in' | 'valve_out' | 'sound_in';
  valor: number;
  unidade: string;
  datetime: string;
  meta?: {
    battery_mv?: number;
    rssi_dbm?: number;
    uptime_sec?: number;
    node_mac?: string;
    raw_value?: number;
  };
}

export interface LatestReadings {
  [sensor_id: string]: TelemetryReading;
}

export interface ReadingHistory {
  readings: Array<{
    datetime: string;
    valor: number;
    unidade: string;
  }>;
  total: number;
}

export interface SensorStatus {
  sensor_id: string;
  elemento_id: string;
  node_mac: string;
  status: 'online' | 'offline';
  last_reading: string;
  signal_strength: number;
}

export interface DailySummary {
  reservoirs: Array<{
    elemento_id: string;
    min_nivel: number;
    max_nivel: number;
    avg_nivel: number;
    volume_consumido: number;
  }>;
}
