# AGUADA Firmware Evolution Plan

## Documento de Planejamento - Node OTA Unificado

**Data**: 2025-12-06  
**Status**: Planejamento

---

## 1. Visão Geral da Evolução

### Arquitetura Proposta (Complexa - Para Referência Futura)

```
┌─────────────────────────────────────────────────────────────────┐
│                    ESP32-C3 Node Unificado                       │
│                                                                  │
│  Modos de Operação:                                              │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐                 │
│  │   SENSOR   │  │   RELAY    │  │  GATEWAY   │                 │
│  │ ESP-NOW TX │  │ TX+RX+WiFi │  │ RX+WiFi    │                 │
│  └────────────┘  └────────────┘  └────────────┘                 │
│                                                                  │
│  Features Comuns:                                                │
│  ├── WiFi AP sempre ativo (config/OTA)                          │
│  ├── Web Server (status, config, OTA)                           │
│  ├── mDNS (aguada-xxxx.local)                                   │
│  ├── NVS (persistência de config)                               │
│  └── Sensores (ultrasonic, sound, valves, voltage)              │
└─────────────────────────────────────────────────────────────────┘
```

### Fases de Desenvolvimento Planejadas

| Fase | Nome           | Features                              | Status    |
| ---- | -------------- | ------------------------------------- | --------- |
| 1    | node_v1_basic  | WiFi AP + Web + Sensores              | Planejado |
| 2    | node_v2_espnow | + ESP-NOW TX                          | Planejado |
| 3    | node_v3_ota    | + OTA via web                         | Planejado |
| 4    | node_v4_relay  | + WiFi STA + ESP-NOW RX + HTTP Client | Planejado |

---

## 2. Componentes e Frameworks Analisados

### ESP-IDF Nativos (Recomendados)

| Exemplo            | Localização                                     | Uso                      |
| ------------------ | ----------------------------------------------- | ------------------------ |
| softap_sta         | `examples/wifi/softap_sta`                      | WiFi AP + STA simultâneo |
| captive_portal     | `examples/protocols/http_server/captive_portal` | Portal de config         |
| simple             | `examples/protocols/http_server/simple`         | HTTP server básico       |
| espnow             | `examples/wifi/espnow`                          | ESP-NOW TX/RX            |
| simple_ota_example | `examples/system/ota/simple_ota_example`        | OTA via HTTPS            |
| restful_server     | `examples/protocols/http_server/restful_server` | REST API + Vue.js        |

### Componentes do Registry (Externos)

| Componente         | Versão | Features                                 |
| ------------------ | ------ | ---------------------------------------- |
| esp32-wifi-manager | 0.0.3  | WiFi AP, Captive Portal, NVS, Web Config |
| mdns               | 1.9.1  | Descoberta de serviços                   |
| littlefs           | 1.20.3 | Filesystem para web pages                |

### Decisão: Usar ESP-IDF Nativo

- Menor dependência externa
- Compatibilidade garantida com IDF 6.x
- Maior controle e customização
- Melhor para aprendizado

---

## 3. Arquitetura Modular Proposta

```
firmware/node_unified/
├── components/
│   ├── aguada_wifi/       # AP + STA + config
│   ├── aguada_web/        # HTTP server + API
│   ├── aguada_sensors/    # Ultrasonic, digital inputs
│   ├── aguada_espnow/     # ESP-NOW TX/RX
│   ├── aguada_ota/        # OTA updates
│   └── aguada_config/     # NVS persistence
├── main/
│   ├── main.c
│   └── CMakeLists.txt
├── html/
│   └── index.html         # Web interface (embedded)
├── CMakeLists.txt
├── Kconfig.projbuild
└── sdkconfig.defaults
```

---

## 4. Coexistência ESP-NOW + WiFi AP

### Requisitos Técnicos

- ESP-NOW e WiFi AP DEVEM usar o mesmo canal
- Modo WiFi: `WIFI_MODE_AP` ou `WIFI_MODE_APSTA`
- Canal fixo (ex: 11) para toda a rede AGUADA

### Código Base

```c
// Inicialização correta para coexistência
esp_wifi_set_mode(WIFI_MODE_APSTA);

wifi_config_t ap_config = {
    .ap = {
        .ssid = "aguada-XXXX",
        .channel = 11,  // Mesmo canal do ESP-NOW
        .max_connection = 4,
    }
};
esp_wifi_set_config(WIFI_IF_AP, &ap_config);

// ESP-NOW usa o mesmo canal automaticamente
esp_now_init();
```

---

## 5. Web Interface Leve

### Características

- Single HTML file
- Inline CSS/JS
- < 10KB total
- Responsivo (mobile-first)
- Auto-refresh via fetch()

### Endpoints API

```
GET  /api/status    - Leituras atuais
GET  /api/config    - Configuração atual
POST /api/config    - Atualizar config
POST /api/ota       - Upload firmware
GET  /api/info      - Info do sistema
POST /api/reboot    - Reiniciar
```

---

## 6. Configuração Hierárquica

```c
typedef struct {
    // Identificação
    char node_id[16];           // "RCON", "RCAV", etc.
    uint8_t mode;               // SENSOR, RELAY, GATEWAY

    // Comunicação
    uint8_t gateway_mac[6];     // MAC do destino ESP-NOW
    uint8_t espnow_channel;     // Canal (default: 11)

    // WiFi (para modo RELAY/GATEWAY)
    char wifi_ssid[32];
    char wifi_pass[64];
    char backend_url[128];

    // Sensores
    uint16_t interval_ms;       // Intervalo de leitura (default: 30000)
    uint16_t deadband_cm;       // Threshold (default: 2)

    // Reservatório
    uint16_t altura_total_cm;   // Altura do reservatório
    uint16_t offset_sensor_cm;  // Offset do sensor
} node_config_t;
```

---

## 7. Estimativa de Tempo

| Fase                              | Tempo Estimado | Complexidade |
| --------------------------------- | -------------- | ------------ |
| Fase 1 (WiFi AP + Web + Sensores) | 2-3 horas      | Média        |
| Fase 2 (ESP-NOW TX)               | 1 hora         | Baixa        |
| Fase 3 (OTA)                      | 1-2 horas      | Média        |
| Fase 4 (Relay completo)           | 2-3 horas      | Alta         |
| **Total**                         | **6-9 horas**  | -            |

---

## 8. Hardware Necessário

### Por Node (ESP32-C3 SuperMini)

- 1× ESP32-C3 SuperMini
- 1× AJ-SR04M (ultrasonic waterproof)
- 1× Sensor de som (opcional)
- 2× Reed switch (válvulas - opcional)
- Fonte 5V

### Gateway/Relay

- Mesmo hardware do node
- Posicionado perto do roteador WiFi

---

## 9. Próximos Passos

1. [ ] Criar estrutura base `firmware/node_unified/`
2. [ ] Implementar Fase 1 (WiFi AP + Web + Sensores)
3. [ ] Testar em ESP32-C3
4. [ ] Adicionar ESP-NOW (Fase 2)
5. [ ] Adicionar OTA (Fase 3)
6. [ ] Implementar modo Relay (Fase 4)

---

## 10. Referências

- ESP-IDF 6.x Documentation
- ESP-NOW Programming Guide
- ESP32-C3 Technical Reference Manual
- AGUADA docs/RULES.md

---

**Nota**: Este documento será atualizado conforme o desenvolvimento avança.
