/**
 * AGUADA Frontend - Shared JavaScript
 * Integrado com API real via api-service.js
 */

// Configuration
const POLL_INTERVAL = 10000; // 10 seconds
const WS_ENABLED = true; // Enable WebSocket for real-time updates

// Sensor configuration (mapped from backend)
const SENSORS = {
    'RCON': { 
        sensor_id: 'SEN_CON_01',
        mac: '20:6e:f1:6b:77:58', 
        name: 'Castelo de Consumo (RCON)', 
        height: 400,
        color: 'rcon'
    },
    'RCAV': { 
        sensor_id: 'SEN_CAV_01',
        mac: 'dc:06:75:67:6a:cc', 
        name: 'Castelo de Incêndio (RCAV)', 
        height: 350,
        color: 'rcav'
    },
    'RB03': { 
        sensor_id: 'SEN_B03_01',
        mac: null, 
        name: 'Casa de Bombas RB03', 
        height: 300,
        color: 'rb03'
    },
    'IE01': { 
        sensor_id: 'SEN_IE01_01',
        mac: null, 
        name: 'Cisterna IE01', 
        height: 250,
        color: 'ie01'
    },
    'IE02': { 
        sensor_id: 'SEN_IE02_01',
        mac: null, 
        name: 'Cisterna IE02', 
        height: 250,
        color: 'ie02'
    },
};

// Global state
let latestReadings = {
    'RCON': { distance_cm: 0, valve_in: 0, valve_out: 0, sound_in: 0, timestamp: null, battery: 0, rssi: 0 },
    'RCAV': { distance_cm: 0, valve_in: 0, valve_out: 0, sound_in: 0, timestamp: null, battery: 0, rssi: 0 },
    'RB03': { distance_cm: 0, valve_in: 0, valve_out: 0, sound_in: 0, timestamp: null, battery: 0, rssi: 0 },
    'IE01': { distance_cm: 0, valve_in: 0, valve_out: 0, sound_in: 0, timestamp: null, battery: 0, rssi: 0 },
    'IE02': { distance_cm: 0, valve_in: 0, valve_out: 0, sound_in: 0, timestamp: null, battery: 0, rssi: 0 },
};

// Exportar para window para acesso global
window.latestReadings = latestReadings;
window.SENSORS = SENSORS;

let allReadings = [];
let alerts = [];

/**
 * Format distance to volume percentage
 */
function getVolumePercent(sensorId, distance_cm) {
    const sensor = SENSORS[sensorId];
    if (!sensor) return 0;
    
    const level = (distance_cm / (sensor.height * 100)) * 100;
    const volumePercent = 100 - (level > 100 ? 100 : level < 0 ? 0 : level);
    return volumePercent;
}

/**
 * Get gauge color based on percentage
 */
function getGaugeColor(percent) {
    if (percent <= 20) return '#ef4444'; // Red - low
    if (percent <= 50) return '#f59e0b'; // Orange - medium
    return '#10b981'; // Green - high
}

/**
 * Format timestamp
 */
function formatTime(date) {
    if (!date) return '-';
    return new Date(date).toLocaleTimeString('pt-BR');
}

function formatDate(date) {
    if (!date) return '-';
    return new Date(date).toLocaleDateString('pt-BR');
}

function formatDateTime(date) {
    if (!date) return '-';
    return new Date(date).toLocaleString('pt-BR');
}

/**
 * Fetch latest readings from API
 */
async function fetchLatestReadings() {
    try {
        console.log('[App] Buscando leituras da API...');
        
        if (!window.apiService) {
            console.error('[App] API Service não carregado');
            return null;
        }

        const readings = await window.apiService.getLatestReadings();
        console.log('[App] Leituras recebidas:', readings);
        
        if (readings) {
            // Atualizar state global
            Object.assign(latestReadings, readings);
            Object.assign(window.latestReadings, readings);
            
            // Disparar evento customizado
            window.dispatchEvent(new CustomEvent('readings-updated', { detail: readings }));
            
            return readings;
        }
        
        return null;
    } catch (error) {
        console.error('[App] Erro ao buscar leituras:', error);
        return null;
    }
}

/**
 * Fetch reading history from API
 */
async function fetchHistoryData(elementoId, days = 7, variavel = null) {
    try {
        if (!window.apiService) {
            console.error('[App] API Service não carregado');
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
    console.warn('[App] generateHistoryData é deprecated, use fetchHistoryData');
    return fetchHistoryData(sensorId, days);
}

/**
 * Navigate to page
 */
function navigateTo(page) {
    // Update active nav
    document.querySelectorAll('nav a, nav button').forEach(link => {
        link.classList.remove('active');
    });
    event.target.classList.add('active');
    
    // In a real app, use router
    // window.location.href = `${page}.html`;
    
    console.log(`Navigate to: ${page}.html`);
}

/**
 * Update stats
 */
function updateStats() {
    const activeSensors = Object.values(latestReadings).filter(r => r.timestamp).length;
    const totalVolume = Object.entries(latestReadings).reduce((sum, [id, r]) => {
        return sum + getVolumePercent(id, r.distance_cm);
    }, 0);
    const avgLevel = totalVolume / Object.keys(SENSORS).length;

    if (document.getElementById('totalSensors')) {
        document.getElementById('totalSensors').textContent = activeSensors;
    }
    if (document.getElementById('totalReadings')) {
        document.getElementById('totalReadings').textContent = '1,248';
    }
    if (document.getElementById('avgStorage')) {
        document.getElementById('avgStorage').textContent = Math.round(avgLevel) + '%';
    }
    if (document.getElementById('lastSync')) {
        document.getElementById('lastSync').textContent = new Date().toLocaleTimeString('pt-BR');
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
        Object.keys(data[0]).join(','),
        ...data.map(row => Object.values(row).join(','))
    ].join('\n');
    
    const blob = new Blob([csv], { type: 'text/csv' });
    const url = window.URL.createObjectURL(blob);
    const a = document.createElement('a');
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
    
    const header = document.querySelector('header');
    if (header && !header.querySelector('nav')) {
        header.insertAdjacentHTML('afterbegin', navHtml);
    }
}

/**
 * Initialize WebSocket connection
 */
function initializeWebSocket() {
    if (!WS_ENABLED || !window.apiService) return;

    window.apiService.connectWebSocket((message) => {
        console.log('[WS] Mensagem recebida:', message);
        
        // Atualizar leituras em tempo real
        if (message.type === 'telemetry' && message.data) {
            const { elemento_id, variavel, valor } = message.data;
            
            if (latestReadings[elemento_id]) {
                latestReadings[elemento_id][variavel] = valor;
                latestReadings[elemento_id].timestamp = new Date().toISOString();
                
                // Disparar evento
                window.dispatchEvent(new CustomEvent('reading-updated', { 
                    detail: { elemento_id, variavel, valor } 
                }));
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
        const statusBadges = document.querySelectorAll('.status-badge');
        statusBadges.forEach(badge => {
            if (badge.textContent.includes('Online') || badge.textContent.includes('Offline')) {
                badge.className = isOnline ? 'status-badge status-online pulsing' : 'status-badge status-offline';
                badge.textContent = isOnline ? '● Online' : '● Offline';
            }
        });
        
        return isOnline;
    } catch (error) {
        console.error('[App] Erro ao verificar saúde do sistema:', error);
        return false;
    }
}

/**
 * Initialize application
 */
async function initializeApp() {
    console.log('[App] Inicializando aplicação AGUADA...');
    
    // Check if API service is loaded
    if (!window.apiService) {
        console.error('[App] API Service não encontrado! Certifique-se de carregar api-service.js');
        return;
    }
    
    // Initialize UI
    initializeUI();
    
    // Check system health
    const isOnline = await checkSystemHealth();
    console.log(`[App] Sistema: ${isOnline ? 'Online' : 'Offline'}`);
    
    // Connect WebSocket if enabled
    if (WS_ENABLED && isOnline) {
        initializeWebSocket();
    }
    
    // Initial data fetch
    await fetchLatestReadings();
    
    console.log('[App] Aplicação inicializada com sucesso');
}

// Auto-initialize when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    console.log('[App] DOM carregado, iniciando app...');
    initializeApp();
});

// Cleanup on page unload
window.addEventListener('beforeunload', () => {
    if (window.apiService) {
        window.apiService.disconnectWebSocket();
    }
});
