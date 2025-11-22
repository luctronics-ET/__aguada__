import { useQuery } from '@tanstack/react-query';
import * as api from '../services/api.service';

// Hook para obter últimas leituras
export function useLatestReadings() {
  return useQuery({
    queryKey: ['readings', 'latest'],
    queryFn: api.getLatestReadings,
    refetchInterval: 5000, // Atualiza a cada 5 segundos
  });
}

// Hook para obter leituras raw
export function useRawReadings(params?: {
  sensor_id?: string;
  elemento_id?: string;
  limit?: number;
}) {
  return useQuery({
    queryKey: ['readings', 'raw', params],
    queryFn: () => api.getRawReadings(params),
    refetchInterval: 10000, // Atualiza a cada 10 segundos
  });
}

// Hook para histórico de sensor
export function useReadingHistory(sensor_id: string, params?: {
  start_date?: string;
  end_date?: string;
  limit?: number;
}) {
  return useQuery({
    queryKey: ['readings', 'history', sensor_id, params],
    queryFn: () => api.getReadingHistory(sensor_id, params),
    enabled: !!sensor_id,
  });
}

// Hook para todos os sensores
export function useSensors() {
  return useQuery({
    queryKey: ['sensors'],
    queryFn: api.getAllSensors,
  });
}

// Hook para status dos sensores
export function useSensorsStatus() {
  return useQuery({
    queryKey: ['sensors', 'status'],
    queryFn: api.getSensorsStatus,
    refetchInterval: 30000, // Atualiza a cada 30 segundos
  });
}

// Hook para alertas
export function useAlerts(params?: {
  status?: string;
  nivel?: string;
  limit?: number;
}) {
  return useQuery({
    queryKey: ['alerts', params],
    queryFn: () => api.getAlerts(params),
    refetchInterval: 15000, // Atualiza a cada 15 segundos
  });
}

// Hook para resumo de alertas
export function useAlertsSummary() {
  return useQuery({
    queryKey: ['alerts', 'summary'],
    queryFn: api.getAlertsSummary,
    refetchInterval: 15000,
  });
}

// Hook para estatísticas diárias
export function useDailyStats(params?: {
  date?: string;
  elemento_id?: string;
}) {
  return useQuery({
    queryKey: ['stats', 'daily', params],
    queryFn: () => api.getDailyStats(params),
  });
}

// Hook para estatísticas de consumo
export function useConsumptionStats(params?: {
  periodo?: 'day' | 'week' | 'month' | 'year';
  elemento_id?: string;
}) {
  return useQuery({
    queryKey: ['stats', 'consumption', params],
    queryFn: () => api.getConsumptionStats(params),
  });
}

// Hook para estatísticas de sensores
export function useSensorsStats() {
  return useQuery({
    queryKey: ['stats', 'sensors'],
    queryFn: api.getSensorsStats,
  });
}

// Hook para health do sistema
export function useSystemHealth() {
  return useQuery({
    queryKey: ['system', 'health'],
    queryFn: api.getSystemHealth,
    refetchInterval: 30000,
  });
}

// Hook para métricas do sistema
export function useSystemMetrics() {
  return useQuery({
    queryKey: ['system', 'metrics'],
    queryFn: api.getSystemMetrics,
    refetchInterval: 10000,
  });
}

// Hook para logs do sistema
export function useSystemLogs(params?: {
  level?: string;
  limit?: number;
}) {
  return useQuery({
    queryKey: ['system', 'logs', params],
    queryFn: () => api.getSystemLogs(params),
    refetchInterval: 5000,
  });
}

// Hook para métricas do gateway
export function useGatewayMetrics() {
  return useQuery({
    queryKey: ['gateway', 'metrics'],
    queryFn: api.getGatewayMetrics,
    refetchInterval: 10000,
  });
}
