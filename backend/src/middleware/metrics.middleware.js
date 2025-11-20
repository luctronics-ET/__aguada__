import metricsService from '../services/metrics.service.js';

/**
 * Middleware para registrar métricas de todas as requisições
 */
export function metricsMiddleware(req, res, next) {
  const startTime = Date.now();

  // Interceptar res.end para registrar métricas após resposta
  const originalEnd = res.end;
  res.end = function(...args) {
    const latency = Date.now() - startTime;
    
    // Registrar métricas
    metricsService.recordEndpointRequest(
      req.method,
      req.path,
      res.statusCode,
      latency
    );
    
    // Registrar latência
    metricsService.recordLatency(latency);
    
    // Registrar erro se status >= 400
    if (res.statusCode >= 400) {
      metricsService.recordError(
        'http_error',
        `${req.method} ${req.path} - ${res.statusCode}`
      );
    }
    
    // Chamar original
    originalEnd.apply(this, args);
  };

  next();
}

export default metricsMiddleware;

