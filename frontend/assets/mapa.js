// frontend/assets/mapa.js
// Renderiza o mapa Leaflet com foco fixo no link do OpenStreetMap solicitado.

(async function () {
  const MAP_CENTER = Object.freeze({ lat: -22.83735, lon: -43.10719 });
  const MAP_ZOOM = 16;
  const MAP_LINK = 'https://www.openstreetmap.org/#map=16/-22.83735/-43.10719';
  const POSITION_OFFSETS = [
    { lat: 0.0004, lon: -0.0003 },
    { lat: -0.0002, lon: 0.0006 },
    { lat: -0.0009, lon: -0.0004 },
    { lat: 0.0007, lon: 0.0005 },
    { lat: -0.0003, lon: 0.0002 },
  ];
  const FALLBACK_RESERVOIRS = [
    { id: 'RCON', nome: 'Castelo de Consumo' },
    { id: 'RCAV', nome: 'Castelo de Incêndio' },
    { id: 'RB03', nome: 'Casa de Bombas RB03' },
    { id: 'IE01', nome: 'Cisterna IE01' },
    { id: 'IE02', nome: 'Cisterna IE02' },
  ];

  let mapInstance = null;
  const markerRefs = {};
  window.__charts = window.__charts || {};

  function parseCoord(value) {
    if (value === null || value === undefined) return null;
    const num = typeof value === 'number' ? value : Number(value);
    return Number.isFinite(num) ? num : null;
  }

  async function loadConfig() {
    const paths = [
      '../config/reservoirs.json',
      '/config/reservoirs.json',
      'config/reservoirs.json',
    ];
    for (const path of paths) {
      try {
        const response = await fetch(path, { cache: 'no-store' });
        if (!response.ok) throw new Error('Sem acesso');
        const data = await response.json();
        return data.reservoirs || data;
      } catch (error) {
        // tenta próximo caminho
      }
    }
    return null;
  }

  function normalizeReservoirs(list) {
    const base = Array.isArray(list) && list.length ? list : FALLBACK_RESERVOIRS;
    return base.map((item, index) => {
      const coords = item.coordenadas || {};
      const lat = parseCoord(coords.latitude);
      const lon = parseCoord(coords.longitude);
      const offset = POSITION_OFFSETS[index % POSITION_OFFSETS.length];

      return {
        ...item,
        id: item.id || item.elemento_id || item.alias || `RES-${index + 1}`,
        nome: item.nome || item.alias || `Reservatório ${index + 1}`,
        coordenadas: {
          ...coords,
          latitude: lat ?? MAP_CENTER.lat + offset.lat,
          longitude: lon ?? MAP_CENTER.lon + offset.lon,
        },
      };
    });
  }

  function renderReservoirList(reservoirs) {
    const listEl = document.getElementById('reservoirList');
    if (!listEl) return;
    listEl.innerHTML = reservoirs
      .map(
        (item) => `
        <li class="reservoir-item">
          <div>
            <strong>${item.nome}</strong>
            <span>${item.id}</span>
          </div>
          <button type="button" data-res-id="${item.id}">Localizar</button>
        </li>`
      )
      .join('');
  }

  function ensureMap() {
    if (mapInstance) return mapInstance;
    mapInstance = L.map('map', {
      zoomControl: true,
      attributionControl: true,
    }).setView([MAP_CENTER.lat, MAP_CENTER.lon], MAP_ZOOM);

    L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
      attribution: '&copy; OpenStreetMap contributors',
      maxZoom: 19,
    }).addTo(mapInstance);
    return mapInstance;
  }

  function wireToolbarActions(map) {
    const recenterBtn = document.getElementById('recenterMap');
    const refreshBtn = document.getElementById('refreshLayers');
    const listEl = document.getElementById('reservoirList');

    recenterBtn?.addEventListener('click', () => {
      map.flyTo([MAP_CENTER.lat, MAP_CENTER.lon], MAP_ZOOM, { duration: 0.6 });
    });

    refreshBtn?.addEventListener('click', () => {
      Object.values(markerRefs).forEach((marker) => {
        if (marker.isPopupOpen && marker.isPopupOpen()) {
          marker.fire('popupopen');
        }
      });
    });

    listEl?.addEventListener('click', (event) => {
      const button = event.target.closest('button[data-res-id]');
      if (!button) return;
      const id = button.getAttribute('data-res-id');
      const marker = markerRefs[id];
      if (!marker) return;
      const latLng = marker.getLatLng();
      map.flyTo(latLng, MAP_ZOOM + 1, { duration: 0.6 });
      marker.openPopup();
    });
  }

  async function fetchHistory(elementoId, limit = 20) {
    try {
      // Usar API Service se disponível
      if (window.apiService && window.SENSORS && window.SENSORS[elementoId]) {
        const sensor = window.SENSORS[elementoId];
        const history = await window.apiService.getReadingHistory(
          sensor.sensor_id || elementoId,
          1, // 1 dia (últimas 24h)
          'distance_cm'
        );
        
        if (history && history.length > 0) {
          // Converter para formato de valores (últimos N registros)
          return history
            .slice(-limit)
            .map(item => {
              const value = Number(item.valor || item.value || 0);
              // Valor já vem em cm, não precisa dividir por 100
              return value;
            });
        }
      }
      
      // Tentar buscar leituras atuais como fallback
      if (window.apiService) {
        const readings = await window.apiService.getLatestReadings();
        if (readings && readings[elementoId] && readings[elementoId].distance_cm) {
          // Retornar array com valor atual repetido (melhor que dados simulados)
          return Array(limit).fill(readings[elementoId].distance_cm);
        }
      }
    } catch (error) {
      console.warn(`[Mapa] Erro ao buscar histórico de ${elementoId}:`, error);
    }

    // Fallback: dados simulados apenas se não houver API
    console.log(`[Mapa] Usando dados simulados para ${elementoId}`);
    const result = [];
    for (let i = 0; i < limit; i += 1) {
      result.push(
        Math.round(
          200 + Math.sin(i / 3 + elementoId.length) * 30 + Math.random() * 10
        )
      );
    }
    return result;
  }

  function buildPopup(id, name, canvasId) {
    return `
      <div style="min-width:260px">
        <strong>${name} (${id})</strong>
        <div class="popup-chart">
          <canvas id="${canvasId}" width="260" height="120" aria-label="Histórico de nível"></canvas>
        </div>
        <div class="popup-buttons">
          <button onclick="window.__aguada_showDetails('${id}')">Detalhes</button>
          <button onclick="window.__aguada_refresh('${id}')">Atualizar</button>
          <button onclick="window.__aguada_openPanel('${id}')">Abrir painel</button>
        </div>
      </div>
    `;
  }

  function mountMarkers(map, reservoirs) {
    reservoirs.forEach((reservoir, index) => {
      const id = reservoir.id;
      const name = reservoir.nome;
      const { latitude, longitude } = reservoir.coordenadas;

      const marker = L.circleMarker([latitude, longitude], {
        radius: 18,
        color: '#0d47a1',
        weight: 3,
        fillColor: '#4f83cc',
        fillOpacity: 0.6,
        className: `reservoir-marker reservoir-marker-${index}`,
      }).addTo(map);

      markerRefs[id] = marker;
      marker.bindTooltip(name, { permanent: true, direction: 'bottom', offset: [0, 14] });

      const canvasId = `chart-${id}`;
      marker.bindPopup(buildPopup(id, name, canvasId), { maxWidth: 320 });

      marker.on('popupopen', async () => {
        const history = await fetchHistory(id, 20);
        const ctx = document.getElementById(canvasId)?.getContext('2d');
        if (!ctx) return;
        if (window.__charts[canvasId]) {
          try {
            window.__charts[canvasId].destroy();
          } catch (error) {}
        }
        const labels = history.map((_, idx) => idx - history.length + 1);
        window.__charts[canvasId] = new Chart(ctx, {
          type: 'line',
          data: {
            labels,
            datasets: [
              {
                label: 'Nível (cm)',
                data: history,
                borderColor: '#2563eb',
                backgroundColor: 'rgba(37,99,235,0.15)',
                tension: 0.3,
                fill: true,
              },
            ],
          },
          options: {
            responsive: false,
            plugins: { legend: { display: false } },
            scales: {
              x: { display: false },
              y: { ticks: { font: { size: 10 } } },
            },
          },
        });
      });
    });
  }

  // Carregar reservatórios e inicializar mapa
  async function initializeMap() {
    // Aguardar API Service se disponível
    if (window.apiService === undefined) {
      let attempts = 0;
      while (!window.apiService && attempts < 50) {
        await new Promise(resolve => setTimeout(resolve, 100));
        attempts++;
      }
    }
    
    // Tentar carregar configuração de reservatórios
    const rawReservoirs = await loadConfig();
    
    // Se não houver config, usar SENSORS se disponível
    let reservoirsToUse = rawReservoirs;
    if (!reservoirsToUse && window.SENSORS) {
      reservoirsToUse = Object.entries(window.SENSORS).map(([id, sensor]) => ({
        id: id,
        nome: sensor.name || id,
        elemento_id: id
      }));
    }
    
    const reservoirs = normalizeReservoirs(reservoirsToUse);
    renderReservoirList(reservoirs);

    const map = ensureMap();
    wireToolbarActions(map);
    mountMarkers(map, reservoirs);
    
    // Atualizar marcadores com dados reais se disponível
    if (window.apiService) {
      updateMarkersWithRealData(map, reservoirs);
    }
  }
  
  // Atualizar marcadores com dados reais
  async function updateMarkersWithRealData(map, reservoirs) {
    try {
      const readings = await window.apiService.getLatestReadings();
      if (!readings) return;
      
      reservoirs.forEach(reservoir => {
        const id = reservoir.id;
        const marker = markerRefs[id];
        if (!marker) return;
        
        const reading = readings[id];
        if (!reading || reading.distance_cm === null || reading.distance_cm === undefined) return;
        
        // Calcular percentual de volume
        let volumePercent = 0;
        if (typeof window.getVolumePercent === 'function') {
          volumePercent = window.getVolumePercent(id, reading.distance_cm);
        } else {
          // Fallback simples
          volumePercent = Math.max(0, Math.min(100, 100 - (reading.distance_cm / 4)));
        }
        
        // Atualizar cor do marcador baseado no nível
        const color = volumePercent <= 20 ? '#ef4444' : 
                     volumePercent <= 50 ? '#f59e0b' : '#10b981';
        
        marker.setStyle({
          fillColor: color,
          color: color
        });
        
        // Atualizar tooltip com informações atuais
        const tooltipText = `${reservoir.nome}<br>Nível: ${volumePercent.toFixed(1)}%`;
        if (marker.getTooltip) {
          const tooltip = marker.getTooltip();
          if (tooltip) {
            tooltip.setContent(tooltipText);
          } else {
            marker.bindTooltip(tooltipText, { permanent: true, direction: 'bottom', offset: [0, 14] });
          }
        }
      });
    } catch (error) {
      console.warn('[Mapa] Erro ao atualizar marcadores com dados reais:', error);
    }
  }

  // Inicializar quando DOM estiver pronto
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initializeMap);
  } else {
    initializeMap();
  }

  document.querySelectorAll('a.map-link').forEach((anchor) => {
    anchor.href = MAP_LINK;
  });

  window.__aguada_showDetails = function (id) {
    alert(`Detalhes do reservatório ${id} (placeholder).`);
  };

  window.__aguada_refresh = function (id) {
    const marker = markerRefs[id];
    if (!marker) return;
    if (!marker.isPopupOpen || !marker.isPopupOpen()) {
      marker.openPopup();
    }
    marker.fire('popupopen');
  };

  window.__aguada_openPanel = function (id) {
    window.open(`/painel.html?elemento_id=${encodeURIComponent(id)}`, '_blank');
  };
})();
