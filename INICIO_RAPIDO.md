# ⚡ AGUADA - Início Rápido

Guia rápido para começar em 5 minutos.

## 🚀 Setup Automático

```bash
cd "/home/luciano/Área de trabalho/aguada"
./setup.sh
```

## 📝 Setup Manual (Passo a Passo)

### 1. Banco de Dados (2 minutos)

```bash
# Criar banco
sudo -u postgres psql << 'EOF'
CREATE USER aguada_user WITH PASSWORD 'aguada_pass_2025';
CREATE DATABASE aguada_db OWNER aguada_user;
\c aguada_db
CREATE SCHEMA aguada;

-- Tabela de sensores
CREATE TABLE aguada.sensores (
    sensor_id VARCHAR(50) PRIMARY KEY,
    elemento_id VARCHAR(50) NOT NULL,
    node_mac VARCHAR(17),
    variavel VARCHAR(50) NOT NULL,
    tipo VARCHAR(50),
    status VARCHAR(20) DEFAULT 'ativo'
);

-- Tabela de leituras
CREATE TABLE aguada.leituras_raw (
    leitura_id SERIAL PRIMARY KEY,
    sensor_id VARCHAR(50) NOT NULL,
    elemento_id VARCHAR(50) NOT NULL,
    variavel VARCHAR(50) NOT NULL,
    valor NUMERIC(10,2) NOT NULL,
    unidade VARCHAR(10) DEFAULT 'cm',
    datetime TIMESTAMP DEFAULT NOW(),
    meta JSONB,
    FOREIGN KEY (sensor_id) REFERENCES aguada.sensores(sensor_id)
);

-- Índices
CREATE INDEX idx_leituras_datetime ON aguada.leituras_raw(datetime DESC);
CREATE INDEX idx_leituras_sensor ON aguada.leituras_raw(sensor_id, datetime DESC);

-- Permissões
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA aguada TO aguada_user;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA aguada TO aguada_user;

-- Sensor de exemplo
INSERT INTO aguada.sensores VALUES 
('SEN_CON_01', 'RCON', '20:6e:f1:6b:77:58', 'distance_cm', 'ultrassonico');
EOF
```

### 2. Backend (2 minutos)

```bash
cd backend

# Criar .env
cat > .env << 'EOF'
PORT=3000
DB_HOST=localhost
DB_PORT=5432
DB_USER=aguada_user
DB_PASSWORD=aguada_pass_2025
DB_NAME=aguada_db
SERIAL_PORT=/dev/ttyACM0
SERIAL_BAUD=115200
EOF

# Instalar dependências
npm install

# Iniciar
npm start
```

### 3. Testar (1 minuto)

```bash
# Terminal 1: Backend rodando
# Terminal 2: Testar

# Health check
curl http://localhost:3000/api/health

# Enviar telemetria de teste
curl -X POST http://localhost:3000/api/telemetry \
  -H "Content-Type: application/json" \
  -d '{
    "mac": "20:6e:f1:6b:77:58",
    "type": "distance_cm",
    "value": 24480,
    "battery": 5000,
    "rssi": -50
  }'

# Ver leituras
curl http://localhost:3000/api/readings/latest

# Verificar no banco
psql -h localhost -U aguada_user -d aguada_db -c \
  "SELECT * FROM aguada.leituras_raw ORDER BY datetime DESC LIMIT 5;"
```

## ✅ Checklist

- [ ] PostgreSQL instalado e rodando
- [ ] Banco `aguada_db` criado
- [ ] Tabelas criadas
- [ ] Backend instalado (`npm install`)
- [ ] Arquivo `.env` configurado
- [ ] Backend rodando (`npm start`)
- [ ] API respondendo (`/api/health`)
- [ ] Telemetria de teste funcionando

## 🐛 Problemas Comuns

### "Cannot find module"
```bash
cd backend
rm -rf node_modules package-lock.json
npm install
```

### "Database connection failed"
```bash
# Verificar se PostgreSQL está rodando
sudo systemctl status postgresql

# Verificar credenciais no .env
cat backend/.env
```

### "Permission denied /dev/ttyACM0"
```bash
sudo chmod 666 /dev/ttyACM0
# ou
sudo usermod -a -G dialout $USER
# (fazer logout/login)
```

## 📚 Próximos Passos

1. Ver `SETUP_COMPLETO.md` para guia detalhado
2. Configurar gateway ESP32
3. Configurar sensores ESP32
4. Criar frontend

---

**Tempo total estimado: 5-10 minutos** ⏱️

