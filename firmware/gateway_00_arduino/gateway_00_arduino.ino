/**
 * AGUADA - Gateway v2.0 (Arduino)
 * ESP-NOW Receiver → MQTT Publisher
 * ESP32-C3 SuperMini
 */

#include <WiFi.h>
#include <esp_now.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ============================================================================
// CONFIGURAÇÕES
// ============================================================================

// WiFi
const char* WIFI_SSID = "luciano";
const char* WIFI_PASSWORD = "Luciano19852012";

// MQTT
const char* MQTT_BROKER = "192.168.0.117";
const int MQTT_PORT = 1883;
const char* MQTT_TOPIC = "aguada/telemetry";

// Hardware - ESP32-C3 SuperMini
#define LED_BUILTIN 8  // LED onboard (heartbeat)

// ============================================================================
// GLOBALS
// ============================================================================

WiFiClient espClient;
PubSubClient mqttClient(espClient);

unsigned long lastWifiCheck = 0;
unsigned long lastMqttCheck = 0;
unsigned long lastHeartbeat = 0;
const unsigned long WIFI_CHECK_INTERVAL = 5000;
const unsigned long MQTT_CHECK_INTERVAL = 5000;
const unsigned long HEARTBEAT_INTERVAL = 1000;  // 1 Hz

bool ledState = false;

// ============================================================================
// ESP-NOW Callback (ESP32 Arduino v5.5+)
// ============================================================================

void onDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  // Converter dados para string
  char jsonStr[251];
  memcpy(jsonStr, incomingData, len);
  jsonStr[len] = '\0';
  
  // DEBUG: Mostrar dados brutos recebidos
  Serial.println();
  Serial.println("╔════════════════════════════════════════════════════╗");
  Serial.printf("║ ✓ ESP-NOW recebido de: %02X:%02X:%02X:%02X:%02X:%02X (%d bytes)\n", 
                recv_info->src_addr[0], recv_info->src_addr[1], recv_info->src_addr[2],
                recv_info->src_addr[3], recv_info->src_addr[4], recv_info->src_addr[5], len);
  Serial.println("╠════════════════════════════════════════════════════╣");
  Serial.print("║ Dados brutos: ");
  Serial.println(jsonStr);
  Serial.println("╚════════════════════════════════════════════════════╝");
  
  // Parse JSON para validação
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, jsonStr);
  
  if (error) {
    Serial.print("✗ JSON inválido: ");
    Serial.println(error.c_str());
    Serial.print("  Raw hex: ");
    for (int i = 0; i < len && i < 50; i++) {
      Serial.printf("%02X ", incomingData[i]);
    }
    Serial.println();
    return;
  }
  
  // Extrair campos
  const char* node_mac = doc["mac"];
  const char* type = doc["type"];
  int value = doc["value"];
  int rssi = doc["rssi"];
  
  Serial.println();
  Serial.println("╔════════════════════════════════════════════════════╗");
  Serial.printf("║ ✓ JSON válido de: %s\n", node_mac);
  Serial.println("╠════════════════════════════════════════════════════╣");
  Serial.printf("║   Type:    %s\n", type);
  Serial.printf("║   Value:   %d\n", value);
  Serial.printf("║   RSSI:    %d dBm\n", rssi);
  Serial.println("╚════════════════════════════════════════════════════╝");
  
  // Publicar no MQTT se conectado
  if (mqttClient.connected()) {
    if (mqttClient.publish(MQTT_TOPIC, jsonStr)) {
      Serial.println("→ Publicado no MQTT com sucesso!");
    } else {
      Serial.println("✗ Falha ao publicar no MQTT");
    }
  } else {
    Serial.println("✗ MQTT desconectado. Dados não enviados.");
  }
}

// ============================================================================
// WiFi Functions
// ============================================================================

void connectWiFi() {
  Serial.println();
  Serial.println("Conectando ao WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("✓ WiFi conectado!");
    Serial.print("  IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("  MAC: ");
    Serial.println(WiFi.macAddress());
  } else {
    Serial.println();
    Serial.println("✗ Falha ao conectar WiFi");
  }
}

void maintainWiFi() {
  if (millis() - lastWifiCheck > WIFI_CHECK_INTERVAL) {
    lastWifiCheck = millis();
    
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi desconectado. Reconectando...");
      connectWiFi();
    }
  }
}

// ============================================================================
// MQTT Functions
// ============================================================================

void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.println("Conectando ao MQTT...");
    
    String clientId = "AGUADA_GATEWAY_";
    clientId += String(random(0xffff), HEX);
    
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("✓ MQTT conectado!");
    } else {
      Serial.print("✗ Falha MQTT, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Tentando novamente em 5s...");
      delay(5000);
    }
  }
}

void maintainMQTT() {
  if (millis() - lastMqttCheck > MQTT_CHECK_INTERVAL) {
    lastMqttCheck = millis();
    
    if (!mqttClient.connected()) {
      Serial.println("MQTT desconectado. Reconectando...");
      connectMQTT();
    }
  }
  
  mqttClient.loop();
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Configurar LED builtin
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  // Blink inicial (3x rápido)
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
  
  Serial.println();
  Serial.println("╔═══════════════════════════════════════════════════════════╗");
  Serial.println("║           AGUADA Gateway v2.0 (Arduino)                  ║");
  Serial.println("║           ESP-NOW → MQTT Bridge                          ║");
  Serial.println("╚═══════════════════════════════════════════════════════════╝");
  Serial.println();
  
  // Conectar WiFi
  connectWiFi();
  
  // CRÍTICO: Configurar canal ESP-NOW (deve ser 1 para alinhar com nodes)
  WiFi.setChannel(1, WIFI_SECOND_CHAN_NONE);
  delay(100);
  Serial.println("✓ Canal WiFi definido: 1");
  
  // Inicializar ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("✗ Erro ao inicializar ESP-NOW");
    ESP.restart();
  }
  Serial.println("✓ ESP-NOW inicializado");
  
  // Registrar callback de recebimento
  esp_now_register_recv_cb(onDataRecv);
  Serial.println("✓ Callback ESP-NOW registrado");
  
  // Adicionar peer broadcast (para receber de qualquer node)
  esp_now_peer_info_t peer;
  memset(&peer, 0, sizeof(esp_now_peer_info_t));
  peer.channel = 1;
  peer.encrypt = false;
  memset(peer.peer_addr, 0xff, 6);  // MAC broadcast FF:FF:FF:FF:FF:FF
  esp_now_add_peer(&peer);
  Serial.println("✓ Peer broadcast adicionado");
  
  // Configurar MQTT
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  connectMQTT();
  
  Serial.println();
  Serial.println("✓ Gateway inicializado e pronto!");
  Serial.println("  - Aguardando dados dos sensores...");
  Serial.println();
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
  // Heartbeat LED (1 Hz)
  if (millis() - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    lastHeartbeat = millis();
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
  }
  
  maintainWiFi();
  maintainMQTT();
  
  delay(10);
}
