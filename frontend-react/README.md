# AGUADA Frontend - Dashboard React

Dashboard moderno e responsivo para o Sistema AGUADA de Monitoramento Hidráulico.

## 🎯 Características

- ✅ **React 18** com TypeScript para type safety
- ✅ **Material-UI v5** para componentes profissionais
- ✅ **React Query** para gerenciamento de estado do servidor
- ✅ **Vite** para build ultrarrápido
- ✅ **Atualização automática** a cada 10 segundos
- ✅ **Design responsivo** para desktop, tablet e mobile

## 📊 Funcionalidades Implementadas

### Dashboard Principal
- ✅ Visualização dos 5 reservatórios (RCON, RCAV, RB03, IE01, IE02)
- ✅ Indicadores de nível com gauge visual
- ✅ Volume atual em m³ e litros
- ✅ Status de válvulas (entrada/saída)
- ✅ Detector de som (água entrando)
- ✅ Indicadores de sinal (RSSI) e bateria
- ✅ Timestamp de última atualização
- ✅ Código de cores baseado no nível:
  - 🟢 Verde: >30% (normal)
  - 🟡 Amarelo: 10-30% (baixo)
  - 🔴 Vermelho: <10% (crítico)

## 🚀 Instalação

### Pré-requisitos

- Node.js 18+
- npm 9+ ou yarn 1.22+
- Backend rodando em `http://192.168.0.100:3000`

### Setup

```bash
# Entrar no diretório
cd frontend-react

# Instalar dependências
npm install

# Copiar arquivo de ambiente
cp .env.example .env

# Editar .env se necessário (opcional)
nano .env

# Iniciar em modo desenvolvimento
npm run dev
```

O aplicativo estará disponível em `http://localhost:3001`

## 📁 Estrutura do Projeto

```
frontend-react/
├── public/                  # Arquivos estáticos
├── src/
│   ├── components/          # Componentes React
│   │   ├── Dashboard/       # Componentes do dashboard
│   │   │   └── ReservoirCard.tsx
│   │   ├── Charts/          # Gráficos (futuro)
│   │   ├── Alerts/          # Alertas (futuro)
│   │   └── Layout/          # Layout (futuro)
│   ├── pages/               # Páginas/rotas
│   │   └── DashboardPage.tsx
│   ├── services/            # Serviços de API
│   │   ├── api.ts
│   │   └── telemetry.service.ts
│   ├── hooks/               # Custom React hooks
│   │   └── useReservoirData.ts
│   ├── types/               # TypeScript types
│   │   ├── reservoir.types.ts
│   │   ├── telemetry.types.ts
│   │   └── api.types.ts
│   ├── utils/               # Funções utilitárias
│   │   ├── formatters.ts
│   │   └── calculations.ts
│   ├── App.tsx              # Componente principal
│   └── main.tsx             # Entry point
├── package.json
├── tsconfig.json
├── vite.config.ts
└── index.html
```

## 🔧 Comandos Disponíveis

```bash
# Desenvolvimento
npm run dev          # Inicia servidor de desenvolvimento (porta 3001)

# Build
npm run build        # Compila para produção

# Preview
npm run preview      # Preview da build de produção

# Testes (futuro)
npm run test         # Executa testes
```

## 🌐 Variáveis de Ambiente

Crie um arquivo `.env` baseado em `.env.example`:

```env
VITE_API_URL=http://192.168.0.100:3000/api
```

## 📡 Integração com Backend

O frontend consome os seguintes endpoints do backend:

- `GET /api/health` - Verificação de saúde
- `GET /api/readings/latest` - Últimas leituras dos sensores
- `GET /api/readings/history/:sensor_id` - Histórico de leituras
- `GET /api/sensors/status` - Status dos sensores

### Polling Automático

- **Leituras**: Atualiza a cada 10 segundos
- **Status dos sensores**: Atualiza a cada 30 segundos
- **Health check**: Atualiza a cada 60 segundos

## 🎨 Customização do Tema

O tema pode ser customizado em `src/App.tsx`:

```typescript
const theme = createTheme({
  palette: {
    primary: { main: '#1976d2' },
    success: { main: '#4caf50' },
    warning: { main: '#ff9800' },
    error: { main: '#f44336' },
  },
});
```

## 🐳 Docker

### Build da imagem

```bash
# Build
docker build -t aguada-frontend:latest .

# Run
docker run -d \
  --name aguada-frontend \
  -p 3001:80 \
  -e VITE_API_URL=http://seu-servidor:3000/api \
  aguada-frontend:latest
```

### Docker Compose

```yaml
version: '3.8'

services:
  frontend:
    build: ./frontend-react
    ports:
      - "3001:80"
    environment:
      - VITE_API_URL=http://backend:3000/api
    depends_on:
      - backend
```

## 📊 Próximas Funcionalidades (Roadmap)

### Fase 2: Gráficos e Histórico (em desenvolvimento)
- [ ] Gráfico de tendência de nível (24h, 7d, 30d)
- [ ] Gráfico de consumo por período
- [ ] Timeline de eventos
- [ ] Export de dados (CSV/Excel)

### Fase 3: Alertas (planejado)
- [ ] Lista de alertas ativos
- [ ] Filtros por tipo/reservatório
- [ ] Notificações visuais
- [ ] Som de alerta (opcional)

### Fase 4: Configurações (planejado)
- [ ] Gestão de sensores
- [ ] Mapeamento MAC → Reservatório
- [ ] Ajuste de thresholds
- [ ] Calibração de sensores

## 🔒 Segurança

- HTTPS em produção (configurar no nginx)
- CORS configurado no backend
- Validação de dados com TypeScript
- Sanitização de inputs

## 📝 Tecnologias Utilizadas

- **React 18** - Framework UI
- **TypeScript** - Type safety
- **Material-UI v5** - Componentes UI
- **React Query** - Server state management
- **Vite** - Build tool
- **Axios** - HTTP client
- **date-fns** - Date utilities
- **Recharts** - Gráficos (futuro)

## 🤝 Contribuindo

1. Fork o projeto
2. Crie uma branch para sua feature (`git checkout -b feature/nova-funcionalidade`)
3. Commit suas mudanças (`git commit -am 'Adiciona nova funcionalidade'`)
4. Push para a branch (`git push origin feature/nova-funcionalidade`)
5. Abra um Pull Request

## 📄 Licença

Este projeto está licenciado sob a MIT License.

## 📞 Suporte

Para dúvidas ou problemas:
- Abra uma issue no GitHub
- Consulte a documentação em `docs/FRONTEND_SPEC.md`

---

**Versão**: 1.0.0  
**Status**: ✅ Implementado - Dashboard funcional  
**Data**: 2025-11-18
