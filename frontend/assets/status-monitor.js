/**
 * AGUADA Status Monitor
 * Componente para monitoramento de status online/offline de sensores e gateways
 */

class StatusMonitor {
  constructor(apiService) {
    this.apiService = apiService;
    this.statusData = {
      sensors: new Map(),
      gateways: new Map(),
      config: {},
      systemStatus: "unknown",
    };
    this.pollInterval = null;
    this.callbacks = [];
  }

  /**
   * Inicializa o monitor de status
   */
  async init(pollIntervalMs = 30000) {
    console.log("[StatusMonitor] Iniciando...");

    // Buscar dados iniciais
    await this.refresh();

    // Iniciar polling
    if (pollIntervalMs > 0) {
      this.pollInterval = setInterval(() => this.refresh(), pollIntervalMs);
    }

    return this;
  }

  /**
   * Atualiza dados de status
   */
  async refresh() {
    try {
      const [statusSummary, sensorsStatus, gatewaysStatus, config] =
        await Promise.all([
          this.apiService.getSystemStatusSummary().catch(() => ({})),
          this.apiService
            .getDetailedSensorsStatus()
            .catch(() => ({ data: [] })),
          this.apiService.getGatewaysStatus().catch(() => ({ data: [] })),
          this.apiService.getStatusConfig().catch(() => ({})),
        ]);

      // Atualizar dados internos
      this.statusData.systemStatus = statusSummary.systemStatus || "unknown";
      this.statusData.config = config;

      // Atualizar sensores
      if (sensorsStatus.data) {
        sensorsStatus.data.forEach((sensor) => {
          this.statusData.sensors.set(sensor.sensorId, sensor);
        });
      }

      // Atualizar gateways
      if (gatewaysStatus.data) {
        gatewaysStatus.data.forEach((gateway) => {
          this.statusData.gateways.set(gateway.gatewayId, gateway);
        });
      }

      // Notificar listeners
      this.notifyCallbacks();

      console.log("[StatusMonitor] Status atualizado:", {
        systemStatus: this.statusData.systemStatus,
        sensors: this.statusData.sensors.size,
        gateways: this.statusData.gateways.size,
      });
    } catch (error) {
      console.error("[StatusMonitor] Erro ao atualizar status:", error);
    }
  }

  /**
   * Para o polling
   */
  stop() {
    if (this.pollInterval) {
      clearInterval(this.pollInterval);
      this.pollInterval = null;
    }
  }

  /**
   * Obtém status de um sensor
   */
  getSensorStatus(sensorId) {
    return (
      this.statusData.sensors.get(sensorId) || {
        sensorId,
        status: "unknown",
        message: "Sem dados",
      }
    );
  }

  /**
   * Obtém status de um gateway
   */
  getGatewayStatus(gatewayId) {
    return (
      this.statusData.gateways.get(gatewayId) || {
        gatewayId,
        status: "unknown",
        message: "Sem dados",
      }
    );
  }

  /**
   * Obtém status geral do sistema
   */
  getSystemStatus() {
    return this.statusData.systemStatus;
  }

  /**
   * Obtém resumo de sensores
   */
  getSensorsSummary() {
    const sensors = Array.from(this.statusData.sensors.values());
    return {
      total: sensors.length,
      online: sensors.filter((s) => s.status === "online").length,
      warning: sensors.filter((s) => s.status === "warning").length,
      offline: sensors.filter((s) => s.status === "offline").length,
      unknown: sensors.filter((s) => s.status === "unknown").length,
    };
  }

  /**
   * Obtém resumo de gateways
   */
  getGatewaysSummary() {
    const gateways = Array.from(this.statusData.gateways.values());
    return {
      total: gateways.length,
      online: gateways.filter((g) => g.status === "online").length,
      warning: gateways.filter((g) => g.status === "warning").length,
      offline: gateways.filter((g) => g.status === "offline").length,
      unknown: gateways.filter((g) => g.status === "unknown").length,
    };
  }

  /**
   * Registra callback para atualizações
   */
  onUpdate(callback) {
    this.callbacks.push(callback);
  }

  /**
   * Notifica callbacks
   */
  notifyCallbacks() {
    this.callbacks.forEach((callback) => {
      try {
        callback(this.statusData);
      } catch (error) {
        console.error("[StatusMonitor] Erro em callback:", error);
      }
    });
  }

  /**
   * Renderiza indicador de status (HTML)
   */
  static renderStatusBadge(status, options = {}) {
    const { size = "normal", showLabel = true, elapsedSec = null } = options;

    const colors = {
      online: { bg: "#10b981", text: "#065f46", label: "Online" },
      warning: { bg: "#f59e0b", text: "#92400e", label: "Intermitente" },
      offline: { bg: "#ef4444", text: "#991b1b", label: "Offline" },
      unknown: { bg: "#6b7280", text: "#374151", label: "Desconhecido" },
    };

    const config = colors[status] || colors.unknown;
    const dotSize = size === "small" ? "8px" : "10px";
    const fontSize = size === "small" ? "11px" : "12px";

    let elapsed = "";
    if (elapsedSec !== null && elapsedSec > 0) {
      if (elapsedSec < 60) {
        elapsed = ` (${elapsedSec}s)`;
      } else if (elapsedSec < 3600) {
        elapsed = ` (${Math.floor(elapsedSec / 60)}m)`;
      } else {
        elapsed = ` (${Math.floor(elapsedSec / 3600)}h)`;
      }
    }

    return `
      <span class="status-badge status-${status}" style="
        display: inline-flex;
        align-items: center;
        gap: 6px;
        padding: 3px 10px;
        border-radius: 12px;
        background: ${config.bg}22;
        color: ${config.text};
        font-size: ${fontSize};
        font-weight: 500;
      ">
        <span class="status-dot-indicator" style="
          width: ${dotSize};
          height: ${dotSize};
          border-radius: 50%;
          background: ${config.bg};
          ${
            status === "online"
              ? "box-shadow: 0 0 6px " +
                config.bg +
                "; animation: pulse-status 2s infinite;"
              : ""
          }
        "></span>
        ${showLabel ? `<span>${config.label}${elapsed}</span>` : ""}
      </span>
    `;
  }

  /**
   * Renderiza widget de status do sistema
   */
  renderSystemWidget() {
    const sensorsSummary = this.getSensorsSummary();
    const gatewaysSummary = this.getGatewaysSummary();
    const systemStatus = this.getSystemStatus();

    const statusColors = {
      healthy: "#10b981",
      degraded: "#f59e0b",
      critical: "#ef4444",
      unknown: "#6b7280",
    };

    const statusLabels = {
      healthy: "✅ Sistema Saudável",
      degraded: "⚠️ Sistema Degradado",
      critical: "🔴 Sistema Crítico",
      unknown: "❓ Status Desconhecido",
    };

    return `
      <div class="status-widget" style="
        background: white;
        border-radius: 12px;
        padding: 16px;
        box-shadow: 0 2px 6px rgba(0,0,0,0.08);
        border-left: 4px solid ${
          statusColors[systemStatus] || statusColors.unknown
        };
      ">
        <div class="status-widget-header" style="
          display: flex;
          justify-content: space-between;
          align-items: center;
          margin-bottom: 12px;
        ">
          <h3 style="margin: 0; font-size: 14px; color: #1e3a5f;">
            ${statusLabels[systemStatus] || statusLabels.unknown}
          </h3>
          <span style="font-size: 11px; color: #6b7280;">
            ${new Date().toLocaleTimeString("pt-BR")}
          </span>
        </div>
        
        <div class="status-widget-body" style="display: flex; gap: 20px;">
          <div class="status-group">
            <div style="font-size: 11px; color: #6b7280; margin-bottom: 4px;">Sensores</div>
            <div style="display: flex; gap: 8px; flex-wrap: wrap;">
              ${
                sensorsSummary.online > 0
                  ? `<span style="color: #10b981; font-weight: 600;">🟢 ${sensorsSummary.online}</span>`
                  : ""
              }
              ${
                sensorsSummary.warning > 0
                  ? `<span style="color: #f59e0b; font-weight: 600;">🟡 ${sensorsSummary.warning}</span>`
                  : ""
              }
              ${
                sensorsSummary.offline > 0
                  ? `<span style="color: #ef4444; font-weight: 600;">🔴 ${sensorsSummary.offline}</span>`
                  : ""
              }
              ${
                sensorsSummary.total === 0
                  ? `<span style="color: #6b7280;">Nenhum</span>`
                  : ""
              }
            </div>
          </div>
          
          <div class="status-group">
            <div style="font-size: 11px; color: #6b7280; margin-bottom: 4px;">Gateways</div>
            <div style="display: flex; gap: 8px; flex-wrap: wrap;">
              ${
                gatewaysSummary.online > 0
                  ? `<span style="color: #10b981; font-weight: 600;">🟢 ${gatewaysSummary.online}</span>`
                  : ""
              }
              ${
                gatewaysSummary.warning > 0
                  ? `<span style="color: #f59e0b; font-weight: 600;">🟡 ${gatewaysSummary.warning}</span>`
                  : ""
              }
              ${
                gatewaysSummary.offline > 0
                  ? `<span style="color: #ef4444; font-weight: 600;">🔴 ${gatewaysSummary.offline}</span>`
                  : ""
              }
              ${
                gatewaysSummary.total === 0
                  ? `<span style="color: #6b7280;">Nenhum</span>`
                  : ""
              }
            </div>
          </div>
        </div>
      </div>
    `;
  }
}

// CSS para animações de status
const statusAnimationStyles = `
  @keyframes pulse-status {
    0%, 100% { opacity: 1; transform: scale(1); }
    50% { opacity: 0.7; transform: scale(1.15); }
  }
  
  .status-badge.status-online .status-dot-indicator {
    animation: pulse-status 2s ease-in-out infinite;
  }
  
  .status-badge.status-warning .status-dot-indicator {
    animation: pulse-status 1s ease-in-out infinite;
  }
`;

// Injetar CSS
if (typeof document !== "undefined") {
  const style = document.createElement("style");
  style.textContent = statusAnimationStyles;
  document.head.appendChild(style);
}

// Exportar para uso global
if (typeof window !== "undefined") {
  window.StatusMonitor = StatusMonitor;
}

// Não usar export default em ambiente de browser sem bundler
// export default StatusMonitor;
