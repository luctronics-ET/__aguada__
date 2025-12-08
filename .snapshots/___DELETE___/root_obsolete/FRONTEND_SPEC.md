# Frontend para Sistema AGUADA - Dashboard de Monitoramento Hidráulico

## 📋 Visão Geral

Criar uma interface web moderna para visualização em tempo real dos dados de monitoramento dos 5 reservatórios de água do sistema AGUADA.

## 🎯 Objetivos

1. **Visualização em Tempo Real** - Dados atualizados dos sensores a cada 10 segundos
2. **Histórico e Análises** - Gráficos de tendências e consumo
3. **Alertas e Notificações** - Avisos de eventos críticos
4. **Gestão de Configurações** - Interface para ajustes do sistema

## 🏗️ Arquitetura Proposta

### Stack Tecnológica Recomendada

**Frontend Framework:** React 18+ com TypeScript
- ✅ Componentização reutilizável
- ✅ Type safety com TypeScript
- ✅ Grande ecossistema de bibliotecas
- ✅ Performance otimizada (Virtual DOM)

**UI Library:** Material-UI (MUI) v5
- ✅ Componentes prontos e responsivos
- ✅ Design system consistente
- ✅ Acessibilidade integrada
- ✅ Temas customizáveis

**State Management:** React Query + Zustand
- ✅ React Query para server state (cache, sincronização)
- ✅ Zustand para client state (UI, preferências do usuário)

**Gráficos:** Recharts ou Chart.js
- ✅ Gráficos de linha para tendências temporais
- ✅ Gauges circulares para níveis atuais
- ✅ Bar charts para comparação de consumo

**Comunicação:** Axios
- ✅ API REST via Axios
- ✅ Interceptors para autenticação
- ✅ Retry logic para resiliência

### Estrutura de Diretórios

```
frontend/
├── public/
│   ├── index.html
│   └── favicon.ico
├── src/
│   ├── components/          # Componentes reutilizáveis
│   │   ├── Dashboard/
│   │   │   ├── ReservoirCard.tsx
│   │   │   ├── ReservoirGauge.tsx
│   │   │   └── SystemOverview.tsx
│   │   ├── Charts/
│   │   │   ├── TrendChart.tsx
│   │   │   ├── ConsumptionChart.tsx
│   │   │   └── EventsTimeline.tsx
│   │   ├── Alerts/
│   │   │   ├── AlertList.tsx
│   │   │   └── AlertBadge.tsx
│   │   └── Layout/
│   │       ├── Header.tsx
│   │       ├── Sidebar.tsx
│   │       └── Footer.tsx
│   ├── pages/               # Páginas/rotas
│   │   ├── DashboardPage.tsx
│   │   ├── HistoryPage.tsx
│   │   ├── AlertsPage.tsx
│   │   └── SettingsPage.tsx
│   ├── services/            # Comunicação com API
│   │   ├── api.ts
│   │   ├── telemetry.service.ts
│   │   └── auth.service.ts
│   ├── hooks/               # Custom React hooks
│   │   ├── useReservoirData.ts
│   │   ├── useRealTimeUpdates.ts
│   │   └── useAlerts.ts
│   ├── types/               # TypeScript types
│   │   ├── reservoir.types.ts
│   │   ├── telemetry.types.ts
│   │   └── api.types.ts
│   ├── utils/               # Utilitários
│   │   ├── formatters.ts
│   │   └── calculations.ts
│   ├── App.tsx
│   └── index.tsx
├── package.json
├── tsconfig.json
└── README.md
```

## 📊 Funcionalidades Principais

### 1. Dashboard Principal

**Tela de Visão Geral:**
- 5 cards representando cada reservatório (RCON, RCAV, RB03, IE01, IE02)
- Para cada reservatório exibir:
  - **Gauge visual** do nível atual (0-100%)
  - **Capacidade atual** em litros e m³
  - **Status de válvulas** (entrada/saída - aberta/fechada)
  - **Detector de som** (água entrando - sim/não)
  - **Última atualização** (timestamp relativo)
  - **Qualidade do sinal** (RSSI em dBm)
  - **Status da bateria** (voltagem)

**Código de Cores:**
- 🟢 **Verde**: Nível normal (>30%)
- 🟡 **Amarelo**: Nível baixo (10-30%)
- 🔴 **Vermelho**: Nível crítico (<10%)
- 🔵 **Azul**: Em abastecimento (som detectado + nível subindo)

**Informações do Sistema:**
- Status de conexão dos 5 nodes ESP32 (online/offline)
- Gateway online/offline
- Última leitura recebida (timestamp)
- Taxa de pacotes (enviados/falhados)

### 2. Gráficos e Histórico

**Gráfico de Tendência:**
- Seletor de período: 24h / 7 dias / 30 dias / customizado
- Linha temporal do nível de cada reservatório
- Zoom e pan interativos
- Tooltips com valores exatos ao passar o mouse
- Marcadores de eventos (abastecimento, vazamento, alertas)

**Gráfico de Consumo:**
- Volume consumido por período
- Comparação entre reservatórios
- Padrões de consumo (hora do dia, dia da semana)
- Taxa de consumo (litros/hora)

**Análise de Eventos:**
- Timeline de eventos detectados:
  - Abastecimentos (início, fim, volume adicionado)
  - Consumos anormais
  - Vazamentos detectados
  - Falhas de sensor/comunicação

### 3. Alertas e Notificações

**Lista de Alertas Ativos:**
- Nível crítico (<10%)
- Vazamento detectado (consumo > -15L/h por >1h)
- Sensor offline (>5 min sem dados)
- Sinal fraco (RSSI < -80 dBm)
- Bateria baixa (<4V)
- Nível CAV crítico (<70%)

**Funcionalidades:**
- Filtros por tipo de alerta
- Filtros por reservatório
- Filtros por período
- Ordenação por severidade/data
- Badge de contagem de alertas ativos
- Som de alerta para críticos (opcional, configurável)

### 4. Configurações

**Gestão de Sensores:**
- Tabela com todos os sensores
- Mapeamento MAC address → Reservatório
- Calibração de sensores (offset)
- Histórico de calibrações

**Thresholds e Limites:**
- Deadband (padrão: 2cm)
- Níveis de alerta (warning/critical)
- Timeouts de comunicação
- Parâmetros de detecção de eventos

**Usuários e Permissões (opcional):**
- Lista de usuários
- Roles: admin, operador, visualizador
- Logs de auditoria

## 🔌 Integração com Backend

### API Endpoints Disponíveis

```typescript
// GET /api/health
interface HealthResponse {
  status: 'ok' | 'error';
  timestamp: string;
  service: string;
  version: string;
}

// GET /api/readings/latest
interface LatestReadings {
  [sensor_id: string]: {
    sensor_id: string;
    elemento_id: string;  // RCON, RCAV, RB03, IE01, IE02
    variavel: string;     // distance_cm, valve_in, valve_out, sound_in
    valor: number;        // Valor da leitura
    unidade: string;      // cm, boolean
    datetime: string;     // ISO 8601
    meta: {
      battery_mv?: number;
      rssi_dbm?: number;
      uptime_sec?: number;
      node_mac?: string;
    };
  };
}

// GET /api/readings/history/:sensor_id?start=&end=
interface ReadingHistory {
  readings: Array<{
    datetime: string;
    valor: number;
    unidade: string;
  }>;
  total: number;
}

// GET /api/sensors/status
interface SensorsStatus {
  sensors: Array<{
    sensor_id: string;
    elemento_id: string;
    node_mac: string;
    status: 'online' | 'offline';
    last_reading: string;
    signal_strength: number;
  }>;
}

// GET /api/readings/daily-summary
interface DailySummary {
  reservoirs: Array<{
    elemento_id: string;
    min_nivel: number;
    max_nivel: number;
    avg_nivel: number;
    volume_consumido: number;
  }>;
}
```

### Exemplo de Implementação

```typescript
// src/services/api.ts
import axios from 'axios';

const api = axios.create({
  baseURL: process.env.REACT_APP_API_URL || 'http://192.168.0.100:3000/api',
  timeout: 5000,
});

// Interceptor para tratamento de erros
api.interceptors.response.use(
  response => response,
  error => {
    console.error('API Error:', error);
    return Promise.reject(error);
  }
);

export default api;

// src/services/telemetry.service.ts
import api from './api';

export const telemetryService = {
  async getLatestReadings() {
    const response = await api.get('/readings/latest');
    return response.data;
  },
  
  async getReadingHistory(
    sensorId: string, 
    start: Date, 
    end: Date
  ) {
    const response = await api.get(`/readings/history/${sensorId}`, {
      params: {
        start: start.toISOString(),
        end: end.toISOString(),
      },
    });
    return response.data;
  },
  
  async getSensorsStatus() {
    const response = await api.get('/sensors/status');
    return response.data;
  },
  
  async getDailySummary() {
    const response = await api.get('/readings/daily-summary');
    return response.data;
  },
};

// src/hooks/useReservoirData.ts
import { useQuery } from '@tanstack/react-query';
import { telemetryService } from '../services/telemetry.service';

export function useReservoirData() {
  return useQuery({
    queryKey: ['latest-readings'],
    queryFn: () => telemetryService.getLatestReadings(),
    refetchInterval: 10000, // Poll a cada 10 segundos
    staleTime: 5000,
    retry: 3,
  });
}

// src/hooks/useReadingHistory.ts
export function useReadingHistory(
  sensorId: string,
  start: Date,
  end: Date
) {
  return useQuery({
    queryKey: ['reading-history', sensorId, start, end],
    queryFn: () => telemetryService.getReadingHistory(sensorId, start, end),
    enabled: !!sensorId,
  });
}
```

## 🎨 Design de Interface

### Layout Principal (Desktop)

```
┌─────────────────────────────────────────────────────────────────┐
│  🌊 AGUADA  │  📊 Dashboard  │  📈 Histórico  │  🔔 Alertas  │ ⚙️ Config │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐
│  │  RCON   │  │  RCAV   │  │  RB03   │  │  IE01   │  │  IE02   │
│  │ ┌─────┐ │  │ ┌─────┐ │  │ ┌─────┐ │  │ ┌─────┐ │  │ ┌─────┐ │
│  │ │ 85% │ │  │ │ 72% │ │  │ │ 45% │ │  │ │ 91% │ │  │ │ 88% │ │
│  │ │ 🟢  │ │  │ │ 🟢  │ │  │ │ 🟡  │ │  │ │ 🟢  │ │  │ │ 🟢  │ │
│  │ └─────┘ │  │ └─────┘ │  │ └─────┘ │  │ └─────┘ │  │ └─────┘ │
│  │         │  │         │  │         │  │         │  │         │
│  │ 68.0 m³ │  │ 57.6 m³ │  │ 36.0 m³ │  │ 231.2 m³│  │ 223.5 m³│
│  │ 68,000L │  │ 57,600L │  │ 36,000L │  │ 231,200L│  │ 223,500L│
│  │         │  │         │  │         │  │         │  │         │
│  │ ✓ Entrada│  │ ✗ Entrada│  │ ✓ Entrada│  │ ✓ Entrada│  │ ✓ Entrada│
│  │ ✗ Saída │  │ ✓ Saída │  │ ✓ Saída │  │ ✗ Saída │  │ ✗ Saída │
│  │ 🔊 Não  │  │ 🔊 Sim  │  │ 🔊 Não  │  │ 🔊 Não  │  │ 🔊 Não  │
│  │         │  │         │  │         │  │         │  │         │
│  │ 📶 -45dBm│  │ 📶 -52dBm│  │ 📶 -48dBm│  │ 📶 -50dBm│  │ 📶 -47dBm│
│  │ 🔋 5.0V │  │ 🔋 5.0V │  │ 🔋 5.0V │  │ 🔋 5.0V │  │ 🔋 5.0V │
│  │         │  │         │  │         │  │         │  │         │
│  │ 2s atrás│  │ 1s atrás│  │ 3s atrás│  │ 2s atrás│  │ 1s atrás│
│  └─────────┘  └─────────┘  └─────────┘  └─────────┘  └─────────┘
│                                                                   │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  📈 Tendência de Nível (Últimas 24 horas)                 │  │
│  │  [Seletor: 24h | 7d | 30d | Customizado]                  │  │
│  │                                                             │  │
│  │  100% ┤                                                     │  │
│  │   75% ┤     ╱╲    ╱╲                                       │  │
│  │   50% ┤    ╱  ╲  ╱  ╲╱╲                                   │  │
│  │   25% ┤╲╱╲╱    ╲╱      ╲                                   │  │
│  │    0% └─────────────────────────────────────────────────   │  │
│  │       00:00   06:00   12:00   18:00   00:00               │  │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

### Paleta de Cores

```
Primary (Azul Água):    #1976d2
Secondary (Azul Escuro): #0d47a1
Success (Verde):        #4caf50
Warning (Laranja):      #ff9800
Error (Vermelho):       #f44336
Info (Azul Claro):      #2196f3

Background:             #fafafa
Surface:                #ffffff
Text Primary:           #212121
Text Secondary:         #757575
```

## 🚀 Roadmap de Implementação

### Fase 1: Setup e Estrutura (1 semana)

**Tarefas:**
- [ ] Configurar Create React App com TypeScript
- [ ] Instalar dependências (MUI, React Query, Axios, Chart.js)
- [ ] Criar estrutura de pastas conforme arquitetura
- [ ] Configurar rotas (React Router v6)
- [ ] Setup de API client (Axios com interceptors)
- [ ] Criar layout base (Header, Sidebar, Content Area)
- [ ] Configurar tema MUI customizado

**Entregável:** Aplicação rodando com layout base e navegação

### Fase 2: Dashboard Principal (1 semana)

**Tarefas:**
- [ ] Criar componente `ReservoirCard`
- [ ] Criar componente `ReservoirGauge` (gauge circular)
- [ ] Implementar hook `useReservoirData`
- [ ] Integração com API `/api/readings/latest`
- [ ] Polling automático a cada 10 segundos
- [ ] Indicadores visuais de status (cores)
- [ ] Formatação de valores (litros, m³, percentual)
- [ ] Indicador de última atualização

**Entregável:** Dashboard funcional exibindo dados em tempo real dos 5 reservatórios

### Fase 3: Gráficos e Histórico (1 semana)

**Tarefas:**
- [ ] Criar componente `TrendChart`
- [ ] Implementar seletor de período (24h, 7d, 30d, customizado)
- [ ] Integração com `/api/readings/history`
- [ ] Implementar zoom e pan
- [ ] Tooltips interativos
- [ ] Legenda com cores por reservatório
- [ ] Export de dados (CSV/Excel)

**Entregável:** Página de histórico com gráficos interativos

### Fase 4: Alertas e Notificações (3 dias)

**Tarefas:**
- [ ] Criar componente `AlertList`
- [ ] Criar componente `AlertBadge` (contador)
- [ ] Filtros por tipo/reservatório/período
- [ ] Integração com lógica de detecção de eventos do backend
- [ ] Notificações visuais (badges, cores)
- [ ] Som de alerta (opcional, configurável)
- [ ] Marcar alertas como lidos

**Entregável:** Sistema de alertas funcional com notificações

### Fase 5: Configurações (4 dias)

**Tarefas:**
- [ ] Tela de gestão de sensores
- [ ] Tabela com MAC → Reservatório
- [ ] Formulário de calibração
- [ ] Ajuste de thresholds
- [ ] Persistência de configurações

**Entregável:** Interface de configuração completa

### Fase 6: Otimização e Testes (3 dias)

**Tarefas:**
- [ ] Testes unitários (Jest + React Testing Library)
- [ ] Testes de integração
- [ ] Performance optimization (lazy loading, memoization)
- [ ] Responsividade mobile (media queries)
- [ ] Tratamento de erros e loading states
- [ ] Documentação (README, JSDoc)

**Entregável:** Aplicação otimizada, testada e documentada

## 📦 Instalação e Setup

### Pré-requisitos

- Node.js 18+ 
- npm 9+ ou yarn 1.22+
- Backend rodando em `http://192.168.0.100:3000`

### Criação do Projeto

```bash
# Criar projeto React com TypeScript
npx create-react-app aguada-frontend --template typescript
cd aguada-frontend

# Instalar dependências principais
npm install @mui/material @emotion/react @emotion/styled
npm install @mui/icons-material
npm install @tanstack/react-query
npm install axios
npm install react-router-dom
npm install recharts
npm install zustand
npm install date-fns

# Instalar dependências de desenvolvimento
npm install -D @types/react-router-dom
npm install -D @testing-library/react @testing-library/jest-dom

# Criar arquivo .env
echo "REACT_APP_API_URL=http://192.168.0.100:3000/api" > .env.local

# Iniciar desenvolvimento
npm start
```

### Estrutura de package.json

```json
{
  "name": "aguada-frontend",
  "version": "1.0.0",
  "description": "Dashboard para Sistema AGUADA de Monitoramento Hidráulico",
  "dependencies": {
    "@emotion/react": "^11.11.0",
    "@emotion/styled": "^11.11.0",
    "@mui/icons-material": "^5.14.0",
    "@mui/material": "^5.14.0",
    "@tanstack/react-query": "^5.0.0",
    "axios": "^1.6.0",
    "date-fns": "^2.30.0",
    "react": "^18.2.0",
    "react-dom": "^18.2.0",
    "react-router-dom": "^6.20.0",
    "recharts": "^2.10.0",
    "zustand": "^4.4.0"
  },
  "devDependencies": {
    "@testing-library/jest-dom": "^6.1.0",
    "@testing-library/react": "^14.1.0",
    "@types/react": "^18.2.0",
    "@types/react-dom": "^18.2.0",
    "typescript": "^5.3.0"
  },
  "scripts": {
    "start": "react-scripts start",
    "build": "react-scripts build",
    "test": "react-scripts test",
    "eject": "react-scripts eject"
  }
}
```

## 🐳 Docker e Deploy

### Dockerfile

```dockerfile
# Build stage
FROM node:18-alpine AS build

WORKDIR /app

COPY package*.json ./
RUN npm ci --only=production

COPY . .
RUN npm run build

# Production stage
FROM nginx:alpine

COPY --from=build /app/build /usr/share/nginx/html
COPY nginx.conf /etc/nginx/conf.d/default.conf

EXPOSE 80

CMD ["nginx", "-g", "daemon off;"]
```

### nginx.conf

```nginx
server {
    listen 80;
    server_name localhost;
    
    root /usr/share/nginx/html;
    index index.html;
    
    # Gzip compression
    gzip on;
    gzip_types text/plain text/css application/json application/javascript;
    
    # SPA routing
    location / {
        try_files $uri $uri/ /index.html;
    }
    
    # API proxy (opcional - se backend no mesmo servidor)
    location /api {
        proxy_pass http://backend:3000;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host $host;
        proxy_cache_bypass $http_upgrade;
    }
    
    # Cache static assets
    location ~* \.(js|css|png|jpg|jpeg|gif|ico|svg)$ {
        expires 1y;
        add_header Cache-Control "public, immutable";
    }
}
```

### docker-compose.yml (Stack completa)

```yaml
version: '3.8'

services:
  frontend:
    build: ./frontend
    ports:
      - "3001:80"
    environment:
      - REACT_APP_API_URL=http://localhost:3000/api
    depends_on:
      - backend
    restart: unless-stopped

  backend:
    build: ./backend
    ports:
      - "3000:3000"
    environment:
      - DB_HOST=postgres
      - DB_PORT=5432
      - DB_NAME=aguada
      - REDIS_HOST=redis
      - NODE_ENV=production
    depends_on:
      - postgres
      - redis
    restart: unless-stopped

  postgres:
    image: timescale/timescaledb:latest-pg15
    ports:
      - "5432:5432"
    environment:
      - POSTGRES_DB=aguada
      - POSTGRES_USER=aguada
      - POSTGRES_PASSWORD=aguada123
    volumes:
      - postgres_data:/var/lib/postgresql/data
      - ./database/schema.sql:/docker-entrypoint-initdb.d/schema.sql
    restart: unless-stopped

  redis:
    image: redis:7-alpine
    ports:
      - "6379:6379"
    restart: unless-stopped

volumes:
  postgres_data:
```

### Deploy em Produção

```bash
# Build da imagem
docker build -t aguada-frontend:latest ./frontend

# Executar
docker run -d \
  --name aguada-frontend \
  -p 80:80 \
  -e REACT_APP_API_URL=http://seu-servidor:3000/api \
  aguada-frontend:latest

# Ou usando docker-compose
docker-compose up -d
```

## 🔒 Segurança

### Checklist de Segurança

- [ ] **HTTPS** em produção (certificado SSL/TLS)
- [ ] **CORS** configurado corretamente no backend
- [ ] **CSP Headers** (Content Security Policy)
- [ ] **Rate Limiting** no backend
- [ ] **Input Sanitization** (XSS protection)
- [ ] **Environment Variables** para configurações sensíveis
- [ ] **Autenticação JWT** (se necessário)
- [ ] **RBAC** - Role-Based Access Control (se multi-usuário)
- [ ] **Audit Logs** para ações administrativas

### Exemplo de CSP Header

```
Content-Security-Policy: 
  default-src 'self'; 
  script-src 'self' 'unsafe-inline'; 
  style-src 'self' 'unsafe-inline'; 
  img-src 'self' data: https:; 
  connect-src 'self' http://192.168.0.100:3000;
```

## 📊 Monitoramento e Analytics

### Métricas Recomendadas

- **Performance:**
  - Lighthouse Score (>90)
  - First Contentful Paint (<1.5s)
  - Time to Interactive (<3s)
  - Bundle size (<200KB gzipped)

- **Uptime:**
  - Disponibilidade >99%
  - Tempo de resposta da API <200ms

- **Erros:**
  - Taxa de erro <0.1%
  - Error tracking com Sentry

### Ferramentas

- **Google Analytics** ou **Plausible** (privacy-focused)
- **Sentry** para error tracking
- **Lighthouse CI** para monitoramento de performance
- **UptimeRobot** para monitoring de disponibilidade

## ✅ Critérios de Aceitação

### Funcionalidades

- [ ] Dashboard carrega em <2 segundos
- [ ] Dados atualizam automaticamente a cada 10 segundos
- [ ] Exibe corretamente os 5 reservatórios
- [ ] Gráficos interativos funcionam (zoom, pan, tooltips)
- [ ] Alertas são exibidos e atualizados
- [ ] Configurações podem ser alteradas e persistidas

### Qualidade

- [ ] Interface responsiva (desktop 1920x1080, tablet 768x1024, mobile 375x667)
- [ ] Funciona em Chrome 90+, Firefox 88+, Safari 14+
- [ ] Sem erros no console
- [ ] Testes com >80% de cobertura
- [ ] Documentação completa (README, comentários)

### Performance

- [ ] Lighthouse Score >90
- [ ] Bundle size <200KB (gzipped)
- [ ] Tempo de carregamento inicial <2s
- [ ] Smooth animations (60fps)

## 📚 Referências Técnicas

- [React Documentation](https://react.dev/)
- [TypeScript Handbook](https://www.typescriptlang.org/docs/)
- [Material-UI Components](https://mui.com/components/)
- [React Query Guide](https://tanstack.com/query/latest)
- [Recharts Examples](https://recharts.org/en-US/examples)
- [Axios Documentation](https://axios-http.com/docs/intro)

## 📞 Suporte e Contribuição

Para dúvidas, sugestões ou problemas:
1. Abra uma **issue** no repositório
2. Use as labels apropriadas (`frontend`, `bug`, `enhancement`)
3. Forneça detalhes (screenshots, logs, passos para reproduzir)

Para contribuir:
1. Fork o repositório
2. Crie uma branch (`git checkout -b feature/nova-funcionalidade`)
3. Commit suas mudanças
4. Abra um Pull Request

---

**Autor:** Sistema AGUADA Team  
**Versão:** 1.0.0  
**Data:** 2025-11-18  
**Status:** Especificação Aprovada - Pronta para Implementação
