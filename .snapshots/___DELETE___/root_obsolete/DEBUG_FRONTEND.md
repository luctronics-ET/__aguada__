# 🔍 Debug do Frontend - Dados Não Carregando

## 🚀 Inicialização Rápida

Para inicializar todo o sistema rapidamente, use o script PHP:

```bash
php init.php

```

Ou se preferir o script bash:

```bash
./setup.sh
```

## ✅ Problema Identificado e Corrigido

**Problema**: A API retorna `nivel_cm` mas o frontend procurava apenas `distance_cm`.

**Solução**: Frontend atualizado para aceitar ambos os formatos.

## 🧪 Como Testar

### 1. Abrir o Navegador
```


### 2. Abrir Console do Desenvolvedor
- Pressione **F12** ou **Ctrl+Shift+I**
- Vá para a aba **Console**

### 3. Verificar Logs
Você deve ver:
```
🔄 Carregando dados de: <http://localhost:3000/api/readings/latest>
📊 Dados recebidos: {success: true, data: {...}}
✅ 5 sensores encontrados
📡 Sensor IE01_US01: {valor: "279.14", datetime: "...", variables: ["nivel_cm"]}
```

### 4. Se Não Aparecer Dados

#### Limpar Cache do Navegador
- **Chrome/Edge**: Ctrl+Shift+R (hard refresh)
- **Firefox**: Ctrl+F5

#### Verificar Erros no Console
- Procure por mensagens em vermelho
- Verifique se há erros de CORS
- Verifique se a API está respondendo

### 5. Testar API Diretamente

```bash
# No terminal
curl http://localhost:3000/api/readings/latest | python3 -m json.tool
```

Deve retornar:
```json
{
  "success": true,
  "data": {
    "IE01_US01": {
      "sensor_id": "IE01_US01",
      "elemento_id": "IE01",
      "variables": {
        "nivel_cm": {
          "valor": "279.14",
          "unidade": "cm",
          "datetime": "..."
        }
      }
    },
    ...
  }
}
```

## 🐛 Problemas Comuns

### "Failed to fetch"
- **Causa**: Backend não está rodando
- **Solução**: `cd backend && npm start`

### "CORS error"
- **Causa**: Backend não permite requisições do frontend
- **Solução**: Verificar se CORS está habilitado no backend

### "Empty data"
- **Causa**: Não há leituras no banco de dados
- **Solução**: Inserir dados de teste (veja abaixo)

## 📊 Inserir Dados de Teste

```bash
# Conectar ao banco
psql -h localhost -U aguada_user -d aguada_db

# Inserir leitura de teste
INSERT INTO aguada.leituras_raw 
(sensor_id, elemento_id, variavel, valor, unidade, datetime, fonte, autor, modo)
VALUES 
('SEN_CON_01', 'RCON', 'distance_cm', 347.20, 'cm', NOW(), 'sensor', 'teste', 'automatica');
```

## ✅ Checklist

- [ ] Backend rodando na porta 3000
- [ ] Frontend rodando na porta 8080
- [ ] Console do navegador aberto (F12)
- [ ] Cache limpo (Ctrl+Shift+R)
- [ ] API respondendo (testar com curl)
- [ ] Dados no banco (verificar com psql)

## 🎯 Status Atual

- ✅ API retornando 5 sensores
- ✅ CORS configurado corretamente
- ✅ Frontend atualizado para aceitar `nivel_cm`
- ✅ Logs de debug adicionados

**Próximo passo**: Abrir <http://localhost:8080> e verificar o console (F12)

