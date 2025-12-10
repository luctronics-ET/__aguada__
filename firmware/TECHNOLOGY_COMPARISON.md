# 📊 Firmware Technologies Comparison - AGUADA

## Decisão Final: ESP-IDF 5.3 LTS Component-Based

---

## 1. Frameworks Avaliados

### ⭐⭐⭐⭐⭐ ESP-IDF (ESCOLHIDO)

**Versão**: 5.3 LTS (suporte até 2028)

**Prós**:

- ✅ Framework oficial Espressif
- ✅ Máximo controle sobre hardware
- ✅ FreeRTOS nativo e otimizado
- ✅ OTA robusto com rollback automático
- ✅ Power management avançado (light/deep sleep)
- ✅ Componentes modulares reutilizáveis
- ✅ Excelente documentação
- ✅ Security features (Secure Boot, Flash Encryption)
- ✅ Debugging avançado (OpenOCD, GDB, Core Dump)
- ✅ WiFi/ESP-NOW otimizados

**Contras**:

- ⚠️ Curva de aprendizado (CMake, Kconfig)
- ⚠️ Compilação mais lenta
- ⚠️ Requer conhecimento de FreeRTOS

**Uso Recomendado**: ✅ **Produção industrial, sistemas críticos**

**Exemplo**:

```c
#include "esp_wifi.h"
#include "esp_now.h"
#include "freertos/FreeRTOS.h"

void app_main(void) {
    // Controle total do sistema
}
```

---

### ⭐⭐⭐ Arduino-ESP32

**Versão**: 3.0.x (baseado em ESP-IDF 5.1)

**Prós**:

- ✅ Fácil aprendizado
- ✅ Enorme biblioteca de exemplos
- ✅ Comunidade massiva
- ✅ Protipagem rápida
- ✅ IDE amigável (Arduino IDE, PlatformIO)

**Contras**:

- ❌ Overhead de abstração (~10-15% performance)
- ❌ OTA limitado (sem rollback automático)
- ❌ Power management básico
- ❌ Menos controle sobre hardware
- ❌ Debugging limitado

**Uso Recomendado**: 🧪 **Prototipagem, projetos hobby**

**Exemplo**:

```cpp
#include <WiFi.h>
#include <esp_now.h>

void setup() {
    WiFi.mode(WIFI_STA);
    esp_now_init();
}

void loop() {
    // Código simples
    delay(1000);
}
```

---

### ⭐⭐⭐⭐ PlatformIO

**Versão**: Core 6.x

**Prós**:

- ✅ Multi-framework (ESP-IDF, Arduino, Zephyr)
- ✅ Build system moderno (Python-based)
- ✅ CI/CD integrado
- ✅ Gerenciamento de bibliotecas
- ✅ IDE agnóstico (VS Code, CLion, Vim)
- ✅ Testing framework (Unity, doctest)

**Contras**:

- ⚠️ Layer extra de abstração
- ⚠️ Configuração inicial complexa
- ⚠️ Overhead em projetos pequenos

**Uso Recomendado**: 👥 **Times grandes, múltiplos projetos**

**Exemplo platformio.ini**:

```ini
[env:esp32c3]
platform = espressif32
framework = espidf
board = esp32-c3-devkitm-1
monitor_speed = 115200
```

---

### ⭐⭐ Zephyr RTOS

**Versão**: 3.x

**Prós**:

- ✅ RTOS industrial certificado
- ✅ Multi-arquitetura (ARM, RISC-V, x86)
- ✅ Stack de rede completo (LwM2M, CoAP, MQTT)
- ✅ Device Tree configuration
- ✅ Safety-critical (automotive, medical)

**Contras**:

- ❌ Curva de aprendizado íngreme
- ❌ Documentação menos amigável
- ❌ Suporte ESP32 não oficial (melhor em Nordic/STM32)
- ❌ Compilação muito lenta
- ❌ Overhead significativo

**Uso Recomendado**: 🏭 **Industrial safety-critical apenas**

**Exemplo**:

```c
#include <zephyr/kernel.h>

void main(void) {
    k_sleep(K_SECONDS(1));
}
```

---

### ❌ Mongoose OS (DESCONTINUADO)

**Status**: Projeto abandonado (2023)

**Era bom para**:

- Cloud-first IoT
- OTA automático
- JavaScript + C híbrido

**Por que NÃO usar**:

- ❌ Projeto morto
- ❌ Sem suporte/atualizações
- ❌ Vulnerabilidades de segurança

---

## 2. Protocolos de Comunicação

### ESP-NOW (ESCOLHIDO)

**Características**:

- ✅ Sem handshake WiFi
- ✅ Latência < 10ms
- ✅ Range 200-250m outdoor
- ✅ Consumo baixo
- ✅ Broadcast/unicast
- ✅ Criptografia AES

**Limitações**:

- ⚠️ Payload máx: 250 bytes
- ⚠️ Máx 20 peers
- ⚠️ Sem ACK automático

**Uso AGUADA**: ✅ **Sensor → Gateway (100% do tempo)**

---

### MQTT

**Características**:

- ✅ Publish/Subscribe
- ✅ QoS 0/1/2
- ✅ Retained messages
- ✅ Last Will Testament

**Limitações**:

- ⚠️ Requer broker
- ⚠️ Overhead TCP/IP
- ⚠️ Mais consumo energia

**Uso AGUADA**: ✅ **Gateway → Backend (opcional)**

---

### HTTP/HTTPS

**Características**:

- ✅ Universal
- ✅ REST APIs
- ✅ TLS/SSL

**Limitações**:

- ❌ Overhead grande
- ❌ Conexão persistente complexa

**Uso AGUADA**: ✅ **Gateway → Backend (atual)**

---

### CoAP (Alternativa)

**Características**:

- ✅ UDP-based (leve)
- ✅ REST-like
- ✅ Observe pattern (pub/sub)

**Limitações**:

- ⚠️ Menos suportado
- ⚠️ NAT traversal complexo

**Uso AGUADA**: ⚠️ **Futuro (opcional)**

---

## 3. Formatos de Dados

### JSON (ATUAL)

**Prós**:

- ✅ Human-readable
- ✅ Debug fácil
- ✅ Universal

**Contras**:

- ❌ ~2-3x maior que binário
- ❌ Parse lento

**Tamanho típico**: 120-150 bytes

```json
{
  "mac": "80:F1:B2:50:31:34",
  "distance_mm": 2450,
  "vcc_bat_mv": 5000,
  "rssi": -50
}
```

---

### Binário Compacto (PROPOSTO)

**Prós**:

- ✅ 16-32 bytes apenas
- ✅ Parse instantâneo
- ✅ Menos overhead

**Contras**:

- ⚠️ Não human-readable
- ⚠️ Debug mais difícil

**Tamanho**: 16-32 bytes

```c
struct { uint16_t magic; uint8_t mac[6]; int16_t dist; ... }
```

---

### MessagePack (Alternativa)

**Prós**:

- ✅ JSON-like mas binário
- ✅ ~40% menor que JSON
- ✅ Schemas

**Contras**:

- ⚠️ Biblioteca extra

**Uso**: ⚠️ **Overkill para AGUADA**

---

### Protobuf (Google)

**Prós**:

- ✅ Schemas tipados
- ✅ Backward compatibility
- ✅ Multi-linguagem

**Contras**:

- ❌ Overhead compile-time
- ❌ Biblioteca pesada (~50KB)

**Uso**: ❌ **Muito para ESP32**

---

## 4. Power Management

### Always-On (ATUAL)

| Modo       | Consumo | Autonomia 2Ah |
| ---------- | ------- | ------------- |
| CPU 160MHz | 80mA    | 25h           |

**Uso**: Gateway sempre ligado

---

### Light Sleep (PROPOSTO)

| Modo         | Consumo   | Autonomia 2Ah |
| ------------ | --------- | ------------- |
| Light Sleep  | 0.8mA     | 104 dias\*    |
| Acordado 10% | 8mA médio | 10 dias       |

\*teórico, na prática ~5-7 dias

**Uso**: ✅ **Sensors com 5V DC**

---

### Deep Sleep

| Modo           | Consumo     | Autonomia 2Ah |
| -------------- | ----------- | ------------- |
| Deep Sleep     | 10µA        | 22 anos\*     |
| Wake up 2s/30s | 0.5mA médio | 166 dias      |

\*teórico

**Uso**: ✅ **Sensors com bateria**

---

### Modem Sleep

| Modo        | Consumo | Autonomia 2Ah |
| ----------- | ------- | ------------- |
| Modem Sleep | 20-30mA | 2-3 dias      |

**Uso**: ⚠️ **Híbrido, pouco ganho**

---

## 5. OTA Strategies

### Rolling Update (PROPOSTO)

```
Node 1 → Wait 24h → Node 2 → Wait 24h → ...
```

**Prós**:

- ✅ Reduz risco
- ✅ Rollback fácil

**Contras**:

- ⚠️ Lento (5 nodes = 5 dias)

---

### Blue-Green Deployment

```
Versão A (produção) ← ─ ─ ┐
Versão B (staging)  ← ─ ─ ┘ Switch
```

**Uso**: ⚠️ **Requer 2x nodes (caro)**

---

### Canary Deployment (RECOMENDADO)

```
1 node (canary) → Monitor 48h → Rollout 100%
```

**Prós**:

- ✅ Detecta bugs cedo
- ✅ Impacto mínimo

---

## 6. Security

### ESP-NOW PMK (PROPOSTO)

**Características**:

- ✅ AES-128 encryption
- ✅ Zero overhead
- ✅ Nativo ESP-IDF

```c
uint8_t pmk[16] = {...};
esp_now_set_pmk(pmk);
```

---

### HMAC Signing

**Características**:

- ✅ Autenticação de mensagem
- ✅ Detecta tampering
- ⚠️ +32 bytes overhead

```c
mbedtls_sha256_hmac(key, data, hmac);
```

---

### TLS/HTTPS (Gateway)

**Uso**: ✅ **Gateway → Backend (já implementado?)**

---

## 7. Decisão Final

| Componente       | Escolha                  | Razão                           |
| ---------------- | ------------------------ | ------------------------------- |
| **Framework**    | ESP-IDF 5.3 LTS          | Controle, performance, OTA      |
| **Build System** | ESP-IDF nativo           | Simplicidade                    |
| **RTOS**         | FreeRTOS                 | Nativo, otimizado               |
| **Comunicação**  | ESP-NOW                  | Baixa latência, baixo consumo   |
| **Formato**      | JSON (v1) + Binário (v2) | Compatibilidade + eficiência    |
| **Power Mode**   | Light Sleep              | 5x autonomia, fácil implementar |
| **OTA**          | Canary deployment        | Seguro, testado                 |
| **Security**     | ESP-NOW PMK              | Simples, eficaz                 |
| **Monitoring**   | Prometheus metrics       | Standard industrial             |

---

## 8. Roadmap de Adoção

### Fase 1: Refatoração (Semana 1-2)

- [x] Proposta criada ✅
- [ ] Estrutura de componentes
- [ ] Tasks FreeRTOS
- [ ] Testes unitários

### Fase 2: Otimização (Semana 3-4)

- [ ] Light Sleep
- [ ] Health monitoring
- [ ] Benchmarks

### Fase 3: Produção (Semana 5-6)

- [ ] OTA manager
- [ ] Security (PMK)
- [ ] Deploy piloto

### Fase 4: Expansão (Semana 7+)

- [ ] Rollout total
- [ ] Documentação final
- [ ] Training equipe

---

**Status**: 📋 **PROPOSTA APROVADA** - Aguardando implementação

**Próximo passo**: Criar branch `firmware-v2` e começar Fase 1
