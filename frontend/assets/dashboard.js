/**
 * Dashboard principal da aplicação estática AGUADA.
 * Extraído de index.html para permitir cache e carregamento não bloqueante.
 */

// Initialize dashboard
async function initDashboard() {
    console.log('[Dashboard] Iniciando...');
    const dashboard = document.getElementById('dashboardGrid');

    try {
        // Aguardar app.js carregar (até 5 segundos)
        let attempts = 0;
        while (!window.apiService && attempts < 50) {
            await new Promise(resolve => setTimeout(resolve, 100));
            attempts++;
        }

        if (!window.apiService) {
            throw new Error('API Service não carregou');
        }

        console.log('[Dashboard] API Service pronto');

        // Buscar dados
        await pollData();

        // Renderizar
        renderDashboard();
        updateStats();

        // Polling periódico
        setInterval(async () => {
            await pollData();
            renderDashboard();
            updateStats();
        }, POLL_INTERVAL);

        // Listener para atualizações em tempo real
        window.addEventListener('readings-updated', () => {
            renderDashboard();
            updateStats();
        });

        console.log('[Dashboard] Inicializado com sucesso!');
    } catch (error) {
        console.error('[Dashboard] Erro:', error);
        dashboard.innerHTML = `
            <div style="grid-column: 1/-1; text-align: center; padding: 40px;">
                <h2 style="color: #ef4444;">❌ Erro ao carregar dados</h2>
                <p style="color: #999;">${error.message}</p>
                <button onclick="window.location.reload()" style="padding: 10px 20px; background: #667eea; color: white; border: none; border-radius: 5px; cursor: pointer; margin-top: 20px;">
                    Recarregar
                </button>
            </div>
        `;
    }
}

// Render dashboard cards
function renderDashboard() {
    const dashboard = document.getElementById('dashboardGrid');

    if (!window.latestReadings) {
        console.warn('[Dashboard] latestReadings não disponível');
        dashboard.innerHTML = '<div style="grid-column: 1/-1; text-align: center; color: #999; padding: 40px;">⏳ Aguardando dados...</div>';
        return;
    }

    console.log('[Dashboard] Renderizando com dados:', window.latestReadings);

    dashboard.innerHTML = Object.entries(SENSORS).map(([id, sensor]) => {
        const reading = latestReadings[id] || {
            distance_cm: 0,
            valve_in: 0,
            valve_out: 0,
            sound_in: 0,
            battery: 5000,
            rssi: -50,
            timestamp: null
        };

        const volumePercent = getVolumePercent(id, reading.distance_cm);
        const isOnline = reading.timestamp && (new Date() - new Date(reading.timestamp)) < 5 * 60 * 1000; // 5 min

        return `
            <div class="card">
                <div class="card-header ${id.toLowerCase()}">
                    <span>${sensor.name}</span>
                    <span class="status-badge ${isOnline ? 'status-online' : 'status-offline'}">
                        ${isOnline ? '● Online' : '● Offline'}
                    </span>
                </div>
                <div class="card-body">
                    ${reading.timestamp && (
                        reading.distance_cm === 0 || reading.distance_cm === 1
                    ) ? `
                        <div class="alert alert-warning">
                            ⚠️ Sensor pode estar com falha (erro: ${reading.distance_cm === 0 ? 'timeout' : 'out of range'})
                        </div>
                    ` : ''}

                    <div class="gauge" style="background: linear-gradient(to right, ${getGaugeColor(volumePercent)})">
                        ${volumePercent.toFixed(1)}%
                    </div>

                    <div class="data-row">
                        <div class="data-item">
                            <div class="data-label">Distância</div>
                            <div class="data-value">
                                ${(reading.distance_cm / 100).toFixed(2)}
                                <span class="data-unit">cm</span>
                            </div>
                        </div>
                        <div class="data-item">
                            <div class="data-label">Volume</div>
                            <div class="data-value">
                                ${volumePercent.toFixed(1)}
                                <span class="data-unit">%</span>
                            </div>
                        </div>
                    </div>

                    <div class="data-row">
                        <div class="data-item">
                            <div class="data-label">Válvula Entrada</div>
                            <div class="data-value">${reading.valve_in ? '🟢 Aberta' : '🔴 Fechada'}</div>
                        </div>
                        <div class="data-item">
                            <div class="data-label">Válvula Saída</div>
                            <div class="data-value">${reading.valve_out ? '🟢 Aberta' : '🔴 Fechada'}</div>
                        </div>
                    </div>

                    <div class="data-row">
                        <div class="data-item">
                            <div class="data-label">Fluxo</div>
                            <div class="data-value">${reading.sound_in ? '💧 Sim' : '⏹️ Não'}</div>
                        </div>
                        <div class="data-item">
                            <div class="data-label">RSSI</div>
                            <div class="data-value">${reading.rssi ? reading.rssi + ' dBm' : '-'}</div>
                        </div>
                    </div>

                    <div class="last-update">
                        ${reading.timestamp ? `Última atualização: ${formatTime(reading.timestamp)}` : 'Aguardando dados...'}
                    </div>
                </div>
            </div>
        `;
    }).join('');
}

// Poll for new data
async function pollData() {
    try {
        await fetchLatestReadings();
    } catch (error) {
        console.error('[Dashboard] Erro ao buscar dados:', error);
    }
}

// Update statistics footer
function updateStats() {
    if (!window.latestReadings) return;
    document.getElementById('totalSensors').textContent = Object.keys(SENSORS).length;
    document.getElementById('lastSync').textContent = formatDateTime(new Date());

    // Calculate average volume
    const volumes = Object.keys(SENSORS).map(id => {
        const reading = latestReadings[id] || { distance_cm: 0 };
        return getVolumePercent(id, reading.distance_cm);
    });
    const avgVolume = volumes.reduce((a, b) => a + b, 0) / volumes.length;
    document.getElementById('avgStorage').textContent = avgVolume.toFixed(1) + '%';
}

// Iniciar dashboard assim que os scripts deferidos terminarem de carregar
document.addEventListener('DOMContentLoaded', () => {
    initDashboard();
});
