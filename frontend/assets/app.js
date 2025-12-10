/**
 * AGUADA Frontend - Shared JavaScript
 * Integrado com API real via api-service.js
 */

// Configuration
const POLL_INTERVAL = 10000; // 10 seconds
const WS_ENABLED = true; // Enable WebSocket for real-time updates

// =============================================================================
// REGRA GLOBAL: Exibição de Valores Offline
// =============================================================================
// Quando sensor offline: exibe último valor conhecido com estilo cinza transparente
// Formato de volume: XX.X m³

/**
 * Verifica se sensor está offline baseado no timestamp
 * @param {string|Date} timestamp - Timestamp da última leitura
 * @param {number} thresholdMs - Limiar em ms (padrão: 5 minutos)
 * @returns {{isOffline: boolean, isWarning: boolean, elapsedMs: number}}
 */
function checkSensorStatus(timestamp, thresholdMs = 5 * 60 * 1000) {
  if (!timestamp) {
    return { isOffline: true, isWarning: false, elapsedMs: Infinity };
  }
  const now = new Date();
  const lastReading = new Date(timestamp);
  const elapsedMs = now - lastReading;

  return {
    isOffline: elapsedMs >= thresholdMs,
    isWarning: elapsedMs >= 2 * 60 * 1000 && elapsedMs < thresholdMs,
    elapsedMs: elapsedMs,
  };
}

/**
 * Formata valor de sensor com indicação de offline
 * @param {number|null} value - Valor atual
 * @param {string} unit - Unidade (ex: "m³", "m", "V")
 * @param {string|Date} timestamp - Timestamp da leitura
 * @param {Object} options - Opções de formatação
 * @returns {{html: string, isOffline: boolean, formattedValue: string}}
 */
function formatSensorValue(value, unit, timestamp, options = {}) {
  const {
    decimals = 1,
    showTimestamp = true,
    offlineLabel = "Último valor",
    naValue = "N/A",
  } = options;

  const status = checkSensorStatus(timestamp);

  // Se não há valor válido
  if (value === null || value === undefined || isNaN(value)) {
    return {
      html: `<span class="sensor-value sensor-value-na">${naValue}</span>`,
      isOffline: true,
      formattedValue: naValue,
    };
  }

  const formattedValue =
    Number(value).toFixed(decimals) + (unit ? ` ${unit}` : "");
  const timestampStr = timestamp ? formatDateTime(timestamp) : "";

  if (status.isOffline) {
    // Sensor offline - exibir com estilo cinza transparente
    return {
      html: `
                <span class="sensor-value sensor-value-offline" title="${offlineLabel}: ${timestampStr}">
                    ${formattedValue}
                    ${
                      showTimestamp
                        ? `<span class="sensor-value-timestamp">${timestampStr}</span>`
                        : ""
                    }
                </span>
            `,
      isOffline: true,
      formattedValue: formattedValue,
    };
  }

  // Sensor online
  return {
    html: `<span class="sensor-value sensor-value-online">${formattedValue}</span>`,
    isOffline: false,
    formattedValue: formattedValue,
  };
}

/**
 * Formata volume em m³ com padrão XX.X
 * @param {number} volumeM3 - Volume em metros cúbicos
 * @param {string|Date} timestamp - Timestamp da leitura
 * @returns {string} HTML formatado
 */
function formatVolume(volumeM3, timestamp) {
  const result = formatSensorValue(volumeM3, "m³", timestamp, {
    decimals: 1,
    showTimestamp: true,
    offlineLabel: "Última leitura",
  });
  return result.html;
}

/**
 * Formata percentual com padrão XX.X%
 * @param {number} percent - Percentual
 * @param {string|Date} timestamp - Timestamp da leitura
 * @returns {string} HTML formatado
 */
function formatPercent(percent, timestamp) {
  const result = formatSensorValue(percent, "%", timestamp, {
    decimals: 1,
    showTimestamp: false,
  });
  return result.html;
}

// Exportar funções globalmente
window.checkSensorStatus = checkSensorStatus;
window.formatSensorValue = formatSensorValue;
window.formatVolume = formatVolume;
window.formatPercent = formatPercent;

// =============================================================================
// Sensor configuration (mapped from backend)
// =============================================================================
const SENSORS = {
  RCON: {
    sensor_id: "SEN_CON_01",
    mac: "20:6e:f1:6b:77:58",
    name: "Castelo de Consumo (RCON)",
    height: 400,
    color: "rcon",
  },
  RCAV: {
    sensor_id: "SEN_CAV_01",
    mac: "dc:06:75:67:6a:cc",
    name: "Castelo de Incêndio (RCAV)",
    height: 350,
    color: "rcav",
  },
  RB03: {
    sensor_id: "SEN_B03_01",
    mac: null,
    name: "Casa de Bombas RB03",
    height: 300,
    color: "rb03",
  },
  IE01: {
    sensor_id: "SEN_IE01_01",
    mac: null,
    name: "Cisterna IE01",
    height: 250,
    color: "ie01",
  },
  IE02: {
    sensor_id: "SEN_IE02_01",
    mac: null,
    name: "Cisterna IE02",
    height: 250,
    color: "ie02",
  },
};

// Global state
let latestReadings = window.latestReadings || {
  RCON: {
    distance_cm: 0,
    valve_in: 0,
    valve_out: 0,
    sound_in: 0,
    timestamp: null,
    battery: 0,
    rssi: 0,
  },
  RCAV: {
    distance_cm: 0,
    valve_in: 0,
    valve_out: 0,
    sound_in: 0,
    timestamp: null,
    battery: 0,
    rssi: 0,
  },
  RB03: {
    distance_cm: 0,
    valve_in: 0,
    valve_out: 0,
    sound_in: 0,
    timestamp: null,
    battery: 0,
    rssi: 0,
  },
  IE01: {
    distance_cm: 0,
    valve_in: 0,
    valve_out: 0,
    sound_in: 0,
    timestamp: null,
    battery: 0,
    rssi: 0,
  },
  IE02: {
    distance_cm: 0,
    valve_in: 0,
    valve_out: 0,
    sound_in: 0,
    timestamp: null,
    battery: 0,
    rssi: 0,
  },
};

// Exportar para window para acesso global
window.latestReadings = latestReadings;
window.SENSORS = SENSORS;

let allReadings = [];
let alerts = [];

const offlineState = {
  banner: null,
  forceCache: false,
};

registerServiceWorker();
setupOfflineHandlers();

function registerServiceWorker() {
  if (typeof window === "undefined" || !("serviceWorker" in navigator)) return;
  window.addEventListener("load", () => {
    navigator.serviceWorker
      .register("./service-worker.js")
      .then(() => console.log("[SW] Registrado com sucesso"))
      .catch((error) => console.warn("[SW] Falha ao registrar:", error));
  });
}

function setupOfflineHandlers() {
  if (typeof window === "undefined") return;

  const initBanner = () => {
    if (offlineState.banner || !document.body) return;
    const banner = document.createElement("div");
    banner.id = "offlineBanner";
    banner.textContent = "Modo offline: exibindo dados em cache";
    document.body.appendChild(banner);
    offlineState.banner = banner;
    updateConnectivityStatus();
  };

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", initBanner);
  } else {
    initBanner();
  }

  window.addEventListener("online", () => updateConnectivityStatus());
  window.addEventListener("offline", () => updateConnectivityStatus());
}

function updateConnectivityStatus(options = {}) {
  if (typeof window === "undefined" || !document.body) return;
  const isOffline = !navigator.onLine;
  const forceCache = options.forceCache ?? offlineState.forceCache;
  offlineState.forceCache = forceCache;

  document.body.classList.toggle("offline", isOffline);
  document.body.classList.toggle("cache-mode", !isOffline && forceCache);

  if (offlineState.banner) {
    if (isOffline) {
      offlineState.banner.textContent = "Sem conexão. Exibindo dados em cache.";
    } else if (forceCache) {
      offlineState.banner.textContent =
        "Backend indisponível. Exibindo dados armazenados.";
    } else {
      offlineState.banner.textContent = "Conectado";
    }
  }

  document.querySelectorAll(".status-badge").forEach((badge) => {
    if (isOffline || forceCache) {
      badge.classList.remove("status-online");
      badge.classList.add("status-offline");
      badge.textContent = "● Offline";
    } else {
      badge.classList.remove("status-offline");
      badge.classList.add("status-online");
      badge.textContent = "● Online";
    }
  });
}

/**
 * Get reservoir configuration from reservoirs.json
 */
function getReservoirConfig(sensorId) {
  // Mapeamento direto dos IDs
  // Área é calculada automaticamente: area_m2 = volume_max_m3 / (nivel_max_cm / 100)
  const reservoirs = {
    // Cilíndricos: nivel_max = 450cm, volume_max = 80m3
    RCON: {
      tipo: "cilindrico",
      nivel_max_cm: 450,
      volume_max_m3: 80.0,
      area_m2: 80.0 / 4.5, // Calculado: 17.777...
      offset_sensor_cm: 20.0, // Distância mínima do sensor acima do nível máximo
    },
    RCAV: {
      tipo: "cilindrico",
      nivel_max_cm: 450,
      volume_max_m3: 80.0,
      area_m2: 80.0 / 4.5, // Calculado: 17.777...
      offset_sensor_cm: 20.0, // Distância mínima do sensor acima do nível máximo
    },
    RB03: {
      tipo: "cilindrico",
      nivel_max_cm: 450,
      volume_max_m3: 80.0,
      area_m2: 80.0 / 4.5, // Calculado: 17.777...
      offset_sensor_cm: 20.0, // Distância mínima do sensor acima do nível máximo
    },
    // Retangulares: nivel_max = 240cm, volume_max = 250.0m3
    IE01: {
      tipo: "retangular",
      nivel_max_cm: 240,
      volume_max_m3: 250.0,
      area_m2: 250.0 / 2.4, // Calculado: 104.166...
      offset_sensor_cm: 20.0, // Distância mínima do sensor acima do nível máximo
    },
    IE02: {
      tipo: "retangular",
      nivel_max_cm: 240,
      volume_max_m3: 250.0,
      area_m2: 250.0 / 2.4, // Calculado: 104.166...
      offset_sensor_cm: 20.0, // Distância mínima do sensor acima do nível máximo
    },
  };
  return reservoirs[sensorId] || null;
}

/**
 * Calculate volume in m³ from distance measurement
 * nivel_atual_cm = nivel_max_cm - distancia_cm + offset_sensor_cm
 * Volume = área_base × (nível_atual_cm / 100)
 */
function calculateVolumeM3(sensorId, distance_cm) {
  const reservoir = getReservoirConfig(sensorId);

  // Validações robustas
  if (!reservoir) {
    console.warn(`[Volume] Configuração não encontrada para ${sensorId}`);
    return 0;
  }

  // Aceitar 0 como valor válido (reservatório vazio)
  if (distance_cm === null || distance_cm === undefined) {
    console.warn(`[Volume] Distância inválida para ${sensorId}:`, distance_cm);
    return 0;
  }

  const distanceNum = Number(distance_cm);
  if (isNaN(distanceNum) || distanceNum < 0) {
    console.warn(
      `[Volume] Distância fora do range para ${sensorId}:`,
      distance_cm
    );
    return 0;
  }

  // nivel_atual_cm = nivel_max_cm - distancia_cm + offset_sensor_cm
  const offset_sensor_cm = reservoir.offset_sensor_cm || 20.0;
  const nivel_cm = reservoir.nivel_max_cm - distanceNum + offset_sensor_cm;

  // Nível negativo = reservatório vazio
  if (nivel_cm <= 0) return 0;

  // Volume = área_base × (nível_cm / 100) para converter para metros
  const volume_m3 = reservoir.area_m2 * (nivel_cm / 100);

  // Limitar ao volume máximo e garantir não-negativo
  return Math.max(0, Math.min(volume_m3, reservoir.volume_max_m3));
}

/**
 * Calculate volume percentage
 * Percentual = (volume_atual / volume_total) × 100
 */
function getVolumePercent(sensorId, distance_cm) {
  const reservoir = getReservoirConfig(sensorId);

  if (!reservoir) {
    console.warn(`[Percent] Configuração não encontrada para ${sensorId}`);
    return 0;
  }

  // Aceitar 0 como válido
  if (distance_cm === null || distance_cm === undefined) {
    return 0;
  }

  const volume_atual = calculateVolumeM3(sensorId, distance_cm);
  const volume_total = reservoir.volume_max_m3;

  if (volume_total <= 0) {
    console.error(
      `[Percent] Volume total inválido para ${sensorId}:`,
      volume_total
    );
    return 0;
  }

  const percentual = (volume_atual / volume_total) * 100;

  // Limitar entre 0 e 100 e arredondar para 2 casas
  return Math.round(Math.max(0, Math.min(100, percentual)) * 100) / 100;
}

/**
 * Get gauge color based on percentage
 */
function getGaugeColor(percent) {
  if (percent <= 20) return "#ef4444"; // Red - low
  if (percent <= 50) return "#f59e0b"; // Orange - medium
  return "#10b981"; // Green - high
}

/**
 * Format timestamp com validação robusta
 */
function formatTime(date) {
  if (!date) return "-";
  try {
    const d = new Date(date);
    if (isNaN(d.getTime())) return String(date);
    return d.toLocaleTimeString("pt-BR", {
      hour: "2-digit",
      minute: "2-digit",
      second: "2-digit",
    });
  } catch (error) {
    console.warn("[Format] Erro ao formatar time:", error);
    return String(date);
  }
}

function formatDate(date) {
  if (!date) return "-";
  try {
    const d = new Date(date);
    if (isNaN(d.getTime())) return String(date);
    return d.toLocaleDateString("pt-BR", {
      day: "2-digit",
      month: "2-digit",
      year: "numeric",
    });
  } catch (error) {
    console.warn("[Format] Erro ao formatar date:", error);
    return String(date);
  }
}

function formatDateTime(date) {
  if (!date) return "-";
  try {
    const d = new Date(date);
    if (isNaN(d.getTime())) return String(date);
    return d.toLocaleString("pt-BR", {
      day: "2-digit",
      month: "2-digit",
      year: "numeric",
      hour: "2-digit",
      minute: "2-digit",
      second: "2-digit",
    });
  } catch (error) {
    console.warn("[Format] Erro ao formatar datetime:", error);
    return String(date);
  }
}

/**
 * Fetch latest readings from API
 */
async function fetchLatestReadings() {
  try {
    console.log("[App] Buscando leituras da API...");

    if (!window.apiService) {
      console.error("[App] API Service não carregado");
      return null;
    }

    const readings = await window.apiService.getLatestReadings();
    console.log("[App] Leituras recebidas:", readings);

    if (readings) {
      // Atualizar state global
      Object.assign(latestReadings, readings);
      Object.assign(window.latestReadings, readings);

      // Disparar evento customizado
      window.dispatchEvent(
        new CustomEvent("readings-updated", { detail: readings })
      );

      updateConnectivityStatus({
        forceCache: window.apiService?.lastDataSource === "offline-cache",
      });

      return readings;
    }

    updateConnectivityStatus({
      forceCache: window.apiService?.lastDataSource === "offline-cache",
    });
    return null;
  } catch (error) {
    console.error("[App] Erro ao buscar leituras:", error);
    updateConnectivityStatus({
      forceCache: window.apiService?.lastDataSource === "offline-cache",
    });
    return null;
  }
}

/**
 * Fetch reading history from API
 */
async function fetchHistoryData(elementoId, days = 7, variavel = null) {
  try {
    if (!window.apiService) {
      console.error("[App] API Service não carregado");
      return [];
    }

    const sensor = SENSORS[elementoId];
    if (!sensor) {
      console.error(`[App] Sensor não encontrado: ${elementoId}`);
      return [];
    }

    const history = await window.apiService.getReadingHistory(
      sensor.sensor_id,
      days,
      variavel
    );

    return history;
  } catch (error) {
    console.error(`[App] Erro ao buscar histórico de ${elementoId}:`, error);
    return [];
  }
}

/**
 * Legacy function for compatibility (deprecated)
 */
function generateHistoryData(sensorId, days = 7) {
  console.warn("[App] generateHistoryData é deprecated, use fetchHistoryData");
  return fetchHistoryData(sensorId, days);
}

/**
 * Navigate to page
 */
function navigateTo(page) {
  // Update active nav
  document.querySelectorAll("nav a, nav button").forEach((link) => {
    link.classList.remove("active");
  });
  event.target.classList.add("active");

  // In a real app, use router
  // window.location.href = `${page}.html`;

  console.log(`Navigate to: ${page}.html`);
}

/**
 * Update stats
 */
function updateStats() {
  const activeSensors = Object.values(latestReadings).filter(
    (r) => r.timestamp
  ).length;
  const totalVolume = Object.entries(latestReadings).reduce((sum, [id, r]) => {
    return sum + getVolumePercent(id, r.distance_cm);
  }, 0);
  const avgLevel = totalVolume / Object.keys(SENSORS).length;

  if (document.getElementById("totalSensors")) {
    document.getElementById("totalSensors").textContent = activeSensors;
  }
  if (document.getElementById("totalReadings")) {
    document.getElementById("totalReadings").textContent = "1,248";
  }
  if (document.getElementById("avgStorage")) {
    document.getElementById("avgStorage").textContent =
      Math.round(avgLevel) + "%";
  }
  if (document.getElementById("lastSync")) {
    document.getElementById("lastSync").textContent =
      new Date().toLocaleTimeString("pt-BR");
  }
}

/**
 * Generate sensor alert
 */
function generateAlert(sensorId, level, message) {
  return {
    id: Date.now(),
    sensor: sensorId,
    level: level, // 'info', 'warning', 'error'
    message: message,
    timestamp: new Date().toISOString(),
  };
}

/**
 * Export data to CSV
 */
function exportToCSV(data, filename) {
  const csv = [
    Object.keys(data[0]).join(","),
    ...data.map((row) => Object.values(row).join(",")),
  ].join("\n");

  const blob = new Blob([csv], { type: "text/csv" });
  const url = window.URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = filename;
  a.click();
}

/**
 * Initialize common UI elements
 */
function initializeUI() {
  // Add navigation
  const navHtml = `
        <nav>
            <a href="index.html" class="active">Dashboard</a>
            <a href="history.html">Histórico</a>
            <a href="alerts.html">Alertas</a>
            <a href="system.html">Sistema</a>
            <a href="config.html">Configurações</a>
        </nav>
    `;

  const header = document.querySelector("header");
  if (header && !header.querySelector("nav")) {
    header.insertAdjacentHTML("afterbegin", navHtml);
  }
}

/**
 * Initialize WebSocket connection
 */
function initializeWebSocket() {
  if (!WS_ENABLED || !window.apiService) return;

  window.apiService.connectWebSocket((message) => {
    console.log("[WS] Mensagem recebida:", message);

    // Atualizar leituras em tempo real
    if (message.type === "telemetry" && message.data) {
      const { elemento_id, variavel, valor } = message.data;

      if (latestReadings[elemento_id]) {
        latestReadings[elemento_id][variavel] = valor;
        latestReadings[elemento_id].timestamp = new Date().toISOString();

        // Disparar evento
        window.dispatchEvent(
          new CustomEvent("reading-updated", {
            detail: { elemento_id, variavel, valor },
          })
        );
      }
    }
  });
}

/**
 * Check system health
 */
async function checkSystemHealth() {
  if (!window.apiService) return false;

  try {
    const isOnline = await window.apiService.isOnline();

    // Atualizar badge de status
    const statusBadges = document.querySelectorAll(".status-badge");
    statusBadges.forEach((badge) => {
      if (
        badge.textContent.includes("Online") ||
        badge.textContent.includes("Offline")
      ) {
        badge.className = isOnline
          ? "status-badge status-online pulsing"
          : "status-badge status-offline";
        badge.textContent = isOnline ? "● Online" : "● Offline";
      }
    });

    return isOnline;
  } catch (error) {
    console.error("[App] Erro ao verificar saúde do sistema:", error);
    return false;
  }
}

/**
 * Initialize application
 */
async function initializeApp() {
  console.log("[App] Inicializando aplicação AGUADA...");

  // Check if API service is loaded
  if (!window.apiService) {
    console.error(
      "[App] API Service não encontrado! Certifique-se de carregar api-service.js"
    );
    return;
  }

  // Initialize UI
  initializeUI();

  // Check system health
  const isOnline = await checkSystemHealth();
  console.log(`[App] Sistema: ${isOnline ? "Online" : "Offline"}`);

  // Connect WebSocket if enabled
  if (WS_ENABLED && isOnline) {
    initializeWebSocket();
  }

  // Initial data fetch
  await fetchLatestReadings();

  console.log("[App] Aplicação inicializada com sucesso");
}

// Auto-initialize when DOM is ready
document.addEventListener("DOMContentLoaded", () => {
  console.log("[App] DOM carregado, iniciando app...");
  initializeApp();
});

// Cleanup on page unload
window.addEventListener("beforeunload", () => {
  if (window.apiService) {
    window.apiService.disconnectWebSocket();
  }
});
