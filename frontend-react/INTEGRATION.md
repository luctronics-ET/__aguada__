# 🚀 Frontend React - AGUADA

Frontend React com TypeScript conectado ao backend real do sistema AGUADA.

## ✅ Implementado

### 📡 **Conexão com Backend**
- ✅ Serviço de API (`api.service.ts`) com axios
- ✅ Hooks personalizados para dados em tempo real (`useApi.ts`)
- ✅ Atualização automática via React Query (5-30s)
- ✅ Tratamento de erros e loading states

### 📄 **Páginas Conectadas**

#### 1. **Dashboard** (`/`)
- Lê dados de `/api/readings/latest`
- Exibe 5 reservatórios (RCON, RCAV, RB03, IE01, IE02)
- Calcula nível e volume baseado em `distance_cm`
- Status em tempo real (normal/warning/critical)
- Atualiza a cada **5 segundos**

#### 2. **Dados** (`/dados`)
- Lê de `/api/readings/raw`
- Tabela com últimas 100 leituras
- Filtros por status
- Estatísticas (total, normal, warning, critical)
- Atualiza a cada **10 segundos**

#### 3. **Alertas** (`/alerts`)
- Lê de `/api/alerts` e `/api/alerts/summary`
- Lista de alertas ativos
- Contadores por nível (crítico, aviso, info)
- Atualiza a cada **15 segundos**

#### 4. **Consumo** (`/consumo`)
- Lê de `/api/stats/consumption`
- Gráfico semanal com Recharts
- Estatísticas de consumo (dia, semana, mês)

#### 5. **Mapa** (`/mapa`)
- SVG com topologia da rede
- Status visual dos 5 reservatórios
- Conexões entre reservatórios

#### 6. **Sistema** (`/system`)
- Lê de `/api/system/health`, `/api/system/metrics`, `/api/system/logs`
- Status dos serviços (Backend, Database, MQTT, Gateway)
- Métricas de performance (uptime, CPU, memória)
- Logs em tempo real
- Atualiza a cada **10-30 segundos**

## 🔧 Configuração

### Variáveis de Ambiente (`.env`)
```env
VITE_API_URL=http://192.168.0.100:3000/api
VITE_WS_URL=ws://192.168.0.100:3000
VITE_REFRESH_INTERVAL=5000
```

### Instalação
```bash
npm install
```

### Desenvolvimento
```bash
npm run dev
# Abre em http://localhost:3001
```

### Build para Produção
```bash
npm run build
# Gera pasta dist/
```

## 📊 Fluxo de Dados

```
ESP32 Sensors → Gateway → MQTT/HTTP → Backend → PostgreSQL
                                          ↓
                                    REST API (/api/*)
                                          ↓
                               Frontend React (React Query)
                                          ↓
                                   UI Components
```

## 🔄 Atualização Automática

| Recurso | Endpoint | Intervalo |
|---------|----------|-----------|
| Dashboard | `/readings/latest` | 5s |
| Dados | `/readings/raw` | 10s |
| Alertas | `/alerts` | 15s |
| Sistema | `/system/metrics` | 10s |
| Sensores | `/sensors/status` | 30s |

## 🎨 Stack Tecnológica

- **React 18** - UI Framework
- **TypeScript** - Type Safety
- **Material-UI (MUI)** - Design System
- **React Query** - Data Fetching & Caching
- **React Router** - Navegação
- **Recharts** - Gráficos
- **Axios** - HTTP Client
- **Vite** - Build Tool

## 📱 Features

- ✅ **Responsive Design** - Mobile-first
- ✅ **Real-time Updates** - Auto-refresh
- ✅ **Loading States** - Spinners enquanto carrega
- ✅ **Error Handling** - Alertas de erro
- ✅ **Type-safe** - TypeScript em todos os componentes
- ✅ **Navbar** - Menu com drawer mobile
- ✅ **Dark Mode Ready** - Preparado para modo escuro

## 🐛 Debug

### Verificar conexão com backend
```bash
curl http://192.168.0.100:3000/api/health
# Deve retornar: {"status":"ok", ...}
```

### Verificar leituras
```bash
curl http://192.168.0.100:3000/api/readings/latest
# Deve retornar array de leituras
```

### Console do navegador
Abra DevTools (F12) e veja:
- **Network**: Requisições à API
- **Console**: Erros ou logs
- **React Query Devtools**: Estado do cache

## 📝 Próximos Passos (Opcional)

- [ ] WebSocket para updates em tempo real (sem polling)
- [ ] Autenticação JWT
- [ ] PWA (Progressive Web App)
- [ ] Service Worker para offline
- [ ] Notificações push
- [ ] Exportar dados para CSV/PDF
- [ ] Gráficos históricos (30 dias)

## 🔗 Endpoints Usados

```typescript
GET /api/readings/latest       // Dashboard
GET /api/readings/raw          // Página Dados
GET /api/alerts                // Página Alertas
GET /api/alerts/summary        // Contadores de alertas
GET /api/stats/consumption     // Página Consumo
GET /api/system/health         // Status serviços
GET /api/system/metrics        // Métricas sistema
GET /api/system/logs           // Logs tempo real
GET /api/sensors/status        // Status sensores
```

## ✨ Dados Reais

Todas as páginas agora exibem **dados reais** vindos do:
- PostgreSQL/TimescaleDB
- Sensores ESP32 via Gateway
- MQTT telemetria
- Backend Node.js/Express

**Nenhum dado mock!** Tudo conectado ao sistema real de monitoramento hidráulico.
