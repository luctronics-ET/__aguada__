import api from './api';
import type { LatestReadings, ReadingHistory, SensorStatus, DailySummary } from '../types/telemetry.types';
import type { HealthResponse } from '../types/api.types';

export const telemetryService = {
  /**
   * Verifica a saúde da API
   */
  async checkHealth(): Promise<HealthResponse> {
    const response = await api.get<HealthResponse>('/health');
    return response.data;
  },

  /**
   * Obtém as últimas leituras de todos os sensores
   */
  async getLatestReadings(): Promise<LatestReadings> {
    const response = await api.get<LatestReadings>('/readings/latest');
    return response.data;
  },

  /**
   * Obtém o histórico de leituras de um sensor específico
   */
  async getReadingHistory(
    sensorId: string,
    start: Date,
    end: Date
  ): Promise<ReadingHistory> {
    const response = await api.get<ReadingHistory>(`/readings/history/${sensorId}`, {
      params: {
        start: start.toISOString(),
        end: end.toISOString(),
      },
    });
    return response.data;
  },

  /**
   * Obtém o status de todos os sensores
   */
  async getSensorsStatus(): Promise<{ sensors: SensorStatus[] }> {
    const response = await api.get<{ sensors: SensorStatus[] }>('/sensors/status');
    return response.data;
  },

  /**
   * Obtém o resumo diário
   */
  async getDailySummary(): Promise<DailySummary> {
    const response = await api.get<DailySummary>('/readings/daily-summary');
    return response.data;
  },
};
