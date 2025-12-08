/**
 * AGUADA API Service
 * Centraliza todas as chamadas HTTP ao backend
 */

class ApiService {
  constructor() {
    this.baseURL = this._getBaseURL();
    this.wsURL = this._getWebSocketURL();
    this.timeout = 10000; // 10s timeout
    this.retryAttempts = 3;
    this.retryDelay = 1000; // 1s
    this.ws = null;
    this.wsReconnectInterval = null;
    this.wsCallbacks = [];
    this.lastDataSource = "network";
    this.dbPromise = null;

    this._initOfflineStore();
  }

  /**
   * Detecta URL base do backend
   */
  _getBaseURL() {
    // Usa caminho relativo para passar pelo nginx proxy
    return "/api";
  }

  /**
   * Detecta URL do WebSocket
   */
  _getWebSocketURL() {
    const protocol = window.location.protocol === "https:" ? "wss:" : "ws:";
    const host = window.location.host;
    return `${protocol}//${host}/ws`;
  }

  /**
   * Fetch com timeout e retry
   */
  async _fetchWithRetry(url, options = {}, attempt = 1) {
    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), this.timeout);

    try {
      const response = await fetch(url, {
        ...options,
        signal: controller.signal,
        headers: {
          "Content-Type": "application/json",
          ...options.headers,
        },
      });

      clearTimeout(timeoutId);

      if (!response.ok) {
        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
      }

      return await response.json();
    } catch (error) {
      clearTimeout(timeoutId);

      // Retry lógica
      if (attempt < this.retryAttempts) {
        console.warn(
          `[API] Tentativa ${attempt} falhou, tentando novamente em ${this.retryDelay}ms...`
        );
        await new Promise((resolve) => setTimeout(resolve, this.retryDelay));
        return this._fetchWithRetry(url, options, attempt + 1);
      }

      console.error("[API] Erro após todas as tentativas:", error);
      throw error;
    }
  }

  /**
   * GET /api/readings/latest
   * Últimas leituras de todos os sensores
   */
  async getLatestReadings() {
    try {
      console.log(
        "[API] Buscando leituras de:",
        `${this.baseURL}/readings/latest`
      );
      const data = await this._fetchWithRetry(
        `${this.baseURL}/readings/latest`
      );
      console.log("[API] Resposta recebida:", data);

      // Normalizar formato para o frontend
      const normalized = this._normalizeReadings(data.data);
      this.lastDataSource = "network";
      console.log("[API] Dados normalizados:", normalized);
      return normalized;
    } catch (error) {
      console.error("[API] Erro ao buscar leituras:", error);

      // Fallback: retornar cache ou dados vazios
      const cached = await this._getCachedReadings();
      if (cached) {
        this.lastDataSource = "offline-cache";
        console.warn("[API] Usando dados do cache offline");
        return cached;
      }

      return null;
    }
  }

  /**
   * GET /api/readings/history/:sensor_id
   * Histórico de leituras de um sensor
   */
  async getReadingHistory(sensorId, days = 7, variavel = null) {
    try {
      let url = `${this.baseURL}/readings/history/${sensorId}?days=${days}`;
      if (variavel) {
        url += `&variavel=${variavel}`;
      }

      const data = await this._fetchWithRetry(url);
      return data.data || [];
    } catch (error) {
      console.error(`[API] Erro ao buscar histórico de ${sensorId}:`, error);
      return [];
    }
  }

  /**
   * GET /api/sensors/status
   * Status de todos os sensores (online/offline)
   */
  async getSensorsStatus() {
    try {
      const data = await this._fetchWithRetry(`${this.baseURL}/sensors/status`);
      return data.data || [];
    } catch (error) {
      console.error("[API] Erro ao buscar status dos sensores:", error);
      return [];
    }
  }

  /**
   * GET /api/alerts
   * Lista de alertas com filtros opcionais
   */
  async getAlerts(filters = {}) {
    try {
      const params = new URLSearchParams(filters);
      const data = await this._fetchWithRetry(
        `${this.baseURL}/alerts?${params}`
      );
      return data.data || [];
    } catch (error) {
      console.error("[API] Erro ao buscar alertas:", error);
      return [];
    }
  }

  /**
   * GET /api/stats/daily
   * Estatísticas diárias
   */
  async getDailyStats() {
    try {
      const data = await this._fetchWithRetry(`${this.baseURL}/stats/daily`);
      return data.data || {};
    } catch (error) {
      console.error("[API] Erro ao buscar estatísticas:", error);
      return {};
    }
  }

  /**
   * GET /api/system/health
   * Health check do sistema
   */
  async getSystemHealth() {
    try {
      const data = await this._fetchWithRetry(`${this.baseURL}/system/health`);
      return data;
    } catch (error) {
      console.error("[API] Sistema offline:", error);
      return { status: "offline" };
    }
  }

  /**
   * POST /api/telemetry
   * Enviar telemetria (usado para testes ou simulação)
   */
  async sendTelemetry(payload) {
    try {
      const data = await this._fetchWithRetry(`${this.baseURL}/telemetry`, {
        method: "POST",
        body: JSON.stringify(payload),
      });
      return data;
    } catch (error) {
      console.error("[API] Erro ao enviar telemetria:", error);
      throw error;
    }
  }

  /**
   * Normaliza formato de leituras para o frontend
   * Agrupa sensores por elemento_id (RCON, RCAV, etc.)
   */
  _normalizeReadings(rawData) {
    if (!rawData) {
      console.warn("[API Normalize] rawData is null/undefined");
      return {};
    }

    console.log(
      "[API Normalize] Starting normalization with",
      Object.keys(rawData).length,
      "sensors"
    );
    const normalized = {};

    Object.entries(rawData).forEach(([sensorId, sensorData]) => {
      const elementoId =
        sensorData.elemento_id || this._mapSensorToElemento(sensorId);
      const variables = sensorData.variables || {};

      console.log(
        `[API Normalize] Processing sensor ${sensorId} → elemento ${elementoId}, variables:`,
        Object.keys(variables)
      );

      // Inicializar elemento se não existir
      if (!normalized[elementoId]) {
        normalized[elementoId] = {
          sensor_id: sensorId,
          elemento_id: elementoId,
          mac_address: sensorData.mac_address,
          distance_cm: 0,
          valve_in: 0,
          valve_out: 0,
          sound_in: 0,
          battery: 5000,
          rssi: -50,
          timestamp: new Date().toISOString(),
        };
        console.log(
          `[API Normalize] Initialized element ${elementoId}:`,
          normalized[elementoId]
        );
      }

      // Processar cada variável
      Object.entries(variables).forEach(([varName, varData]) => {
        const valor = this._parseValue(varData?.valor, 0);
        const datetime = varData?.datetime;

        // Mapear nome da variável (com ou sem prefixo IE01_, IE02_)
        const cleanVarName = varName.replace(/^(IE01_|IE02_)/, "");

        console.log(
          `[API Normalize] → Variable ${varName} (clean: ${cleanVarName}) = ${valor}, datetime: ${datetime}`
        );

        switch (cleanVarName) {
          case "distance_cm":
            normalized[elementoId].distance_cm = valor;
            if (datetime) normalized[elementoId].timestamp = datetime;
            console.log(
              `[API Normalize]   ✓ Set distance_cm=${valor} for ${elementoId}`
            );
            break;
          case "valve_in":
            normalized[elementoId].valve_in = valor;
            console.log(
              `[API Normalize]   ✓ Set valve_in=${valor} for ${elementoId}`
            );
            break;
          case "valve_out":
            normalized[elementoId].valve_out = valor;
            console.log(
              `[API Normalize]   ✓ Set valve_out=${valor} for ${elementoId}`
            );
            break;
          case "sound_in":
            normalized[elementoId].sound_in = valor;
            console.log(
              `[API Normalize]   ✓ Set sound_in=${valor} for ${elementoId}`
            );
            break;
          case "battery":
            normalized[elementoId].battery = valor;
            break;
          case "rssi":
            normalized[elementoId].rssi = valor;
            break;
        }

        // Atualizar timestamp se mais recente
        if (
          datetime &&
          new Date(datetime) > new Date(normalized[elementoId].timestamp)
        ) {
          normalized[elementoId].timestamp = datetime;
        }
      });
    });

    console.log(
      "[API Normalize] Normalization complete. Elements:",
      Object.keys(normalized)
    );
    console.log("[API Normalize] Normalized data:", normalized);

    // Cache no localStorage
    this._cacheReadings(normalized);

    return normalized;
  }

  /**
   * Obtém configuração do reservatório para conversão
   */
  _getReservoirConfig(elementoId) {
    const reservoirs = {
      RCON: { nivel_max_cm: 450, offset_sensor_cm: 20.0 },
      RCAV: { nivel_max_cm: 450, offset_sensor_cm: 20.0 },
      RB03: { nivel_max_cm: 450, offset_sensor_cm: 20.0 },
      IE01: { nivel_max_cm: 240, offset_sensor_cm: 20.0 },
      IE02: { nivel_max_cm: 240, offset_sensor_cm: 20.0 },
    };
    return reservoirs[elementoId] || null;
  }

  /**
   * Mapeia sensor_id para elemento_id
   */
  _mapSensorToElemento(sensorId) {
    const mapping = {
      SEN_CON_01: "RCON",
      SEN_CAV_01: "RCAV",
      SEN_B03_01: "RB03",
      SEN_IE01_01: "IE01",
      SEN_IE02_01: "IE02",
      // Mapear também os IDs novos que a API retorna
      RCON_US01: "RCON",
      RCAV_US01: "RCAV",
      RB03_US01: "RB03",
      IE01_US01: "IE01",
      IE02_US01: "IE02",
    };

    // Se não encontrar mapeamento direto, tentar extrair do padrão
    if (!mapping[sensorId]) {
      // Padrão: IE01_US01 -> IE01
      const match = sensorId.match(/^([A-Z0-9]+)_/);
      if (match) {
        return match[1];
      }
    }

    return mapping[sensorId] || sensorId;
  }

  /**
   * Parse valor com fallback
   */
  _parseValue(value, fallback = 0) {
    if (value === null || value === undefined) return fallback;
    const parsed = Number(value);
    return isNaN(parsed) ? fallback : parsed;
  }

  /**
   * Cache leituras no localStorage
   */
  _cacheReadings(readings) {
    this._saveOfflineReadings(readings).catch((error) => {
      console.warn("[Offline] Erro ao salvar em IndexedDB:", error);
    });

    this._setLocalCache(readings);
  }

  async _getCachedReadings() {
    const offline = await this._getOfflineReadings();
    if (offline) {
      return offline;
    }
    return this._getLocalCache();
  }

  _setLocalCache(readings) {
    try {
      localStorage.setItem(
        "aguada_readings_cache",
        JSON.stringify({
          data: readings,
          timestamp: Date.now(),
        })
      );
    } catch (error) {
      console.warn("[Cache] Erro ao salvar cache:", error);
    }
  }

  _getLocalCache() {
    try {
      const cached = localStorage.getItem("aguada_readings_cache");
      if (!cached) return null;

      const { data, timestamp } = JSON.parse(cached);
      if (Date.now() - timestamp > 5 * 60 * 1000) {
        return null;
      }
      return data;
    } catch (error) {
      return null;
    }
  }

  _initOfflineStore() {
    if (typeof window === "undefined" || !("indexedDB" in window)) {
      console.warn("[Offline] IndexedDB não suportado neste navegador");
      return;
    }

    this.dbPromise = new Promise((resolve, reject) => {
      const request = indexedDB.open("aguada_offline", 1);

      request.onupgradeneeded = (event) => {
        const db = event.target.result;
        if (!db.objectStoreNames.contains("latest_readings")) {
          db.createObjectStore("latest_readings", { keyPath: "id" });
        }
      };

      request.onsuccess = () => resolve(request.result);
      request.onerror = () => reject(request.error);
    }).catch((error) => {
      console.warn("[Offline] Falha ao iniciar IndexedDB:", error);
      return null;
    });
  }

  async _saveOfflineReadings(readings) {
    if (!this.dbPromise) return;
    try {
      const db = await this.dbPromise;
      if (!db) return;
      const tx = db.transaction(["latest_readings"], "readwrite");
      const store = tx.objectStore("latest_readings");
      store.put({ id: "latest", data: readings, timestamp: Date.now() });

      return new Promise((resolve, reject) => {
        tx.oncomplete = () => resolve(true);
        tx.onerror = () => reject(tx.error);
        tx.onabort = () => reject(tx.error);
      });
    } catch (error) {
      console.warn("[Offline] Erro ao salvar IndexedDB:", error);
    }
  }

  async _getOfflineReadings() {
    if (!this.dbPromise) return null;
    try {
      const db = await this.dbPromise;
      if (!db) return null;

      const tx = db.transaction(["latest_readings"], "readonly");
      const store = tx.objectStore("latest_readings");
      const request = store.get("latest");

      return await new Promise((resolve, reject) => {
        request.onsuccess = () => {
          const result = request.result;
          resolve(result ? result.data : null);
        };
        request.onerror = () => reject(request.error);
      });
    } catch (error) {
      console.warn("[Offline] Erro ao ler IndexedDB:", error);
      return null;
    }
  }

  /**
   * Conecta ao WebSocket para atualizações em tempo real
   */
  connectWebSocket(onMessage) {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      console.log("[WS] Já conectado");
      return;
    }

    try {
      this.ws = new WebSocket(this.wsURL);

      this.ws.onopen = () => {
        console.log("[WS] Conectado ao servidor");

        // Limpar intervalo de reconexão
        if (this.wsReconnectInterval) {
          clearInterval(this.wsReconnectInterval);
          this.wsReconnectInterval = null;
        }
      };

      this.ws.onmessage = (event) => {
        try {
          const message = JSON.parse(event.data);

          // Chamar callback
          if (onMessage && typeof onMessage === "function") {
            onMessage(message);
          }

          // Chamar callbacks registrados
          this.wsCallbacks.forEach((callback) => callback(message));
        } catch (error) {
          console.error("[WS] Erro ao processar mensagem:", error);
        }
      };

      this.ws.onerror = (error) => {
        console.error("[WS] Erro:", error);
      };

      this.ws.onclose = () => {
        console.warn("[WS] Desconectado, tentando reconectar em 5s...");

        // Tentar reconectar
        if (!this.wsReconnectInterval) {
          this.wsReconnectInterval = setInterval(() => {
            console.log("[WS] Tentando reconectar...");
            this.connectWebSocket(onMessage);
          }, 5000);
        }
      };
    } catch (error) {
      console.error("[WS] Erro ao conectar:", error);
    }
  }

  /**
   * Adiciona callback para mensagens WebSocket
   */
  onWebSocketMessage(callback) {
    if (typeof callback === "function") {
      this.wsCallbacks.push(callback);
    }
  }

  /**
   * Desconecta WebSocket
   */
  disconnectWebSocket() {
    if (this.ws) {
      this.ws.close();
      this.ws = null;
    }

    if (this.wsReconnectInterval) {
      clearInterval(this.wsReconnectInterval);
      this.wsReconnectInterval = null;
    }

    this.wsCallbacks = [];
  }

  /**
   * Checa se está conectado ao backend
   */
  async isOnline() {
    try {
      const health = await this.getSystemHealth();
      return health.status === "ok";
    } catch (error) {
      return false;
    }
  }
}

// Exportar instância global
window.apiService = new ApiService();
