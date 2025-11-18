import { useQuery } from '@tanstack/react-query';
import { telemetryService } from '../services/telemetry.service';

/**
 * Hook para obter as últimas leituras dos sensores
 * Atualiza automaticamente a cada 10 segundos
 */
export function useReservoirData() {
  return useQuery({
    queryKey: ['latest-readings'],
    queryFn: () => telemetryService.getLatestReadings(),
    refetchInterval: 10000, // Poll a cada 10 segundos
    staleTime: 5000, // Considera os dados válidos por 5 segundos
    retry: 3, // Tenta 3 vezes em caso de erro
    retryDelay: 1000, // Aguarda 1 segundo entre tentativas
  });
}

/**
 * Hook para obter o histórico de leituras de um sensor
 */
export function useReadingHistory(
  sensorId: string,
  start: Date,
  end: Date,
  enabled = true
) {
  return useQuery({
    queryKey: ['reading-history', sensorId, start.toISOString(), end.toISOString()],
    queryFn: () => telemetryService.getReadingHistory(sensorId, start, end),
    enabled: enabled && !!sensorId,
    staleTime: 60000, // Histórico válido por 1 minuto
  });
}

/**
 * Hook para obter o status dos sensores
 */
export function useSensorsStatus() {
  return useQuery({
    queryKey: ['sensors-status'],
    queryFn: () => telemetryService.getSensorsStatus(),
    refetchInterval: 30000, // Atualiza a cada 30 segundos
    staleTime: 15000,
  });
}

/**
 * Hook para verificar a saúde da API
 */
export function useApiHealth() {
  return useQuery({
    queryKey: ['api-health'],
    queryFn: () => telemetryService.checkHealth(),
    refetchInterval: 60000, // Verifica a cada 1 minuto
    retry: 1,
  });
}
