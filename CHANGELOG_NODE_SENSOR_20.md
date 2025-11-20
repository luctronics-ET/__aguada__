# 📋 AGUADA - Alterações do Projeto: Firmware Dual (node_sensor_20)

**Data**: 19 de novembro de 2025  
**Versão**: 2.0.0  
**Autor**: Equipe AGUADA

---

## 🎯 Objetivo da Mudança

Criar um novo tipo de firmware **TYPE_DUAL_ULTRA** (node_sensor_20) que permita monitorar **2 reservatórios** (IE01 e IE02) com um único ESP32-C3, reduzindo de 5 para 4 microcontroladores no sistema.

---

## 📊 Resumo das Alterações

### Antes (Sistema Antigo)
- **5 ESP32-C3** (1 por reservatório)
- **5 firmwares idênticos** (TYPE_SINGLE_ULTRA)
- **RCON, RCAV, RB03, IE01, IE02** - cada um com seu ESP32

### Depois (Sistema Novo)
- **4 ESP32-C3** (economia de 1 microcontrolador)
- **2 tipos de firmware**:
  - **TYPE_SINGLE_ULTRA** (node_sensor_10): RCON, RCAV, RB03
  - **TYPE_DUAL_ULTRA** (node_sensor_20): IE01 + IE02
- **IE01 e IE02** compartilham o mesmo ESP32-C3

---

## 🔧 Alterações Técnicas

### 1. Novo Firmware: `firmware/node_sensor_20/`

#### Estrutura de Arquivos
```
firmware/node_sensor_20/
├── CMakeLists.txt           # Configuração do projeto
├── sdkconfig.defaults       # Configurações padrão ESP-IDF
├── README.md                # Documentação completa
└── main/
    ├── main.c               # Código principal (470 linhas)
    ├── config.h             # Configuração de GPIOs e constantes
    └── CMakeLists.txt       # Build do componente
```

#### Características Técnicas
- **2 sensores ultrassônicos** AJ-SR04M
- **4 válvulas** (2 por reservatório)
- **2 detectores de som** (1 por reservatório)
- **8 variáveis enviadas** (4 por reservatório)
- **Protocolo ESP-NOW** individual por variável

#### GPIOs Utilizados

| Componente | IE01 GPIO | IE02 GPIO |
|------------|-----------|-----------|
| Ultrasonic TRIG | 0 | 2 |
| Ultrasonic ECHO | 1 | 3 |
| Sound Detector | 5 | 6 |
| Valve IN | 7 | 9 |
| Valve OUT | 8 | 10 |
| LED Status | 8 (compartilhado) | 8 (compartilhado) |

**Total GPIOs usados**: 11 (vs 6 no TYPE_SINGLE_ULTRA)

### 2. Protocolo de Dados

#### TYPE_SINGLE_ULTRA (node_sensor_10)
```json
{"mac":"20:6e:f1:6b:77:58","type":"distance_cm","value":24480,"battery":5000,"uptime":120,"rssi":-50}
{"mac":"20:6e:f1:6b:77:58","type":"valve_in","value":1,"battery":5000,"uptime":120,"rssi":-50}
{"mac":"20:6e:f1:6b:77:58","type":"valve_out","value":0,"battery":5000,"uptime":120,"rssi":-50}
{"mac":"20:6e:f1:6b:77:58","type":"sound_in","value":0,"battery":5000,"uptime":120,"rssi":-50}
```
**Total**: 4 variáveis por ciclo

#### TYPE_DUAL_ULTRA (node_sensor_20)
```json
{"mac":"XX:XX:XX:XX:XX:XX","type":"IE01_distance_cm","value":25480,"battery":5000,"uptime":120,"rssi":-50}
{"mac":"XX:XX:XX:XX:XX:XX","type":"IE02_distance_cm","value":18350,"battery":5000,"uptime":120,"rssi":-50}
{"mac":"XX:XX:XX:XX:XX:XX","type":"IE01_valve_in","value":1,"battery":5000,"uptime":120,"rssi":-50}
{"mac":"XX:XX:XX:XX:XX:XX","type":"IE01_valve_out","value":0,"battery":5000,"uptime":120,"rssi":-50}
{"mac":"XX:XX:XX:XX:XX:XX","type":"IE02_valve_in","value":1,"battery":5000,"uptime":120,"rssi":-50}
{"mac":"XX:XX:XX:XX:XX:XX","type":"IE02_valve_out","value":0,"battery":5000,"uptime":120,"rssi":-50}
{"mac":"XX:XX:XX:XX:XX:XX","type":"IE01_sound_in","value":0,"battery":5000,"uptime":120,"rssi":-50}
{"mac":"XX:XX:XX:XX:XX:XX","type":"IE02_sound_in","value":0,"battery":5000,"uptime":120,"rssi":-50}
```
**Total**: 8 variáveis por ciclo

**Diferença chave**: Prefixo `IE01_` ou `IE02_` no campo `type` identifica o reservatório.

---

## 📝 Arquivos Modificados

### 1. README.md
- ✅ Atualizada tabela de reservatórios (IE01 e IE02 compartilham ESP32 #4)
- ✅ Adicionada coluna "Firmware" na tabela
- ✅ Documentados os 2 tipos de firmware (TYPE_SINGLE_ULTRA e TYPE_DUAL_ULTRA)
- ✅ Atualizada estrutura do projeto com `node_sensor_20/`
- ✅ Notas sobre total de 4 ESP32-C3 (não mais 5)

### 2. docs/RULES.md
- ✅ Seção 2.1: Atualizada descrição de IE01 e IE02
- ✅ Seção 4.1: Adicionada tabela de tipos de dados para TYPE_DUAL_ULTRA
- ✅ Seção 4.1: Documentados os 2 tipos de hardware (single e dual)
- ✅ GPIOs específicos para IE01 e IE02 documentados
- ✅ Total de ESP32-C3 atualizado para 4

### 3. config/sensors.json
- ✅ Removido: `SEN_IE01_01` e `SEN_IE02_01` (sensores individuais)
- ✅ Adicionado: `SEN_IE_DUAL` (sensor dual)
- ✅ Configuração completa dos 2 ultrassônicos, 4 válvulas, 2 sons
- ✅ Mapeamento de GPIOs IE01 e IE02
- ✅ Tipos de telemetria com prefixo (IE01_, IE02_)
- ✅ `node_mapping_backend`: Node IE Dual com firmware v2.0.0

### 4. .github/copilot-instructions.md
- ✅ Seção "Firmware Types" adicionada
- ✅ Documentados TYPE_SINGLE_ULTRA e TYPE_DUAL_ULTRA
- ✅ Exemplos de código para ambos os tipos
- ✅ Tabela de GPIOs para IE01 e IE02
- ✅ Protocolo de transmissão com prefixo documentado
- ✅ Total de ESP32-C3 atualizado para 4

### 5. firmware/node_sensor_20/ (NOVO)
- ✅ CMakeLists.txt (projeto ESP-IDF)
- ✅ sdkconfig.defaults (configurações ESP32-C3)
- ✅ main/CMakeLists.txt (componente)
- ✅ main/config.h (GPIOs, constantes, configurações)
- ✅ main/main.c (firmware completo - 470 linhas)
- ✅ README.md (documentação técnica - 350 linhas)

---

## 🧪 Como Testar

### 1. Compilar o Firmware
```bash
cd firmware/node_sensor_20
idf.py set-target esp32c3
idf.py build
```

### 2. Gravar no ESP32-C3
```bash
# Descobrir porta USB
ls -la /dev/ttyACM*

# Gravar firmware
idf.py -p /dev/ttyACM0 flash monitor
```

### 3. Saída Esperada
```
I (403) AGUADA_NODE20: === AGUADA NODE 20 - DUAL ULTRASONIC (IE01 + IE02) ===
I (404) AGUADA_NODE20: Firmware: TYPE_DUAL_ULTRA
I (414) AGUADA_NODE20: GPIO inicializado (IE01: trig=0 echo=1 | IE02: trig=2 echo=3)
I (1752) AGUADA_NODE20: Node MAC: XX:XX:XX:XX:XX:XX
I (1753) AGUADA_NODE20: ESP-NOW OK - Gateway: 80:F1:B2:50:2E:C4
I (4162) AGUADA_NODE20: IE01: 254.80 cm (11 amostras)
I (6523) AGUADA_NODE20: IE02: 183.50 cm (11 amostras)
I (6524) AGUADA_NODE20: → {"mac":"...","type":"IE01_distance_cm","value":25480,...}
I (6534) AGUADA_NODE20: → {"mac":"...","type":"IE02_distance_cm","value":18350,...}
...
```

### 4. Verificar no Backend
```bash
# Verificar recepção de dados
curl http://localhost:3000/api/readings/latest | jq
```

Deve retornar dados com `type` começando com `IE01_` ou `IE02_`.

---

## 📊 Comparação de Recursos

| Característica | TYPE_SINGLE_ULTRA | TYPE_DUAL_ULTRA |
|----------------|-------------------|-----------------|
| **Reservatórios** | 1 | 2 |
| **Ultrassônicos** | 1 | 2 |
| **Válvulas** | 2 | 4 |
| **Detectores de som** | 1 | 2 |
| **GPIOs usados** | 6 | 11 |
| **Variáveis enviadas** | 4 | 8 |
| **Firmware** | node_sensor_10 | node_sensor_20 |
| **Versão** | 1.0.1 | 2.0.0 |
| **Prefixo no type** | Nenhum | IE01_ ou IE02_ |
| **Total de ESP32** | 3 (RCON, RCAV, RB03) | 1 (IE01 + IE02) |

---

## ✅ Benefícios da Mudança

### Vantagens
1. **Redução de hardware**: 4 ESP32-C3 ao invés de 5 (-20%)
2. **Economia de custos**: 1 microcontrolador a menos
3. **Simplicidade**: Menos nodes para gerenciar
4. **Otimização**: IE01 e IE02 são cisternas adjacentes no subsolo
5. **Protocolo mantido**: Continua usando transmissão individual por variável

### Desvantagens
1. **Firmware específico**: node_sensor_20 não é universal (mas IE01/IE02 são casos especiais)
2. **Mais GPIOs**: 11 vs 6 (ainda dentro do limite do ESP32-C3)
3. **Mais variáveis**: 8 transmissões a cada ciclo (vs 4 no single)
4. **Complexidade**: Código ligeiramente mais complexo (2 sensores simultâneos)

---

## 🔍 Validação

### Checklist de Testes

- [ ] Firmware compila sem erros
- [ ] ESP32-C3 inicializa corretamente
- [ ] IE01 ultrassônico funcionando
- [ ] IE02 ultrassônico funcionando
- [ ] IE01 válvulas lendo estados
- [ ] IE02 válvulas lendo estados
- [ ] IE01 som detectando água
- [ ] IE02 som detectando água
- [ ] ESP-NOW transmitindo para gateway
- [ ] Backend recebendo dados com prefixo IE01_
- [ ] Backend recebendo dados com prefixo IE02_
- [ ] Dashboard mostrando IE01 e IE02 separadamente
- [ ] Heartbeat a cada 30 segundos
- [ ] LED piscando a cada 3 segundos

---

## 📚 Documentação Relacionada

- **Firmware README**: `firmware/node_sensor_20/README.md`
- **Regras do sistema**: `docs/RULES.md`
- **GPIO Pinout**: `docs/ESP32_C3_SUPER_MINI_PINOUT.md`
- **Instruções AI**: `.github/copilot-instructions.md`
- **Configuração sensores**: `config/sensors.json`

---

## 🚀 Próximos Passos

1. **Gravar firmware** no ESP32-C3 que monitorará IE01+IE02
2. **Obter MAC address** do ESP32-C3 após boot
3. **Atualizar sensors.json** com o MAC real
4. **Instalar hardware** no subsolo (cisternas IE)
5. **Testar transmissão** de dados em produção
6. **Validar cálculos** de volume para IE01 e IE02
7. **Configurar alertas** específicos para cisternas
8. **Atualizar dashboard** para mostrar 2 cisternas

---

## 📞 Suporte

Em caso de dúvidas ou problemas:
- Consultar `firmware/node_sensor_20/README.md`
- Ler `docs/RULES.md` Seção 4 (Telemetria)
- Verificar logs serial: `idf.py monitor`
- Testar API: `curl http://localhost:3000/api/readings/latest`

---

**Versão do documento**: 1.0  
**Última atualização**: 19 de novembro de 2025  
**Status**: ✅ Implementação concluída, aguardando testes em hardware
