import axios from 'axios';

// URL do backend (pode ser configurada via variável de ambiente)
const API_BASE_URL = import.meta.env.VITE_API_URL || 'http://192.168.0.100:3000/api';

// Criar instância do axios
const api = axios.create({
  baseURL: API_BASE_URL,
  timeout: 10000,
  headers: {
    'Content-Type': 'application/json',
  },
});

// Interceptor para requests (adicionar token se necessário)
api.interceptors.request.use(
  (config) => {
    // Futuramente adicionar token JWT aqui
    // const token = localStorage.getItem('token');
    // if (token) {
    //   config.headers.Authorization = `Bearer ${token}`;
    // }
    return config;
  },
  (error) => {
    return Promise.reject(error);
  }
);

// Interceptor para responses (tratamento de erros)
api.interceptors.response.use(
  (response) => response,
  (error) => {
    if (error.response) {
      // Erro com resposta do servidor
      console.error('API Error:', error.response.status, error.response.data);
    } else if (error.request) {
      // Erro sem resposta (timeout, rede)
      console.error('Network Error:', error.message);
    } else {
      console.error('Error:', error.message);
    }
    return Promise.reject(error);
  }
);

// ============================================================================
// READINGS
// ============================================================================

export const getLatestReadings = async () => {
  const response = await api.get('/readings/latest');
  return response.data;
};

export const getRawReadings = async (params?: {
  sensor_id?: string;
  elemento_id?: string;
  limit?: number;
  offset?: number;
}) => {
  const response = await api.get('/readings/raw', { params });
  return response.data;
};

export const getReadingHistory = async (sensor_id: string, params?: {
  start_date?: string;
  end_date?: string;
  limit?: number;
}) => {
  const response = await api.get(`/readings/history/${sensor_id}`, { params });
  return response.data;
};

export const getDailySummary = async (params?: {
  elemento_id?: string;
  date?: string;
}) => {
  const response = await api.get('/readings/daily-summary', { params });
  return response.data;
};

// ============================================================================
// SENSORS
// ============================================================================

export const getAllSensors = async () => {
  const response = await api.get('/sensors');
  return response.data;
};

export const getSensorsStatus = async () => {
  const response = await api.get('/sensors/status');
  return response.data;
};

export const getSensorById = async (sensor_id: string) => {
  const response = await api.get(`/sensors/${sensor_id}`);
  return response.data;
};

// ============================================================================
// ALERTS
// ============================================================================

export const getAlerts = async (params?: {
  status?: string;
  nivel?: string;
  limit?: number;
}) => {
  const response = await api.get('/alerts', { params });
  return response.data;
};

export const getAlertsSummary = async () => {
  const response = await api.get('/alerts/summary');
  return response.data;
};

export const resolveAlert = async (alert_id: number, data: any) => {
  const response = await api.put(`/alerts/${alert_id}/resolve`, data);
  return response.data;
};

// ============================================================================
// STATISTICS
// ============================================================================

export const getDailyStats = async (params?: {
  date?: string;
  elemento_id?: string;
}) => {
  const response = await api.get('/stats/daily', { params });
  return response.data;
};

export const getConsumptionStats = async (params?: {
  periodo?: 'day' | 'week' | 'month' | 'year';
  elemento_id?: string;
}) => {
  const response = await api.get('/stats/consumption', { params });
  return response.data;
};

export const getSensorsStats = async () => {
  const response = await api.get('/stats/sensors');
  return response.data;
};

export const getEventsStats = async (params?: {
  start_date?: string;
  end_date?: string;
}) => {
  const response = await api.get('/stats/events', { params });
  return response.data;
};

// ============================================================================
// SYSTEM
// ============================================================================

export const getSystemHealth = async () => {
  const response = await api.get('/system/health');
  return response.data;
};

export const getSystemLogs = async (params?: {
  level?: string;
  limit?: number;
}) => {
  const response = await api.get('/system/logs', { params });
  return response.data;
};

export const getSystemMetrics = async () => {
  const response = await api.get('/system/metrics');
  return response.data;
};

export const getSystemAlerts = async () => {
  const response = await api.get('/system/alerts');
  return response.data;
};

// ============================================================================
// GATEWAY
// ============================================================================

export const getGatewayMetrics = async () => {
  const response = await api.get('/gateway/metrics');
  return response.data;
};

// ============================================================================
// HEALTH CHECK
// ============================================================================

export const healthCheck = async () => {
  const response = await api.get('/health');
  return response.data;
};

export default api;
