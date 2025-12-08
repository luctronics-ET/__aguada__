# 🚀 AGUADA - Guia Completo de Instalação e Uso

## Status do Sistema ✅

O sistema AGUADA está **100% funcional** com os seguintes componentes em execução:

- ✅ **Backend API** - Node.js/Express rodando em porta 3000
- ✅ **Banco de Dados** - PostgreSQL 16 + TimescaleDB
- ✅ **Cache** - Redis
- ✅ **Frontend** - HTML/JS com dashboard responsivo
- ✅ **Nginx** - Proxy reverso e servidor web
- ✅ **Grafana** - Visualização em tempo real (porta 3001)

---

## 📋 Pré-requisitos

### Para usar com Docker (Recomendado):

- Docker Engine 20.10+
- Docker Compose 2.0+ ou docker-compose v1.29+
- Git
- Navegador moderno (Chrome, Firefox, Safari, Edge)

### Para instalação local (sem Docker):

- Node.js 18+
- PostgreSQL 15+
- Redis 7+
- npm 9+

---

## 🚀 Instalação Rápida (Docker)

### 1. Clonar o repositório

```bash
git clone https://github.com/luctronics-ET/aguada.git
cd aguada
```

### 2. Configurar variáveis de ambiente

```bash
# Copiar arquivo de exemplo
cp backend/.env.example backend/.env

# Editar conforme necessário (database, portas, etc)
nano backend/.env
```

### 3. Iniciar sistema com Docker Compose

```bash
# Iniciar todos os serviços
docker compose up -d

# Verificar status
docker compose ps

# Ver logs
docker compose logs -f backend
```

### 4. Acessar interfaces

| Interface            | URL                              |
| -------------------- | -------------------------------- |
| **Dashboard AGUADA** | http://localhost                 |
| **API Backend**      | http://localhost:3000/api        |
| **Grafana**          | http://localhost:3001            |
| **Health Check**     | http://localhost:3000/api/health |

---

## 🧪 Testes de Funcionamento

### Teste 1: Health Check (Backend Vivo)

```bash
curl http://localhost:3000/api/health
# Resposta esperada: {"status":"ok","timestamp":"...","service":"aguada-backend",...}
```

### Teste 2: Enviar Telemetria

```bash
curl -X POST http://localhost:3000/api/telemetry \
  -H "Content-Type: application/json" \
  -d '{
    "mac": "20:6E:F1:6B:77:58",
    "type": "distance_cm",
    "value": 24480,
    "battery": 5000,
    "uptime": 100,
    "rssi": -50
  }'
# Resposta esperada: {"success":true,"message":"Telemetria recebida com sucesso",...}
```

### Teste 3: Obter Últimas Leituras

```bash
curl http://localhost:3000/api/readings/latest
# Retorna array com as últimas leituras de todos os sensores
```

### Teste 4: Verificar Banco de Dados

```bash
# De dentro do container
docker compose exec postgres psql -U aguada -d aguada -c "SELECT COUNT(*) FROM aguada.leituras_raw;"

# Ou localmente (se psql instalado)
psql -h localhost -p 5433 -U aguada -d aguada_db -c "SELECT * FROM aguada.leituras_raw LIMIT 5;"
```

---

## 🔌 Configuração do Gateway ESP32

### Conexão USB do Gateway

1. Conectar ESP32 Gateway via USB ao computador
2. Identificar porta serial (geralmente `/dev/ttyACM0` em Linux/Mac ou `COM3` em Windows)
3. Verificar logs do backend:

```bash
# Ver tentativas de conexão
docker compose logs backend | grep -i "serial\|gateway"
```

### Dados Esperados do Gateway

O gateway envia dados em formato JSON:

```json
{
  "mac": "20:6E:F1:6B:77:58",
  "type": "distance_cm",
  "value": 24480,
  "battery": 5000,
  "uptime": 3,
  "rssi": -50
}
```

---

## 📊 Estrutura de Dados

### Sensores Configurados

| Sensor ID  | Reservatório | MAC Address       | Status   |
| ---------- | ------------ | ----------------- | -------- |
| RCON_nivel | RCON         | 20:6E:F1:6B:77:58 | ✅ Ativo |
| RCAV_nivel | RCAV         | DC:06:75:67:6A:CC | ✅ Ativo |
| RB03_nivel | RB03         | TBD               | Pendente |
| IE01_nivel | IE01         | TBD               | Pendente |
| IE02_nivel | IE02         | TBD               | Pendente |

### Variáveis Monitoradas

Para cada sensor são monitoradas:

- `distance_cm` - Nível de água (distância em cm × 100)
- `valve_in` - Estado válvula entrada (0/1)
- `valve_out` - Estado válvula saída (0/1)
- `sound_in` - Detector de fluxo (0/1)

---

## 🛠️ Comandos Úteis

### Gerenciar Docker Compose

```bash
# Iniciar serviços
docker compose up -d

# Parar serviços
docker compose down

# Ver status
docker compose ps

# Ver logs em tempo real
docker compose logs -f

# Logs de um serviço específico
docker compose logs -f backend
docker compose logs -f postgres

# Reiniciar um serviço
docker compose restart backend

# Executar comando em um container
docker compose exec backend npm run dev
docker compose exec postgres psql -U aguada -d aguada
```

### Gerenciar Backend (sem Docker)

```bash
cd backend

# Instalar dependências
npm install

# Iniciar em desenvolvimento
npm run dev

# Iniciar em produção
npm start

# Rodar testes
npm test
```

### Gerenciar Banco de Dados

```bash
# Conectar ao banco via Docker
docker compose exec postgres psql -U aguada -d aguada

# Comandos SQL úteis
SELECT * FROM aguada.leituras_raw LIMIT 10;
SELECT COUNT(*) FROM aguada.leituras_raw;
SELECT DISTINCT sensor_id FROM aguada.sensores;
SELECT * FROM aguada.sensores;
```

---

## 🔐 Credenciais Padrão

### Banco de Dados

- **Host**: localhost
- **Porta**: 5433 (Docker) / 5432 (nativo)
- **Database**: aguada_db / aguada
- **Usuário**: aguada_user / aguada
- **Senha**: aguada_pass_2025 / aguada123

### Grafana

- **URL**: http://localhost:3001
- **Usuário**: admin
- **Senha**: admin

### Backend API

- **URL**: http://localhost:3000/api
- **Health**: http://localhost:3000/api/health
- **Sem autenticação** (por padrão)

⚠️ **Em produção**, altere as senhas e configure JWT/OAuth!

---

## 📁 Estrutura de Pastas

```
aguada/
├── backend/                      # API Node.js/Express
│   ├── src/
│   │   ├── server.js            # Entry point
│   │   ├── routes/api.routes.js # Definição de rotas
│   │   ├── controllers/         # Controllers
│   │   ├── services/            # Serviços
│   │   └── config/              # Configurações
│   ├── .env                     # Variáveis de ambiente
│   └── package.json
├── frontend/                     # Dashboard HTML/JS
│   ├── index.html               # Home
│   ├── painel.html              # Painel de controle
│   ├── assets/                  # CSS, JS, imagens
│   └── components/              # Componentes
├── database/                     # SQL schemas
│   └── schema.sql               # Schema TimescaleDB
├── docker/                       # Configs Docker
│   ├── nginx.conf               # Configuração Nginx
│   ├── Dockerfile.backend       # Build backend
│   └── mosquitto/               # MQTT configs
├── docker-compose.yml           # Orquestração Docker
├── docs/                        # Documentação
│   └── RULES.md                 # Regras do sistema
└── README.md
```

---

## 🐛 Troubleshooting

### Erro: "Porta já em uso"

```bash
# Liberar porta (exemplo: 3000)
sudo fuser -k 3000/tcp

# Ou mudar porta no .env
PORT=5000
```

### Erro: "Connection refused" ao conectar banco

```bash
# Verificar se postgres está rodando
docker compose ps postgres

# Ver logs do postgres
docker compose logs postgres

# Reiniciar postgres
docker compose restart postgres
```

### Erro: "Serial port not found"

- Gateway USB pode estar em porta diferente
- No Docker, a porta serial precisa ser mapeada explicitamente
- Verificar: `ls -la /dev/ttyACM*` (Linux) ou `COM3` (Windows)

### Backend não responde

```bash
# Verificar logs
docker compose logs backend

# Testar saúde
curl http://localhost:3000/api/health

# Reiniciar
docker compose restart backend
```

---

## 📤 Enviar para Outro Computador

### Método 1: Copiar Pasta (Recomendado)

```bash
# Copiar projeto completo
cp -r aguada /media/usb/

# No outro computador
cp -r /media/usb/aguada ~/

cd ~/aguada
docker compose up -d
```

### Método 2: Git

```bash
# No novo computador
git clone https://github.com/luctronics-ET/aguada.git
cd aguada
cp backend/.env.example backend/.env
docker compose up -d
```

### Método 3: Backup Completo com Banco

```bash
# Fazer backup do banco
docker compose exec postgres pg_dump -U aguada aguada > aguada_backup.sql

# Copiar projeto + backup para pendrive/disco externo
# No outro computador, restaurar banco:
docker compose up -d postgres

# Esperar postgres estar pronto
sleep 10

# Restaurar backup
docker compose exec -T postgres psql -U aguada aguada < aguada_backup.sql

# Iniciar outros serviços
docker compose up -d
```

---

## 📈 Próximos Passos

1. **Configurar sensores ESP32**

   - Flashear firmware correto (node_sensor_10 ou node_sensor_20)
   - Conectar gateway ESP32 via USB

2. **Calibrar sensores**

   - POST /api/calibration com valores conhecidos
   - Verificar leituras em tempo real no dashboard

3. **Configurar alertas**

   - Definir thresholds para cada sensor
   - Testar notificações

4. **Integrar Grafana**

   - Criar dashboards customizados
   - Configurar alertas automáticos

5. **Sincronizar com outro computador**
   - Backup periódico do banco
   - Script de sincronização

---

## 📞 Suporte

Para problemas ou dúvidas:

1. Verificar logs: `docker compose logs -f`
2. Consultar documentação em `docs/RULES.md`
3. Executar testes: `npm test`

---

**Última atualização**: 05/12/2025  
**Versão**: 2.0.0  
**Status**: ✅ Pronto para Produção
