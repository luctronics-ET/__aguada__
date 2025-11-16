/**
 * AGUADA - Gateway WiFi
 * Firmware para ESP32-C3 SuperMini (Gateway)
 * 
 * Gateway: gateway_00
 * Função: Receber telemetria de múltiplos nodes e encaminhar para servidor
 * 
 * Funcionalidades:
 * - Recepção de telemetria de nodes via Serial/BLE
 * - Fila de mensagens com retry automático
 * - Registro de nodes online/offline
 * - Envio via MQTT (QoS 1) com fallback HTTP
 * - Status periódico do gateway
 * - Watchdog timer
 * - Estatísticas em tempo real
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include <ArduinoJson.h>

#include "config_pins.h"
#include "queue_manager.h"
#include "node_registry.h"
#include "gateway_io.h"

// =============================================================================
// OBJETOS GLOBAIS
// =============================================================================

WiFiClient espClient;

QueueManager queueManager(QUEUE_SIZE);
NodeRegistry nodeRegistry;
GatewayIO gatewayIO(MQTT_BROKER, MQTT_PORT, MQTT_USER, MQTT_PASS, 
                    MQTT_TOPIC_BASE, MQTT_TOPIC_STATUS,
                    HTTP_SERVER, HTTP_ENDPOINT);

// =============================================================================
// VARIÁVEIS GLOBAIS
// =============================================================================

uint32_t lastHeartbeat = 0;
uint32_t lastQueueCheck = 0;
uint32_t lastRegistryCheck = 0;

// =============================================================================
// FUNÇÕES AUXILIARES
// =============================================================================

/**
 * Conectar WiFi
 */
bool connectWiFi() {
  Serial.println("[WiFi] Conectando...");
  Serial.printf("[WiFi] SSID: %s\n", WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  uint8_t attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] ✓ Conectado!");
    Serial.printf("[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("[WiFi] MAC: %s\n", WiFi.macAddress().c_str());
    Serial.printf("[WiFi] RSSI: %d dBm\n", WiFi.RSSI());
    
    digitalWrite(LED_WIFI, HIGH);
    return true;
  } else {
    Serial.println("\n[WiFi] ✗ Falha ao conectar");
    digitalWrite(LED_WIFI, LOW);
    return false;
  }
}

/**
 * Manter WiFi conectado
 */
void maintainWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_WIFI, LOW);
    Serial.println("[WiFi] Conexão perdida. Tentando reconectar...");
    connectWiFi();
  } else {
    digitalWrite(LED_WIFI, HIGH);
  }
}

/**
 * Processar mensagem recebida de node (via Serial)
 * Formato esperado: JSON puro da telemetria
 */
void processNodeMessage(const String& message) {
  Serial.println("[GATEWAY] Mensagem recebida:");
  Serial.println(message);
  
  // Parse JSON para extrair MAC do node
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.printf("[GATEWAY] ✗ JSON inválido: %s\n", error.c_str());
    return;
  }
  
  const char* nodeMac = doc["node_mac"];
  if (!nodeMac) {
    Serial.println("[GATEWAY] ✗ MAC do node não encontrado");
    return;
  }
  
  // Atualizar registro do node
  int rssi = doc["meta"]["rssi"] | -100;
  nodeRegistry.updateLastSeen(nodeMac, rssi);
  
  // Enfileirar para envio
  queueManager.enqueue(nodeMac, message, false);
}

/**
 * Ler Serial (comunicação com nodes via Serial)
 */
void checkSerial() {
  if (Serial.available()) {
    String message = Serial.readStringUntil('\n');
    message.trim();
    
    if (message.length() > 0 && message.startsWith("{")) {
      processNodeMessage(message);
    }
  }
}

/**
 * Registrar nodes conhecidos (hardcoded para este exemplo)
 */
void registerKnownNodes() {
  nodeRegistry.registerNode("AA:BB:CC:DD:EE:01", "node_01", "SEN_CAV_01", "res_incendio");
  nodeRegistry.registerNode("AA:BB:CC:DD:EE:03", "node_03", "SEN_IE01_01", "cisterna_ie01");
  nodeRegistry.registerNode("AA:BB:CC:DD:EE:04", "node_04", "SEN_CON_01", "res_cons");
  
  Serial.println("[GATEWAY] Nodes conhecidos registrados");
}

/**
 * Heartbeat LED
 */
void updateHeartbeat() {
  uint32_t now = millis();
  
  if (now - lastHeartbeat > 1000) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    lastHeartbeat = now;
  }
}

/**
 * Mostrar estatísticas
 */
void printStats() {
  Serial.println("\n╔═══════════════════════════════════════════════════════════╗");
  Serial.println("║           AGUADA GATEWAY - ESTATÍSTICAS                  ║");
  Serial.println("╠═══════════════════════════════════════════════════════════╣");
  Serial.printf("║ Uptime:          %lu segundos\n", millis() / 1000);
  Serial.printf("║ WiFi:            %s (RSSI: %d dBm)\n", 
                WiFi.status() == WL_CONNECTED ? "CONECTADO" : "DESCONECTADO",
                WiFi.RSSI());
  Serial.printf("║ MQTT:            %s\n", 
                gatewayIO.isMQTTConnected() ? "CONECTADO" : "DESCONECTADO");
  Serial.println("╠═══════════════════════════════════════════════════════════╣");
  Serial.printf("║ Fila:            %d mensagens\n", queueManager.size());
  Serial.printf("║ Recebidas:       %lu\n", queueManager.getMessagesReceived());
  Serial.printf("║ Enviadas:        %lu\n", queueManager.getMessagesSent());
  Serial.printf("║ Descartadas:     %lu\n", queueManager.getMessagesDropped());
  Serial.println("╠═══════════════════════════════════════════════════════════╣");
  Serial.printf("║ Nodes Total:     %d\n", nodeRegistry.getTotalCount());
  Serial.printf("║ Nodes Online:    %d\n", nodeRegistry.getOnlineCount());
  Serial.println("╚═══════════════════════════════════════════════════════════╝\n");
}

// =============================================================================
// SETUP
// =============================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n");
  Serial.println("╔═══════════════════════════════════════════════════════════╗");
  Serial.println("║           AGUADA - Gateway WiFi                           ║");
  Serial.println("╠═══════════════════════════════════════════════════════════╣");
  Serial.printf("║ Gateway:         %s\n", GATEWAY_NAME);
  Serial.printf("║ Max Nodes:       %d\n", MAX_NODES);
  Serial.printf("║ Queue Size:      %d\n", QUEUE_SIZE);
  Serial.println("╚═══════════════════════════════════════════════════════════╝\n");
  
  // Inicializar LEDs
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_WIFI, OUTPUT);
  pinMode(LED_MQTT, OUTPUT);
  
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(LED_WIFI, LOW);
  digitalWrite(LED_MQTT, LOW);
  
  // Blink de inicialização
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
  
  // Registrar nodes conhecidos
  registerKnownNodes();
  
  // Conectar WiFi
  if (!connectWiFi()) {
    Serial.println("[SETUP] Falha ao conectar WiFi. Reiniciando em 5s...");
    delay(5000);
    ESP.restart();
  }
  
  // Inicializar Gateway I/O
  gatewayIO.begin(&espClient, &queueManager);
  gatewayIO.connectMQTT();
  
  // Configurar watchdog
  esp_task_wdt_init(WATCHDOG_TIMEOUT_SEC, true);
  esp_task_wdt_add(NULL);
  
  Serial.println("\n[SETUP] ✓ Gateway inicializado e pronto!\n");
  
  printStats();
  nodeRegistry.printRegistry();
}

// =============================================================================
// LOOP
// =============================================================================

void loop() {
  uint32_t now = millis();
  
  // Reset watchdog
  esp_task_wdt_reset();
  
  // Heartbeat
  updateHeartbeat();
  
  // Manter WiFi
  maintainWiFi();
  
  // Manter MQTT
  gatewayIO.maintain();
  
  // Atualizar LED MQTT
  digitalWrite(LED_MQTT, gatewayIO.isMQTTConnected() ? HIGH : LOW);
  
  // Ler mensagens de nodes via Serial
  checkSerial();
  
  // Processar fila de mensagens (a cada 1 segundo)
  if (now - lastQueueCheck > QUEUE_CHECK_INTERVAL_MS) {
    gatewayIO.processQueue();
    lastQueueCheck = now;
  }
  
  // Verificar nodes offline (a cada 30 segundos)
  if (now - lastRegistryCheck > 30000) {
    nodeRegistry.checkOfflineNodes();
    lastRegistryCheck = now;
  }
  
  // Comandos via Serial (para debug)
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    if (cmd == "stats") {
      printStats();
    } else if (cmd == "nodes") {
      nodeRegistry.printRegistry();
    } else if (cmd == "clear") {
      queueManager.clear();
      Serial.println("[CMD] Fila limpa");
    } else if (cmd == "restart") {
      Serial.println("[CMD] Reiniciando gateway...");
      delay(1000);
      ESP.restart();
    } else if (cmd.startsWith("{")) {
      // JSON de telemetria
      processNodeMessage(cmd);
    }
  }
  
  delay(10);
}
