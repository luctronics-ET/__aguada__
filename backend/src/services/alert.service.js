import metricsService from './metrics.service.js';
import logger from '../config/logger.js';
import { broadcastAlert } from '../websocket/wsHandler.js';

/**
 * Serviço de alertas automáticos
 */
class AlertService {
  constructor() {
    this.alertHistory = [];
    this.maxHistory = 100;
    this.checkInterval = null;
    this.lastCheck = null;
  }

  /**
   * Inicia monitoramento periódico de alertas
   */
  startMonitoring(intervalMs = 60000) { // 1 minuto por padrão
    if (this.checkInterval) {
      clearInterval(this.checkInterval);
    }

    this.checkInterval = setInterval(async () => {
      await this.checkAndNotify();
    }, intervalMs);

    logger.info('Sistema de alertas iniciado', { intervalMs });
  }

  /**
   * Para monitoramento
   */
  stopMonitoring() {
    if (this.checkInterval) {
      clearInterval(this.checkInterval);
      this.checkInterval = null;
    }
    logger.info('Sistema de alertas parado');
  }

  /**
   * Verifica condições e notifica alertas
   */
  async checkAndNotify() {
    try {
      const alerts = await metricsService.checkAlerts();
      this.lastCheck = new Date();

      // Processar cada alerta
      for (const alert of alerts) {
        // Verificar se já foi notificado recentemente (evitar spam)
        const recentAlert = this.alertHistory.find(
          a => a.type === alert.type && 
          (Date.now() - new Date(a.timestamp).getTime()) < 5 * 60 * 1000 // 5 minutos
        );

        if (!recentAlert) {
          // Novo alerta - notificar
          await this.notifyAlert(alert);
        } else {
          logger.debug('Alerta já notificado recentemente', { type: alert.type });
        }
      }

      // Limpar histórico antigo
      this.cleanHistory();
    } catch (error) {
      logger.error('Erro ao verificar alertas:', error);
    }
  }

  /**
   * Notifica um alerta
   */
  async notifyAlert(alert) {
    const alertRecord = {
      ...alert,
      id: `alert_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`,
      timestamp: new Date().toISOString(),
      notified: true,
    };

    // Adicionar ao histórico
    this.alertHistory.push(alertRecord);

    // Log
    logger.warn('Alerta detectado', alertRecord);

    // Broadcast via WebSocket
    broadcastAlert(alertRecord);

    // Aqui pode adicionar outras notificações:
    // - Email
    // - SMS
    // - Slack/Discord
    // - etc.
  }

  /**
   * Limpa histórico antigo
   */
  cleanHistory() {
    if (this.alertHistory.length > this.maxHistory) {
      // Manter apenas os mais recentes
      this.alertHistory = this.alertHistory
        .sort((a, b) => new Date(b.timestamp) - new Date(a.timestamp))
        .slice(0, this.maxHistory);
    }
  }

  /**
   * Obtém histórico de alertas
   */
  getHistory(limit = 50) {
    return this.alertHistory
      .sort((a, b) => new Date(b.timestamp) - new Date(a.timestamp))
      .slice(0, limit);
  }

  /**
   * Obtém estatísticas de alertas
   */
  getStats() {
    const now = Date.now();
    const last24h = this.alertHistory.filter(
      a => (now - new Date(a.timestamp).getTime()) < 24 * 60 * 60 * 1000
    );

    const byLevel = {
      error: last24h.filter(a => a.level === 'error').length,
      warning: last24h.filter(a => a.level === 'warning').length,
    };

    const byType = {};
    last24h.forEach(alert => {
      byType[alert.type] = (byType[alert.type] || 0) + 1;
    });

    return {
      total: this.alertHistory.length,
      last24h: last24h.length,
      byLevel,
      byType,
      lastCheck: this.lastCheck,
    };
  }
}

// Singleton
const alertService = new AlertService();

export default alertService;

