import logger from '../config/logger.js';
import queueService from './queue.service.js';
import { getClientsCount } from '../websocket/wsHandler.js';

/**
 * Serviço de métricas para monitoramento do sistema
 */
class MetricsService {
  constructor() {
    // Métricas de telemetria
    this.telemetry = {
      totalReceived: 0,
      totalProcessed: 0,
      totalFailed: 0,
      lastReceived: null,
      lastProcessed: null,
    };

    // Métricas de latência (em ms)
    this.latency = {
      samples: [], // Array circular de últimas 1000 amostras
      maxSamples: 1000,
    };

    // Métricas por endpoint
    this.endpoints = new Map();

    // Métricas de erro
    this.errors = {
      total: 0,
      byType: new Map(),
      lastError: null,
    };

    // Iniciar coleta periódica
    this.startCollection();
  }

  /**
   * Registra recebimento de telemetria
   */
  recordTelemetryReceived() {
    this.telemetry.totalReceived++;
    this.telemetry.lastReceived = new Date();
  }

  /**
   * Registra processamento de telemetria
   */
  recordTelemetryProcessed() {
    this.telemetry.totalProcessed++;
    this.telemetry.lastProcessed = new Date();
  }

  /**
   * Registra falha no processamento
   */
  recordTelemetryFailed() {
    this.telemetry.totalFailed++;
  }

  /**
   * Registra latência de uma requisição
   */
  recordLatency(ms) {
    this.latency.samples.push(ms);
    if (this.latency.samples.length > this.latency.maxSamples) {
      this.latency.samples.shift(); // Remove mais antigo
    }
  }

  /**
   * Calcula percentis de latência
   */
  calculatePercentiles() {
    if (this.latency.samples.length === 0) {
      return { p50: 0, p95: 0, p99: 0, min: 0, max: 0, avg: 0 };
    }

    const sorted = [...this.latency.samples].sort((a, b) => a - b);
    const len = sorted.length;

    const percentile = (p) => {
      const index = Math.ceil((p / 100) * len) - 1;
      return sorted[Math.max(0, index)];
    };

    const sum = sorted.reduce((a, b) => a + b, 0);

    return {
      p50: percentile(50),
      p95: percentile(95),
      p99: percentile(99),
      min: sorted[0],
      max: sorted[len - 1],
      avg: Math.round(sum / len),
      count: len,
    };
  }

  /**
   * Registra requisição de endpoint
   */
  recordEndpointRequest(method, path, statusCode, latency) {
    const key = `${method} ${path}`;
    
    if (!this.endpoints.has(key)) {
      this.endpoints.set(key, {
        count: 0,
        errors: 0,
        latencies: [],
      });
    }

    const endpoint = this.endpoints.get(key);
    endpoint.count++;
    
    if (statusCode >= 400) {
      endpoint.errors++;
    }

    if (latency) {
      endpoint.latencies.push(latency);
      if (endpoint.latencies.length > 100) {
        endpoint.latencies.shift();
      }
    }
  }

  /**
   * Registra erro
   */
  recordError(type, message) {
    this.errors.total++;
    
    if (!this.errors.byType.has(type)) {
      this.errors.byType.set(type, 0);
    }
    this.errors.byType.set(type, this.errors.byType.get(type) + 1);
    
    this.errors.lastError = {
      type,
      message,
      timestamp: new Date(),
    };
  }

  /**
   * Obtém estatísticas de endpoint
   */
  getEndpointStats() {
    const stats = [];
    
    this.endpoints.forEach((data, key) => {
      const avgLatency = data.latencies.length > 0
        ? Math.round(data.latencies.reduce((a, b) => a + b, 0) / data.latencies.length)
        : 0;
      
      const errorRate = data.count > 0
        ? ((data.errors / data.count) * 100).toFixed(2)
        : 0;

      stats.push({
        endpoint: key,
        requests: data.count,
        errors: data.errors,
        errorRate: parseFloat(errorRate),
        avgLatency,
      });
    });

    return stats.sort((a, b) => b.requests - a.requests);
  }

  /**
   * Obtém métricas completas do sistema
   */
  async getMetrics() {
    const queueStats = await queueService.getQueueStats();
    const latencyPercentiles = this.calculatePercentiles();
    const endpointStats = this.getEndpointStats();
    const wsClients = getClientsCount();

    // Calcular taxa de erro geral
    const totalRequests = Array.from(this.endpoints.values())
      .reduce((sum, ep) => sum + ep.count, 0);
    const totalErrors = Array.from(this.endpoints.values())
      .reduce((sum, ep) => sum + ep.errors, 0);
    const errorRate = totalRequests > 0
      ? ((totalErrors / totalRequests) * 100).toFixed(2)
      : 0;

    // Calcular throughput (leituras por segundo nos últimos 60s)
    const now = Date.now();
    const recentSamples = this.latency.samples.filter(
      (_, idx) => now - (this.latency.samples.length - idx) * 100 < 60000
    );
    const throughput = recentSamples.length / 60; // Aproximação

    return {
      timestamp: new Date().toISOString(),
      telemetry: {
        ...this.telemetry,
        successRate: this.telemetry.totalReceived > 0
          ? ((this.telemetry.totalProcessed / this.telemetry.totalReceived) * 100).toFixed(2)
          : 0,
      },
      latency: latencyPercentiles,
      throughput: {
        readingsPerSecond: throughput.toFixed(2),
        totalReadings: this.telemetry.totalReceived,
      },
      endpoints: {
        total: this.endpoints.size,
        stats: endpointStats,
        errorRate: parseFloat(errorRate),
      },
      errors: {
        total: this.errors.total,
        byType: Object.fromEntries(this.errors.byType),
        lastError: this.errors.lastError,
      },
      queue: queueStats,
      websocket: {
        connectedClients: wsClients,
      },
      uptime: {
        seconds: Math.floor(process.uptime()),
        formatted: this.formatUptime(process.uptime()),
      },
    };
  }

  /**
   * Formata uptime em formato legível
   */
  formatUptime(seconds) {
    const days = Math.floor(seconds / 86400);
    const hours = Math.floor((seconds % 86400) / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = Math.floor(seconds % 60);

    if (days > 0) {
      return `${days}d ${hours}h ${minutes}m`;
    } else if (hours > 0) {
      return `${hours}h ${minutes}m ${secs}s`;
    } else if (minutes > 0) {
      return `${minutes}m ${secs}s`;
    } else {
      return `${secs}s`;
    }
  }

  /**
   * Verifica condições de alerta
   */
  async checkAlerts() {
    const metrics = await this.getMetrics();
    const alerts = [];

    // Alerta: Taxa de erro > 1%
    if (metrics.endpoints.errorRate > 1) {
      alerts.push({
        level: 'warning',
        type: 'high_error_rate',
        message: `Taxa de erro alta: ${metrics.endpoints.errorRate}%`,
        value: metrics.endpoints.errorRate,
        threshold: 1,
      });
    }

    // Alerta: Latência p95 > 500ms
    if (metrics.latency.p95 > 500) {
      alerts.push({
        level: 'warning',
        type: 'high_latency',
        message: `Latência p95 alta: ${metrics.latency.p95}ms`,
        value: metrics.latency.p95,
        threshold: 500,
      });
    }

    // Alerta: Queue > 80% cheia
    if (metrics.queue && metrics.queue.waiting + metrics.queue.active > 40) {
      alerts.push({
        level: 'warning',
        type: 'queue_full',
        message: `Queue quase cheia: ${metrics.queue.waiting + metrics.queue.active} jobs`,
        value: metrics.queue.waiting + metrics.queue.active,
        threshold: 40,
      });
    }

    // Alerta: Muitas falhas de telemetria
    const failureRate = metrics.telemetry.totalReceived > 0
      ? ((metrics.telemetry.totalFailed / metrics.telemetry.totalReceived) * 100)
      : 0;
    if (failureRate > 5) {
      alerts.push({
        level: 'error',
        type: 'high_failure_rate',
        message: `Taxa de falha alta: ${failureRate.toFixed(2)}%`,
        value: failureRate,
        threshold: 5,
      });
    }

    // Alerta: Sem leituras recentes (> 5 minutos)
    if (metrics.telemetry.lastReceived) {
      const minutesSinceLastReading = (Date.now() - new Date(metrics.telemetry.lastReceived).getTime()) / 60000;
      if (minutesSinceLastReading > 5) {
        alerts.push({
          level: 'error',
          type: 'no_recent_readings',
          message: `Sem leituras há ${Math.floor(minutesSinceLastReading)} minutos`,
          value: minutesSinceLastReading,
          threshold: 5,
        });
      }
    }

    return alerts;
  }

  /**
   * Inicia coleta periódica de métricas
   */
  startCollection() {
    // Limpar métricas antigas a cada 5 minutos
    setInterval(() => {
      // Manter apenas últimas 1000 amostras de latência
      if (this.latency.samples.length > this.latency.maxSamples) {
        this.latency.samples = this.latency.samples.slice(-this.latency.maxSamples);
      }

      // Limpar estatísticas de endpoints antigas (manter apenas últimas 24h de dados)
      // Por enquanto, mantemos tudo - pode ser otimizado no futuro
    }, 5 * 60 * 1000);
  }

  /**
   * Reseta todas as métricas (útil para testes)
   */
  reset() {
    this.telemetry = {
      totalReceived: 0,
      totalProcessed: 0,
      totalFailed: 0,
      lastReceived: null,
      lastProcessed: null,
    };
    this.latency.samples = [];
    this.endpoints.clear();
    this.errors = {
      total: 0,
      byType: new Map(),
      lastError: null,
    };
  }
}

// Singleton
const metricsService = new MetricsService();

export default metricsService;

