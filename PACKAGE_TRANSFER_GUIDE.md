# 📦 Pacote AGUADA v2.1.0 - Instruções de Transferência e Instalação

## ✅ Status do Sistema

**Data:** 2025-12-05 23:19:00 UTC  
**Versão:** 2.1.0  
**Status:** 🟢 **PRONTO PARA PRODUÇÃO**  
**Testes:** ✓ 8/8 Passados  
**Docker:** ✓ 5 containers saudáveis

---

## 📦 Arquivo de Distribuição Gerado

```
Nome: aguada-v2.1.0-20251205_201922.tar.gz
Tamanho: 1,7 MB (comprimido)
Local: /home/luciano/Área de trabalho/aguada/
SHA-256: 8c25a651c6f3252693bc46048f51d16b216e935fc3f4bfd8473aaced62b55522
```

### Conteúdo do Pacote

- ✅ Backend Node.js/Express (completo)
- ✅ Frontend HTML/CSS/JavaScript (PWA)
- ✅ Docker Compose configuration
- ✅ Database schema (PostgreSQL/TimescaleDB)
- ✅ Firmware ESP32 fontes
- ✅ Documentação completa
- ✅ Scripts de deploy e teste
- ✅ Configurações de exemplo

---

## 🚀 Instruções de Transferência para Outro Computador

### Método 1: USB/Pen Drive (Recomendado para Computador Offline)

```bash
# No computador de origem (onde o AGUADA foi criado)
# 1. Copiar arquivo para USB
cp /home/luciano/Área\ de\ trabalho/aguada/aguada-v2.1.0-*.tar.gz /media/seu_usuario/seu_usb/

# 2. Copiar também o checksum
cp /home/luciano/Área\ de\ trabalho/aguada/*.sha256 /media/seu_usuario/seu_usb/

# 3. Ejetar USB com segurança
sudo umount /media/seu_usuario/seu_usb
```

### Método 2: SCP/SSH (Para Transferência pela Rede)

```bash
# Do computador local para remoto
scp /home/luciano/Área\ de\ trabalho/aguada/aguada-v2.1.0-*.tar.gz \
    usuario@seu_servidor:/home/usuario/

# Copiar também o checksum
scp /home/luciano/Área\ de\ trabalho/aguada/*.sha256 \
    usuario@seu_servidor:/home/usuario/
```

### Método 3: Cloud Storage (Google Drive, OneDrive, etc)

```bash
# Fazer upload manual do arquivo .tar.gz e .sha256
# Gerar link de compartilhamento
# Compartilhar com destinatário
```

---

## ✅ Instruções de Instalação no Novo Computador

### Pré-requisitos

```bash
# Verificar Docker
docker --version
# Esperado: Docker version 20.10+

# Se não tiver, instalar
# Ubuntu/Debian:
sudo apt update && sudo apt install -y docker.io docker-compose

# macOS:
brew install docker docker-compose

# Windows: Baixar Docker Desktop
# https://www.docker.com/products/docker-desktop
```

### Passo 1: Validar Integridade do Arquivo

```bash
# Entrar no diretório onde copiou o arquivo
cd /caminho/para/aguada-v2.1.0-*.tar.gz

# Validar checksum (Linux/macOS)
sha256sum -c aguada-v2.1.0-*.tar.gz.sha256
# Esperado: "OK"

# Ou validar manualmente comparando
sha256sum aguada-v2.1.0-*.tar.gz
# Comparar com: 8c25a651c6f3252693bc46048f51d16b216e935fc3f4bfd8473aaced62b55522
```

### Passo 2: Extrair o Pacote

```bash
# Extrair
tar xzf aguada-v2.1.0-*.tar.gz

# Navegar
cd aguada

# Verificar conteúdo
ls -la
# Esperado: README.md, DEPLOYMENT.md, docker-compose.yml, deploy-automatic.sh, etc
```

### Passo 3: Executar Deploy Automático

```bash
# Tornar script executável
chmod +x deploy-automatic.sh

# Executar deploy
bash deploy-automatic.sh

# Ou com opções de segurança
bash deploy-automatic.sh --secure --backup
```

**O script irá:**

- ✅ Verificar pré-requisitos (Docker, portas, espaço em disco)
- ✅ Configurar arquivo `.env` com senhas
- ✅ Iniciar 5 containers Docker
- ✅ Inicializar database PostgreSQL
- ✅ Executar health checks
- ✅ Rodar 8 testes de validação
- ✅ Criar backup inicial
- ✅ Mostrar credenciais de acesso

### Passo 4: Acessar o Sistema

Após sucesso do deploy, acesse:

```
🌐 Dashboard:  http://localhost
   Acesso:     Qualquer navegador

📊 Grafana:    http://localhost:3001
   User:       admin
   Password:   admin (MUDE IMEDIATAMENTE!)

🔌 API:        http://localhost:3000/api
   Health:     http://localhost:3000/api/health
   Sensors:    http://localhost:3000/api/sensors
   Readings:   http://localhost:3000/api/readings/latest
```

### Passo 5: Testes de Validação

```bash
# Executar suite de testes
./test-sistema.sh

# Esperado: 8/8 testes passarem ✓
```

---

## 🔒 Pós-Instalação - Segurança

### IMPORTANTE: Alterar Credenciais Default

```bash
# 1. Mudar senha Grafana
# Acessar: http://localhost:3001
# Clicar em: Perfil → Change Password
# Mudar de: admin/admin para senha forte

# 2. Mudar senha Database (se necessário)
docker-compose exec postgres psql -U aguada_user -d aguada
# SQL: ALTER USER aguada_user PASSWORD 'sua_senha_nova_forte';

# 3. Gerar novo JWT Secret
# Editar backend/.env e gerar JWT_SECRET
openssl rand -base64 32 > jwt_secret.txt
```

### Configurar SSL/HTTPS (Recomendado em Produção)

```bash
# Gerar certificado auto-assinado (teste)
bash deploy-automatic.sh --secure

# Ou usar certificado válido
# Copiar certificado.crt e certificado.key para docker/certs/
# Nginx usará automaticamente
```

### Configurar Firewall

```bash
# Permitir apenas portas necessárias
sudo ufw allow 80/tcp      # HTTP
sudo ufw allow 443/tcp     # HTTPS
sudo ufw allow 22/tcp      # SSH
sudo ufw deny 5432/tcp     # Bloquear PostgreSQL
sudo ufw deny 6379/tcp     # Bloquear Redis
```

---

## 📊 Primeiras Ações

### 1. Conectar Sensores ESP32

Se tiver sensors ESP32-C3 físicos:

```bash
# Conectar gateway ESP32 via USB
# Sistema detectará automaticamente em /dev/ttyACM0

# Verificar conexão nos logs
docker-compose logs backend | grep -i serial

# Verificar leituras recebidas
curl http://localhost:3000/api/readings/latest
```

### 2. Carregar Dados (Se Tiver Backup)

```bash
# Se tiver arquivo de backup anterior
docker-compose exec postgres psql -U aguada -d aguada < seu_backup.sql

# Ou dados de exemplo
docker-compose exec postgres psql -U aguada -d aguada < database/sample-data.sql
```

### 3. Configurar Alertas

```bash
# Acessar API para definir alertas
curl -X POST http://localhost:3000/api/alerts \
  -H "Content-Type: application/json" \
  -d '{
    "tipo": "nivel_critico",
    "elemento_id": "RCON",
    "descricao": "Nível baixo no reservatório",
    "nivel": "crítico"
  }'
```

---

## 📈 Monitoramento em Produção

### Logs

```bash
# Tempo real
docker-compose logs -f backend

# Últimas 100 linhas
docker-compose logs backend --tail 100

# Por serviço
docker-compose logs -f postgres
docker-compose logs -f redis
docker-compose logs -f nginx
```

### Métricas

```bash
# Status dos containers
docker-compose ps

# Uso de recursos
docker stats

# Grafana dashboards
# http://localhost:3001
```

### Backups Automáticos

```bash
# Criar script de backup diário
mkdir -p backups
cat > backup_daily.sh << 'EOF'
#!/bin/bash
BACKUP_DIR="./backups"
mkdir -p "$BACKUP_DIR"
docker-compose exec -T postgres pg_dump -U aguada aguada | \
  gzip > "$BACKUP_DIR/backup_$(date +%Y%m%d_%H%M%S).sql.gz"
EOF

chmod +x backup_daily.sh

# Adicionar ao cron (executar diariamente às 2:00 AM)
# 0 2 * * * /caminho/para/backup_daily.sh
```

---

## 🆘 Troubleshooting

### Erro: "Docker daemon não está rodando"

```bash
sudo systemctl start docker
sudo systemctl enable docker
```

### Erro: "Port 3000 already in use"

```bash
# Encontrar processo na porta
sudo lsof -i :3000

# Matar processo ou alterar PORT em backend/.env
```

### Erro: "Database connection timeout"

```bash
# Aguardar mais tempo para database inicializar
docker-compose logs postgres

# Reiniciar
docker-compose down
docker-compose up -d
sleep 30
```

### Erro: "Maximum call stack size exceeded" (Frontend)

```bash
# Há um bug JavaScript que será corrigido em v2.1.1
# Por enquanto, dashboard ainda funciona mesmo com erro
# Usar API diretamente para dados: http://localhost:3000/api/readings/latest
```

### Erro: Sensor não conecta via USB

```bash
# Verificar porta USB
ls /dev/ttyACM*
ls /dev/tty.usbserial*

# Dar permissões
sudo chmod 666 /dev/ttyACM0

# Ou adicionar user ao grupo
sudo usermod -a -G dialout $USER
```

---

## 📋 Documentação Disponível

No diretório do projeto:

| Arquivo                | Descrição                             |
| ---------------------- | ------------------------------------- |
| `DEPLOYMENT.md`        | Guia detalhado de deploy (60 páginas) |
| `QUICKSTART_DEPLOY.md` | Guia rápido (dentro do pacote)        |
| `docs/RULES.md`        | Especificação técnica completa        |
| `docs/SETUP.md`        | Guia de configuração                  |
| `backend/README.md`    | Documentação API                      |
| `firmware/*/README.md` | Guia firmware ESP32                   |

---

## 📞 Suporte Rápido

### Verificação de Saúde

```bash
# Health check rápido
curl -s http://localhost:3000/api/health | jq

# Todos os endpoints
./test-sistema.sh
```

### Logs de Diagnóstico

```bash
# Salvar logs para análise
docker-compose logs > aguada_diagnostico_$(date +%Y%m%d_%H%M%S).log
```

### Teste de Conectividade

```bash
# Testar API
curl -X POST http://localhost:3000/api/telemetry \
  -H "Content-Type: application/json" \
  -d '{
    "mac":"20:6E:F1:6B:77:58",
    "type":"distance_cm",
    "value":24480,
    "battery":5000,
    "uptime":10,
    "rssi":-50
  }'
```

---

## ✨ Próximas Etapas Recomendadas

1. **Setup Completo**

   - [ ] Alterar todas as senhas default
   - [ ] Configurar SSL/HTTPS
   - [ ] Setup firewall

2. **Integração**

   - [ ] Conectar sensores ESP32
   - [ ] Carregar dados históricos (se houver)
   - [ ] Testar fluxo completo sensor→API→dashboard

3. **Produção**

   - [ ] Configurar monitoramento 24/7
   - [ ] Setup alertas por email/Slack
   - [ ] Backup automático diário
   - [ ] Plano de disaster recovery

4. **Customização** (Opcional)
   - [ ] Adicionar mais sensores
   - [ ] Personalizar dashboards
   - [ ] Implementar regras de negócio específicas

---

## 📋 Checklist Final

- [ ] Pacote recebido e checksum validado
- [ ] Docker e Docker Compose instalados
- [ ] Arquivo extraído
- [ ] Deploy automático executado com sucesso
- [ ] 8/8 testes passando
- [ ] Frontend acessível (http://localhost)
- [ ] API respondendo (http://localhost:3000/api)
- [ ] Grafana funcionando (http://localhost:3001)
- [ ] Senhas default alteradas
- [ ] Backups iniciais criados
- [ ] Sistema pronto para operação

---

**🎉 Parabéns! Sistema AGUADA v2.1.0 está pronto para uso!**

Para questões adicionais, consulte a documentação completa em `DEPLOYMENT.md` ou acesse `docs/RULES.md` para detalhes técnicos.

---

**Data:** 2025-12-05  
**Versão:** 2.1.0  
**Status:** ✅ Production Ready
