// ============================================================
//  Lift Alarm Dashboard — ESP32 Firmware (WebSocket Edition)
//  Libraries required (install via Arduino Library Manager):
//    • ESPAsyncWebServer  (me-no-dev)
//    • AsyncTCP           (me-no-dev)   ← ESPAsyncWebServer dependency
//    • PubSubClient       (Nick O'Leary)
//    • ArduinoJson        (Benoit Blanchon)  v6 or v7
//  Board: ESP32 Dev Module  |  Partition: Default 4MB with SPIFFS
// ============================================================

#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <ESPAsyncWebServer.h>     // replaces WebServer
#include <ArduinoJson.h>

// ─────────────────────────────────────────────────────────────
//  CONFIGURATION  — edit these blocks before flashing
// ─────────────────────────────────────────────────────────────

// WiFi credentials
const char* WIFI_SSID     = "Nethmina's Galaxy A53 5G";
const char* WIFI_PASSWORD = "12345678";

// MQTT broker
const char* MQTT_SERVER   = "broker.hivemq.com";
const int   MQTT_PORT     = 1883;
const char* MQTT_CLIENT_ID = "ESP32_LiftDash_v2";

// ── Supabase REST logging endpoint ────────────────────────────
// Replace with your own project URL + table name + anon key.
// e.g. https://<project>.supabase.co/rest/v1/lift_events
const char* DB_ENDPOINT   = "https://<YOUR_PROJECT>.supabase.co/rest/v1/lift_events";
const char* DB_ANON_KEY   = "Bearer <YOUR_SUPABASE_ANON_KEY>";

// ─────────────────────────────────────────────────────────────
//  PIN DEFINITIONS
// ─────────────────────────────────────────────────────────────
#define PIN_BUZZER     2
#define PIN_LED_POWER  19
#define PIN_LED_WIFI   21

// ─────────────────────────────────────────────────────────────
//  LIFT STATE  — [zone 0..2][lift 0..1]
//  Maps to:
//    zone 0 → Lotus Side  → lift1, lift2
//    zone 1 → Duke Side   → lift3, lift4
//    zone 2 → OTS         → lift5, lift6
// ─────────────────────────────────────────────────────────────
bool liftLatched[3][2] = {
  {false, false},
  {false, false},
  {false, false}
};

// Zone/lift → human-readable strings (for DB payload)
const char* ZONE_NAMES[3] = {"Lotus Side", "Duke Side", "OTS"};
// liftNumber[zone][slot] = human lift number (1-6)
const int LIFT_NUMBER[3][2] = {{1,2},{3,4},{5,6}};

// ─────────────────────────────────────────────────────────────
//  UNO NODE HEALTH
// ─────────────────────────────────────────────────────────────
unsigned long lastSeenUNO[3]      = {0, 0, 0};
const unsigned long UNO_TIMEOUT_MS = 15000UL;

// ─────────────────────────────────────────────────────────────
//  ALARM LATCH
// ─────────────────────────────────────────────────────────────
bool alarmLatched = false;

// ─────────────────────────────────────────────────────────────
//  BUZZER (non-blocking pattern)
// ─────────────────────────────────────────────────────────────
const unsigned long BUZZER_ON_MS  = 1000UL;
const unsigned long BUZZER_OFF_MS = 2000UL;
unsigned long lastBuzzerChange    = 0;
bool buzzerState                  = false;

// ─────────────────────────────────────────────────────────────
//  WiFi LED
// ─────────────────────────────────────────────────────────────
const unsigned long WIFI_BLINK_MS = 500UL;
unsigned long lastWifiBlink       = 0;
bool wifiLedState                 = false;

// ─────────────────────────────────────────────────────────────
//  SERVER / WS / MQTT OBJECTS
// ─────────────────────────────────────────────────────────────
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");          // WebSocket endpoint at /ws

WiFiClient   espClient;
PubSubClient mqttClient(espClient);

// ─────────────────────────────────────────────────────────────
//  FORWARD DECLARATIONS
// ─────────────────────────────────────────────────────────────
void broadcastState();
void logEventToDatabase(int zone, int slot, const char* status);
String buildFullStateJson();

// ─────────────────────────────────────────────────────────────
//  HELPER — recompute alarmLatched from liftLatched[][]
// ─────────────────────────────────────────────────────────────
void recomputeAlarm() {
  alarmLatched = false;
  for (int z = 0; z < 3; z++)
    for (int s = 0; s < 2; s++)
      if (liftLatched[z][s]) alarmLatched = true;
}

// ─────────────────────────────────────────────────────────────
//  HELPER — build complete JSON state for initial WS sync
// ─────────────────────────────────────────────────────────────
String buildFullStateJson() {
  // {"type":"full","lift":[[0,0],[0,0],[0,0]],"uno":[1,1,0]}
  StaticJsonDocument<256> doc;
  doc["type"] = "full";

  JsonArray liftArr = doc.createNestedArray("lift");
  for (int z = 0; z < 3; z++) {
    JsonArray row = liftArr.createNestedArray();
    for (int s = 0; s < 2; s++)
      row.add(liftLatched[z][s] ? 1 : 0);
  }

  JsonArray unoArr = doc.createNestedArray("uno");
  for (int z = 0; z < 3; z++) {
    bool active = (lastSeenUNO[z] != 0) &&
                  ((millis() - lastSeenUNO[z]) <= UNO_TIMEOUT_MS);
    unoArr.add(active ? 1 : 0);
  }

  String out;
  serializeJson(doc, out);
  return out;
}

// ─────────────────────────────────────────────────────────────
//  BROADCAST — push a single lift-change event to all WS clients
// ─────────────────────────────────────────────────────────────
void broadcastLiftEvent(int zone, int slot, const char* status) {
  // {"type":"lift","zone":0,"slot":0,"lift_id":1,"status":"ON"}
  StaticJsonDocument<128> doc;
  doc["type"]    = "lift";
  doc["zone"]    = zone;
  doc["slot"]    = slot;
  doc["lift_id"] = LIFT_NUMBER[zone][slot];
  doc["status"]  = status;

  String payload;
  serializeJson(doc, payload);
  ws.textAll(payload);
  Serial.printf("[WS] Broadcast: %s\n", payload.c_str());
}

// ─────────────────────────────────────────────────────────────
//  BROADCAST — push UNO health change to all WS clients
// ─────────────────────────────────────────────────────────────
void broadcastUnoEvent(int zone, bool active) {
  // {"type":"uno","zone":0,"active":1}
  StaticJsonDocument<64> doc;
  doc["type"]   = "uno";
  doc["zone"]   = zone;
  doc["active"] = active ? 1 : 0;

  String payload;
  serializeJson(doc, payload);
  ws.textAll(payload);
}

// ─────────────────────────────────────────────────────────────
//  DATABASE LOGGING — non-blocking HTTP POST to Supabase/Firebase
//  This runs inside the MQTT callback on the main core; keep it
//  as fast as possible (use HTTPClient in async-like pattern).
// ─────────────────────────────────────────────────────────────
void logEventToDatabase(int zone, int slot, const char* status) {
  if (WiFi.status() != WL_CONNECTED) return;

  // Build ISO-8601 timestamp (epoch-based; requires NTP for accuracy,
  // falls back to millis() offset if NTP not configured)
  char tsStr[32];
  // For production, sync NTP in setup() and use configTime() + getLocalTime()
  snprintf(tsStr, sizeof(tsStr), "%lu", millis());

  // Build JSON body
  StaticJsonDocument<256> doc;
  doc["lift_id"]   = LIFT_NUMBER[zone][slot];
  doc["zone"]      = ZONE_NAMES[zone];
  doc["status"]    = status;  // "ON" or "OFF"
  doc["timestamp"] = tsStr;   // replace with ISO string after NTP sync

  String body;
  serializeJson(doc, body);

  HTTPClient http;
  http.begin(DB_ENDPOINT);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", DB_ANON_KEY);
  http.addHeader("apikey", DB_ANON_KEY + 7); // strip "Bearer " prefix for Supabase apikey header
  http.addHeader("Prefer", "return=minimal"); // Supabase: don't echo body back

  int httpCode = http.POST(body);
  if (httpCode > 0) {
    Serial.printf("[DB] POST %s → HTTP %d\n", DB_ENDPOINT, httpCode);
  } else {
    Serial.printf("[DB] POST failed: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

// ─────────────────────────────────────────────────────────────
//  MQTT CALLBACK  (called from PubSubClient inside loop())
// ─────────────────────────────────────────────────────────────
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Decode payload
  char msg[16] = {0};
  unsigned int cpLen = min(length, (unsigned int)15);
  memcpy(msg, payload, cpLen);
  msg[cpLen] = '\0';
  // Trim trailing whitespace
  for (int i = cpLen - 1; i >= 0 && (msg[i] == ' ' || msg[i] == '\n' || msg[i] == '\r'); i--)
    msg[i] = '\0';

  String t = String(topic);
  Serial.printf("[MQTT] topic=%s  msg=%s\n", topic, msg);

  // ── Update UNO "last seen" on ANY message from that UNO ──
  int unoZone = -1;
  if      (t.startsWith("test/relay/uno1/")) unoZone = 0;
  else if (t.startsWith("test/relay/uno2/")) unoZone = 1;
  else if (t.startsWith("test/relay/uno3/")) unoZone = 2;

  if (unoZone >= 0) {
    bool wasActive = (lastSeenUNO[unoZone] != 0) &&
                     ((millis() - lastSeenUNO[unoZone]) <= UNO_TIMEOUT_MS);
    lastSeenUNO[unoZone] = millis();
    if (!wasActive) {
      // Transition: dead → alive — push UNO health event
      broadcastUnoEvent(unoZone, true);
    }
  }

  // ── Determine zone + slot from topic ──
  int zone = -1, slot = -1;
  if      (t == "test/relay/uno1/lift1") { zone = 0; slot = 0; }
  else if (t == "test/relay/uno1/lift2") { zone = 0; slot = 1; }
  else if (t == "test/relay/uno2/lift3") { zone = 1; slot = 0; }
  else if (t == "test/relay/uno2/lift4") { zone = 1; slot = 1; }
  else if (t == "test/relay/uno3/lift5") { zone = 2; slot = 0; }
  else if (t == "test/relay/uno3/lift6") { zone = 2; slot = 1; }

  if (zone < 0) return; // Not a lift topic (e.g., /status)

  // ── Apply state change (latch ON, allow OFF reset) ──
  bool newState = (strcmp(msg, "ON") == 0);
  bool oldState = liftLatched[zone][slot];

  if (newState && !oldState) {
    // Transition OFF → ON
    liftLatched[zone][slot] = true;
    alarmLatched = true;
    // ① WS broadcast
    broadcastLiftEvent(zone, slot, "ON");
    // ② Database log
    logEventToDatabase(zone, slot, "ON");
  } else if (!newState && oldState) {
    // Transition ON → OFF  (MQTT-driven reset, e.g. UNO sends "OFF")
    liftLatched[zone][slot] = false;
    recomputeAlarm();
    broadcastLiftEvent(zone, slot, "OFF");
    logEventToDatabase(zone, slot, "OFF");
  }
}

// ─────────────────────────────────────────────────────────────
//  MQTT RECONNECT  (non-blocking friendly)
// ─────────────────────────────────────────────────────────────
unsigned long lastMqttAttempt = 0;
const unsigned long MQTT_RETRY_MS = 5000UL;

void tryReconnectMQTT() {
  if (millis() - lastMqttAttempt < MQTT_RETRY_MS) return;
  lastMqttAttempt = millis();

  Serial.print("[MQTT] Connecting...");
  if (mqttClient.connect(MQTT_CLIENT_ID)) {
    Serial.println(" connected.");
    mqttClient.subscribe("test/relay/uno1/lift1");
    mqttClient.subscribe("test/relay/uno1/lift2");
    mqttClient.subscribe("test/relay/uno2/lift3");
    mqttClient.subscribe("test/relay/uno2/lift4");
    mqttClient.subscribe("test/relay/uno3/lift5");
    mqttClient.subscribe("test/relay/uno3/lift6");
    mqttClient.subscribe("test/relay/uno1/status");
    mqttClient.subscribe("test/relay/uno2/status");
    mqttClient.subscribe("test/relay/uno3/status");
  } else {
    Serial.printf(" failed (rc=%d), retry in %lus\n",
                  mqttClient.state(), MQTT_RETRY_MS / 1000);
  }
}

// ─────────────────────────────────────────────────────────────
//  WEBSOCKET EVENT HANDLER
// ─────────────────────────────────────────────────────────────
void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
               AwsEventType type, void* arg, uint8_t* data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("[WS] Client #%u connected from %s\n",
                    client->id(), client->remoteIP().toString().c_str());
      // Send full state snapshot so the new client is immediately in sync
      client->text(buildFullStateJson());
      break;

    case WS_EVT_DISCONNECT:
      Serial.printf("[WS] Client #%u disconnected\n", client->id());
      break;

    case WS_EVT_DATA: {
      // ── Handle text commands from the browser ──
      AwsFrameInfo* info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len &&
          info->opcode == WS_TEXT) {
        data[len] = 0; // null-terminate
        String cmd = String((char*)data);

        // Expected format: {"cmd":"reset","zone":0,"slot":0}
        StaticJsonDocument<64> doc;
        if (deserializeJson(doc, cmd) == DeserializationError::Ok) {
          const char* c = doc["cmd"];
          if (c && strcmp(c, "reset") == 0) {
            int z = doc["zone"] | -1;
            int s = doc["slot"] | -1;
            if (z >= 0 && z < 3 && s >= 0 && s < 2) {
              bool wasOn = liftLatched[z][s];
              liftLatched[z][s] = false;
              recomputeAlarm();
              if (wasOn) {
                broadcastLiftEvent(z, s, "OFF");
                logEventToDatabase(z, s, "RESET");
              }
            }
          }
        }
      }
      break;
    }

    case WS_EVT_ERROR:
      Serial.printf("[WS] Error on client #%u\n", client->id());
      break;
    default: break;
  }
}

// ─────────────────────────────────────────────────────────────
//  PERIODIC UNO HEALTH BROADCAST  (detect timeout transitions)
// ─────────────────────────────────────────────────────────────
bool unoWasActive[3] = {false, false, false};
unsigned long lastHealthCheck = 0;
const unsigned long HEALTH_CHECK_MS = 3000UL;

void checkUnoHealth() {
  if (millis() - lastHealthCheck < HEALTH_CHECK_MS) return;
  lastHealthCheck = millis();

  for (int z = 0; z < 3; z++) {
    bool active = (lastSeenUNO[z] != 0) &&
                  ((millis() - lastSeenUNO[z]) <= UNO_TIMEOUT_MS);
    if (active != unoWasActive[z]) {
      unoWasActive[z] = active;
      broadcastUnoEvent(z, active);
    }
  }
}

// ─────────────────────────────────────────────────────────────
//  BUZZER  (non-blocking)
// ─────────────────────────────────────────────────────────────
void updateBuzzer() {
  if (!alarmLatched) {
    if (buzzerState) {
      digitalWrite(PIN_BUZZER, LOW);
      buzzerState = false;
    }
    return;
  }
  unsigned long now = millis();
  if (buzzerState && now - lastBuzzerChange >= BUZZER_ON_MS) {
    buzzerState = false;
    lastBuzzerChange = now;
    digitalWrite(PIN_BUZZER, LOW);
  } else if (!buzzerState && now - lastBuzzerChange >= BUZZER_OFF_MS) {
    buzzerState = true;
    lastBuzzerChange = now;
    digitalWrite(PIN_BUZZER, HIGH);
  }
}

// ─────────────────────────────────────────────────────────────
//  WiFi LED  (solid = connected, blink = connecting)
// ─────────────────────────────────────────────────────────────
void updateWifiLed() {
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(PIN_LED_WIFI, HIGH);
    return;
  }
  if (millis() - lastWifiBlink >= WIFI_BLINK_MS) {
    lastWifiBlink = millis();
    wifiLedState = !wifiLedState;
    digitalWrite(PIN_LED_WIFI, wifiLedState);
  }
}

// ─────────────────────────────────────────────────────────────
//  DASHBOARD HTML  (served as /  — stored in PROGMEM)
//  index.html.h must be in the same sketch folder.
// ─────────────────────────────────────────────────────────────
#include "index.html.h"   // defines: const char INDEX_HTML[] PROGMEM = R"==(...)";

// ─────────────────────────────────────────────────────────────
//  SETUP
// ─────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(PIN_BUZZER,    OUTPUT);
  pinMode(PIN_LED_POWER, OUTPUT);
  pinMode(PIN_LED_WIFI,  OUTPUT);
  digitalWrite(PIN_LED_POWER, HIGH);
  digitalWrite(PIN_BUZZER,    LOW);

  // ── WiFi ──
  Serial.printf("[WiFi] Connecting to %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
    updateWifiLed();
  }
  Serial.println();
  Serial.println("====================================");
  Serial.println("  ESP32 Lift Dashboard v2 — WebSocket");
  Serial.print  ("  Dashboard → http://");
  Serial.println(WiFi.localIP());
  Serial.println("====================================");

  // ── MQTT ──
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(512);   // enlarge if JSON payloads grow

  // ── WebSocket ──
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // ── HTTP routes ──
  // Dashboard HTML is served from PROGMEM (index.html.h).
  // To use LittleFS instead: call LittleFS.begin(true) and use
  //   server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send_P(200, "text/html", INDEX_HTML);
  });

  // 404 catch-all
  server.onNotFound([](AsyncWebServerRequest* req) {
    req->send(404, "text/plain", "Not found");
  });

  server.begin();
  Serial.println("[HTTP] Async server started on port 80");
}

// ─────────────────────────────────────────────────────────────
//  LOOP  — keep it lean; heavy work is callback-driven
// ─────────────────────────────────────────────────────────────
void loop() {
  updateWifiLed();
  updateBuzzer();
  checkUnoHealth();

  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) tryReconnectMQTT();
    mqttClient.loop();
  }

  // Periodically clean up disconnected WebSocket clients
  ws.cleanupClients();
}
