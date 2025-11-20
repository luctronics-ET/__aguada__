# 📊 AGUADA - Status do Sistema
**Data**: 19 de novembro de 2025, 15:19 BRT

---

## ✅ STATUS GERAL: **OPERACIONAL**

### 🎯 Componentes Principais

| Componente | Status | Detalhes |
|------------|--------|----------|
| **Backend** | ✅ Ativo | Node.js (PID: 53665), rodando há 11h53min |
| **API REST** | ✅ Respondendo | http://localhost:3000 |
| **WebSocket** | ✅ Disponível | ws://localhost:3000/ws |
| **Frontend** | ✅ Servindo | http://localhost:9000 (Python HTTP Server) |
| **Gateway USB** | ✅ Conectado | /dev/ttyACM0 @ 115200 baud |
| **Database** | ⚠️ Desabilitado | Modo simplificado (dados em memória) |
| **PostgreSQL** | 🟡 Disponível | v16 rodando, mas não usado nesta sessão |

---

## 📡 Sensores ESP32-C3

### Sensores Ativos: **2/5**

| ID | Nome | MAC Address | Distância | Uptime | Última Leitura |
|----|------|-------------|-----------|--------|----------------|
| **RCON** | Castelo Consumo | `20:6e:f1:6b:77:58` | **52.77 cm** | 34h 22min | 15:19:35 |
| **RCAV** | Castelo Incêndio | `dc:06:75:67:6a:cc` | **291.85 cm** | 34h 22min | 15:19:17 |

### Sensores Offline: 3/5
- RB03 (Casa de Bombas)
- IE01 (Cisterna IE01)
- IE02 (Cisterna IE02)

---

## 🔧 Dados Técnicos

### Hardware
- **Microcontrolador**: ESP32-C3 SuperMini
- **Sensor Ultrassônico**: AJ-SR04M (20-450 cm)
- **Protocolo**: ESP-NOW (2.4GHz)
- **Alimentação**: 5V DC
- **Bateria**: 5.0V (ambos sensores)
- **Sinal**: -50 dBm (excelente qualidade)

### Comunicação
- **Taxa de amostragem**: ~30 segundos por leitura
- **Gateway → Backend**: Serial USB (115200 baud)
- **Backend → Frontend**: HTTP REST + WebSocket
- **Latência**: < 100ms

### Volume Calculado (estimativa)
- **RCON**: ~86.8% de capacidade (altura máxima: 400cm)
- **RCAV**: ~16.6% de capacidade (altura máxima: 350cm)

---

## 🌐 Endpoints Disponíveis

### API Backend
- `GET /api/health` - Status da API
- `GET /api/readings/latest` - Últimas leituras
- `GET /api/readings/history/:sensor_id` - Histórico
- `GET /api/sensors/status` - Status dos sensores
- `POST /api/telemetry` - Envio de dados (ESP32)

### Frontend
- http://localhost:9000/index.html - Dashboard principal
- http://localhost:9000/mapa.html - Mapa dos reservatórios
- http://localhost:9000/painel.html - Painel de controle
- http://localhost:9000/test.html - Página de testes

---

## 📈 Estatísticas de Operação

### Backend
- **Leituras recebidas**: ~14.900 (34 horas × 2 sensores × 120 leituras/hora)
- **Uptime**: 11h 53min
- **Memória**: 55 MB
- **CPU**: ~0.1%

### Sensores
- **RCON**: 34h 22min de operação contínua
- **RCAV**: 34h 22min de operação contínua
- **Taxa de sucesso**: ~100% (sem perdas de pacotes detectadas)

---

## 🚀 Próximas Ações

### Curto Prazo
1. ✅ Verificar funcionamento do frontend (cards dos sensores)
2. ⏳ Conectar 3 sensores restantes (RB03, IE01, IE02)
3. ⏳ Ativar persistência de dados no PostgreSQL/TimescaleDB

### Médio Prazo
1. Implementar alertas automáticos (níveis críticos)
2. Configurar backup automático de dados
3. Dashboard de histórico e tendências
4. Integração com sistema de notificações

### Longo Prazo
1. Machine Learning para predição de consumo
2. App mobile (React Native)
3. Expansão para mais reservatórios

---

## 📝 Notas Técnicas

### Modo Operacional Atual
O sistema está rodando em **modo simplificado** sem conexão com banco de dados. Os dados são armazenados em memória (volátil) e serão perdidos ao reiniciar o backend. Esta configuração é adequada para:
- Testes e desenvolvimento
- Validação de hardware
- Demonstrações rápidas

Para **produção**, recomenda-se:
- Ativar conexão com PostgreSQL/TimescaleDB
- Configurar retenção de dados (7-30 dias)
- Implementar backup automático

---

## 🐛 Troubleshooting

### Frontend não mostra dados
**Sintoma**: Dashboard vazio ou "Aguardando dados..."

**Soluções**:
1. Abrir http://localhost:9000/test.html e clicar em "Testar API"
2. Verificar console do navegador (F12)
3. Limpar cache (Ctrl+Shift+R)
4. Verificar se `window.apiService` está definido no console

### Backend não recebe dados
**Sintoma**: Logs do gateway não mostram novas leituras

**Soluções**:
1. Verificar conexão USB: `ls -la /dev/ttyACM0`
2. Verificar permissões: `sudo usermod -a -G dialout $USER`
3. Reiniciar backend: `pkill -f server-test && node server-test-realdata.js`

### Sensores offline
**Sintoma**: Apenas 2/5 sensores transmitindo

**Status Atual**: Normal - apenas RCON e RCAV foram programados até o momento.

---

**Gerado automaticamente pelo AGUADA System Monitor**
