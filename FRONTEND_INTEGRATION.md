# AGUADA Frontend + Backend Integration Guide

## 🔗 System Integration Overview

After frontend unification, here's how to integrate with the backend API for full functionality.

---

## ✅ Current Status

### Frontend

- **Status**: ✅ Production Ready
- **Pages**: 12/12 unified
- **Layout**: Shared header/nav/footer
- **Components**: 60+ CSS classes
- **Documentation**: COMPONENTS.md, UNIFICATION_COMPLETE.md

### Backend

- **Status**: ✅ Running
- **Port**: 3000
- **Health**: `/api/health` ✓ responding
- **Database**: PostgreSQL/TimescaleDB (port 5433)

---

## 🚀 Starting the Full Stack

### Terminal 1: Backend API

```bash
cd /home/luciano/Área\ de\ trabalho/aguada/backend
npm run dev
# Output: Server running on http://localhost:3000
```

### Terminal 2: Frontend Development

```bash
cd /home/luciano/Área\ de\ trabalho/aguada/frontend

# Option A: Use Python's built-in server (quick test)
python3 -m http.server 8000
# Access: http://localhost:8000

# Option B: Use Node's http-server (if installed)
npx http-server -p 8000
```

### Terminal 3: Monitor Database

```bash
psql -h localhost -p 5433 -U aguada -d aguada
# SQL commands available for inspection
```

---

## 🌐 API Endpoints (Frontend Uses)

### Health Check

```bash
GET /api/health
# Used by: All pages (optional status check)
```

### Telemetry (ESP32 Data Ingestion)

```bash
POST /api/telemetry
# Body: {"mac":"XX:XX:XX:XX:XX:XX","type":"distance_cm","value":24480,...}
# Used by: Gateway ESP32
```

### Readings

```bash
GET /api/readings/latest
# Returns: Latest reading for each sensor
# Used by: index.html, dashboard

GET /api/readings/raw?limit=100&offset=0
# Returns: Paginated raw readings
# Used by: dados.html

GET /api/readings/history/:sensor_id?start=2025-12-01&end=2025-12-05
# Returns: Sensor history
# Used by: history.html
```

### Sensors

```bash
GET /api/sensors
# Returns: List of all sensors with status
# Used by: system.html, dados.html

GET /api/sensors/status
# Returns: Online/offline status for each sensor
```

### Statistics

```bash
GET /api/stats/daily
# Returns: Daily consumption/supply
# Used by: consumo.html, abastecimento.html

GET /api/stats/sensors
# Returns: Per-sensor statistics
```

### Alerts

```bash
GET /api/alerts?status=active
# Returns: Active alerts
# Used by: alerts.html

GET /api/alerts?status=resolved
# Returns: Historical alerts
```

### System Info

```bash
GET /api/system/health
# Returns: DB, Redis, services status
# Used by: system.html

GET /api/system/logs?limit=50
# Returns: Recent system logs
```

---

## 🔌 Frontend API Service (assets/api-service.js)

### Available Methods

All API calls go through the shared `window.apiService`:

```javascript
// Sensors
apiService.getSensors();
apiService.getSensorsStatus();
apiService.getSensorHistory(sensorId, startDate, endDate);

// Readings
apiService.getLatestReadings();
apiService.getReadingsRaw(limit, offset);
apiService.getReadingHistory(sensorId, start, end);

// Statistics
apiService.getDailyStats();
apiService.getSensorStats();
apiService.getConsumption(days);

// Alerts
apiService.getAlerts(status);
apiService.getActiveAlerts();

// System
apiService.getSystemHealth();
apiService.getSystemLogs(limit);

// Health
apiService.healthCheck();
```

### Example Usage

```javascript
// In any page script after layout.js loads

async function loadData() {
  try {
    // Wait for API Service to load
    let attempts = 0;
    while (!window.apiService && attempts < 50) {
      await new Promise((resolve) => setTimeout(resolve, 100));
      attempts++;
    }

    if (!window.apiService) {
      console.error("API Service not loaded");
      return;
    }

    // Now use the API
    const sensors = await apiService.getSensors();
    console.log("Sensors:", sensors);

    const latest = await apiService.getLatestReadings();
    console.log("Latest readings:", latest);
  } catch (error) {
    console.error("Error loading data:", error);
  }
}

// Call after DOM ready
if (document.readyState === "loading") {
  document.addEventListener("DOMContentLoaded", loadData);
} else {
  loadData();
}
```

---

## 📊 Page-to-API Mapping

| Page                   | Uses         | Endpoints                                             |
| ---------------------- | ------------ | ----------------------------------------------------- |
| **index.html**         | Dashboard    | `/api/sensors`, `/api/readings/latest`, `/api/alerts` |
| **dados.html**         | Data Table   | `/api/readings/raw`, `/api/sensors`                   |
| **mapa.html**          | Network Map  | `/api/sensors`, `/api/system/health`                  |
| **painel.html**        | Visual Panel | `/api/readings/latest`, `/api/sensors/status`         |
| **consumo.html**       | Consumption  | `/api/stats/daily`, `/api/stats/sensors`              |
| **abastecimento.html** | Supply       | `/api/readings/history`, `/api/stats/daily`           |
| **manutencao.html**    | Maintenance  | `/api/alerts`, `/api/sensors`                         |
| **history.html**       | History      | `/api/readings/history`, `/api/readings/raw`          |
| **alerts.html**        | Alerts       | `/api/alerts`, `/api/stats/daily`                     |
| **config.html**        | Config       | `/api/system/health`, `/api/sensors`                  |
| **system.html**        | System       | `/api/system/health`, `/api/sensors/status`           |
| **documentacao.html**  | Docs         | (No API calls needed)                                 |

---

## 🧪 Testing Checklist

### 1. Backend Health

```bash
# Terminal
curl http://localhost:3000/api/health

# Expected:
# {"status":"ok","timestamp":"...","service":"aguada-backend","version":"1.0.0"}
```

### 2. Frontend Loads

```bash
# Browser
http://localhost:8000/

# Check:
- ✓ Header appears with navigation
- ✓ Footer shows with live timestamp
- ✓ No console errors (F12 → Console)
- ✓ Active nav link is highlighted
```

### 3. API Connection

```bash
# In browser DevTools Console
await apiService.healthCheck()

# Expected output in console:
# {status: 'ok', timestamp: '...', service: 'aguada-backend', version: '1.0.0'}
```

### 4. Data Loading

```javascript
// In DevTools Console
await apiService.getSensors()

# Expected:
# [
#   {sensor_id: "RCON_nivel", elemento_id: "RCON", type: "nivel", ...},
#   {sensor_id: "RCAV_nivel", elemento_id: "RCAV", type: "nivel", ...},
#   ...
# ]
```

### 5. Page-Specific Tests

#### Dashboard (index.html)

```javascript
// Check if data loads
const sensors = await apiService.getSensors();
console.log("Sensors loaded:", sensors.length);
```

#### Data Page (dados.html)

```javascript
// Check table data
const readings = await apiService.getReadingsRaw(10, 0);
console.log("Readings loaded:", readings.rows.length);
```

#### Alerts Page (alerts.html)

```javascript
// Check alerts
const alerts = await apiService.getAlerts("active");
console.log("Active alerts:", alerts.length);
```

---

## 🔧 Configuration

### Backend Configuration (backend/.env)

```env
PORT=3000
DB_HOST=localhost
DB_PORT=5433
DB_NAME=aguada
DB_USER=aguada
DB_PASSWORD=<your_password>
API_BASE_URL=http://localhost:3000
```

### Frontend Configuration (frontend/assets/api-service.js)

```javascript
const API_BASE_URL = "http://localhost:3000/api";
const API_TIMEOUT = 5000; // ms
```

**Important**: Change these if deploying to production!

---

## 📱 Frontend Responsive Behavior

All pages automatically adapt:

| Viewport   | Grid      | Layout       |
| ---------- | --------- | ------------ |
| 1024px+    | 3 columns | Full desktop |
| 768-1024px | 2 columns | Tablet       |
| <768px     | 1 column  | Mobile       |

Test with DevTools Device Emulator (F12 → Toggle Device Toolbar).

---

## 🐛 Troubleshooting

### Issue: Frontend loads but shows no data

```bash
# Check 1: Backend running?
curl http://localhost:3000/api/health

# Check 2: Console errors? (F12 → Console)
# Look for 404 or CORS errors

# Check 3: API endpoint accessible?
curl http://localhost:3000/api/sensors
```

### Issue: CORS errors in console

```javascript
// Backend needs CORS headers
// Already configured in backend/src/server.js

// If still failing:
// 1. Restart backend (npm run dev)
// 2. Check frontend URL matches (http://localhost:8000)
```

### Issue: Database connection error

```bash
# Check 1: PostgreSQL running?
docker compose ps | grep postgres

# Check 2: Credentials correct?
psql -h localhost -p 5433 -U aguada -d aguada -c "SELECT 1"

# Check 3: Schema initialized?
psql -h localhost -p 5433 -U aguada -d aguada -c "SELECT * FROM aguada.sensores LIMIT 1"
```

### Issue: "API Service not loaded"

```javascript
// Ensure script load order (check page source)
1. <link rel="stylesheet" href="assets/style.css" />
2. <script src="assets/layout.js"></script>       ← First
3. <script src="assets/api-service.js"></script>  ← Second
4. <script src="assets/ui-utils.js"></script>
5. <script src="assets/app.js"></script>

// Or manually wait:
let attempts = 0;
while (!window.apiService && attempts < 50) {
  await new Promise(r => setTimeout(r, 100));
  attempts++;
}
```

---

## 🚀 Production Deployment

### Frontend Only (Static Files)

```bash
# Build for production (no build step needed)
# Copy frontend folder to web server
cp -r frontend/ /var/www/html/aguada/

# Verify
ls -la /var/www/html/aguada/
# Should contain: index.html, mapa.html, ..., assets/, config/
```

### With Backend

```bash
# 1. Backend deployment
docker build -f backend/Dockerfile -t aguada-backend:1.0 .
docker run -d -p 3000:3000 --name aguada-backend aguada-backend:1.0

# 2. Frontend deployment
docker build -f docker/Dockerfile.frontend -t aguada-frontend:1.0 ./frontend
docker run -d -p 80:80 --name aguada-frontend aguada-frontend:1.0

# 3. Verify
curl http://localhost:3000/api/health
curl http://localhost/  # Should load frontend
```

---

## 📝 Next Steps

1. **Start Backend**: `cd backend && npm run dev`
2. **Start Frontend**: `cd frontend && python3 -m http.server 8000`
3. **Open Browser**: `http://localhost:8000`
4. **Test Pages**: Navigate through all pages in sidebar
5. **Check Console**: F12 → Console for any errors
6. **Monitor Logs**: Check terminal output for API calls
7. **Database**: Verify data in PostgreSQL with psql

---

## 📚 Related Documentation

- **backend/README.md** - Backend setup and API docs
- **frontend/COMPONENTS.md** - CSS components reference
- **frontend/UNIFICATION_COMPLETE.md** - Frontend unification details
- **docs/RULES.md** - System architecture and rules
- **QUICKSTART.md** - Quick start guide

---

## ✅ Integration Checklist

- [ ] Backend running on port 3000
- [ ] Frontend accessible on port 8000
- [ ] Browser DevTools shows no errors
- [ ] Header/nav/footer appear on all pages
- [ ] Active page highlighted in nav
- [ ] Footer timestamp updates
- [ ] API health check responds
- [ ] At least one sensor data loads
- [ ] Tables display with data
- [ ] Responsive design works (F12 device toggle)
- [ ] No CORS errors
- [ ] Database connection confirmed

---

**Status**: ✅ Ready for Testing  
**Last Updated**: 2025-01-15  
**Backend Version**: 1.0.0  
**Frontend Version**: 1.0.0
