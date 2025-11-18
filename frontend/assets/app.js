/**
 * AGUADA Frontend - Shared JavaScript
 */

// Configuration
const API_BASE = 'http://192.168.0.100:3000/api';
const POLL_INTERVAL = 10000; // 10 seconds

// Sensor configuration
const SENSORS = {
    'RCON': { 
        mac: '20:6E:F1:6B:77:58', 
        name: 'Reservatório RCON', 
        height: 400,
        color: 'rcon'
    },
    'RCAV': { 
        mac: 'DC:06:75:67:6A:CC', 
        name: 'Reservatório RCAV', 
        height: 350,
        color: 'rcav'
    },
    'RB03': { 
        mac: 'XX:XX:XX:XX:XX:XX', 
        name: 'Reservatório RB03', 
        height: 300,
        color: 'rb03'
    },
    'IE01': { 
        mac: 'XX:XX:XX:XX:XX:XX', 
        name: 'Elemento IE01', 
        height: 250,
        color: 'ie01'
    },
    'IE02': { 
        mac: 'XX:XX:XX:XX:XX:XX', 
        name: 'Elemento IE02', 
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
 * Poll for new data
 */
async function fetchLatestReadings() {
    try {
        // In production, fetch from API:
        // const response = await fetch(`${API_BASE}/readings/latest`);
        // const data = await response.json();

        // For demo, simulate random data
        Object.keys(SENSORS).forEach(sensorId => {
            latestReadings[sensorId] = {
                distance_cm: Math.floor(Math.random() * 40000) + 10000,
                valve_in: Math.random() > 0.5 ? 1 : 0,
                valve_out: Math.random() > 0.5 ? 1 : 0,
                sound_in: Math.random() > 0.7 ? 1 : 0,
                battery: 4800 + Math.floor(Math.random() * 400),
                rssi: -50 - Math.floor(Math.random() * 30),
                timestamp: new Date().toISOString(),
            };
        });

        return latestReadings;
    } catch (error) {
        console.error('Erro ao buscar leituras:', error);
        return null;
    }
}

/**
 * Simulate reading history for demo
 */
function generateHistoryData(sensorId, days = 7) {
    const readings = [];
    const now = new Date();
    
    for (let i = days * 24; i >= 0; i--) {
        const timestamp = new Date(now - i * 60 * 60 * 1000);
        readings.push({
            timestamp: timestamp.toISOString(),
            distance_cm: Math.floor(Math.random() * 40000) + 10000 + Math.sin(i / 24) * 5000,
            valve_in: Math.random() > 0.7 ? 1 : 0,
            valve_out: Math.random() > 0.7 ? 1 : 0,
            sound_in: Math.random() > 0.8 ? 1 : 0,
        });
    }
    
    return readings;
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

// Auto-initialize when DOM is ready
document.addEventListener('DOMContentLoaded', initializeUI);
