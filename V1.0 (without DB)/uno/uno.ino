#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

// ---------- Ethernet (UNO3 - OTS Side) ----------
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x03 };   // unique MAC for UNO3
IPAddress ip(192, 168, 1, 103);                       // static IP for UNO3

// ---------- MQTT ----------
const char* mqtt_server = "broker.hivemq.com";
EthernetClient ethClient;
PubSubClient client(ethClient);

// ---------- Buttons (OTS Side) ----------
const int lift5Pin = A0;   // Lift 5 button
const int lift6Pin = A4;   // Lift 6 button

// ---------- LEDs ----------
const int LED_POWER = 6;   // Power indicator LED
const int LED_ETH   = 7;   // Ethernet status LED

// ---------- Ethernet LED blink ----------
const unsigned long ETH_BLINK_MS = 500;
unsigned long lastEthBlink = 0;
bool ethLedState = false;

// ---------- Timings ----------
const unsigned long CHECK_DELAY   = 300;    // ms
const unsigned long HEARTBEAT_MS  = 10000;  // 10 seconds
const unsigned long MQTT_RETRY_MS = 5000;   // 5 seconds

// ---------- State ----------
bool lastLift5 = false;
bool lastLift6 = false;
unsigned long lastHeartbeat = 0;
unsigned long lastMQTTTry = 0;

// ---------- Publish helper ----------
void publishLift(const char* topic, bool state) {
  client.publish(topic, state ? "ON" : "OFF", true); // retained = true
}

// ---------- Ethernet LED update (added only) ----------
void updateEthernetLed() {
  // Ethernet.linkStatus() requires newer Ethernet library (most Arduino IDE have it).
  // If link is ON -> solid ON
  // If link is OFF -> blink
  if (Ethernet.linkStatus() == LinkON) {
    digitalWrite(LED_ETH, HIGH);
    return;
  }

  // blink when link is OFF
  if (millis() - lastEthBlink >= ETH_BLINK_MS) {
    lastEthBlink = millis();
    ethLedState = !ethLedState;
    digitalWrite(LED_ETH, ethLedState);
  }
}

// ---------- MQTT reconnect (non-blocking) ----------
void reconnectMQTT() {
  if (millis() - lastMQTTTry < MQTT_RETRY_MS) return;
  lastMQTTTry = millis();

  Serial.print("Connecting MQTT...");
  if (client.connect("UNO3_OTS")) {
    Serial.println("connected");

    // publish current states once connected
    publishLift("test/relay/uno3/lift5", lastLift5);
    publishLift("test/relay/uno3/lift6", lastLift6);

    // publish heartbeat too
    client.publish("test/relay/uno3/status", "ALIVE", true);
  } else {
    Serial.print("failed, rc=");
    Serial.println(client.state());
  }
}

void setup() {
  Serial.begin(9600);

  // ---------- LEDs setup (added only) ----------
  pinMode(LED_POWER, OUTPUT);
  pinMode(LED_ETH, OUTPUT);

  digitalWrite(LED_POWER, HIGH); // power LED ON always
  digitalWrite(LED_ETH, LOW);

  // Buttons wired to GND, use internal pull-ups (pressed = ON)
  pinMode(lift5Pin, INPUT_PULLUP);
  pinMode(lift6Pin, INPUT_PULLUP);

  // Start Ethernet
  Serial.println("Starting Ethernet...");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("DHCP failed, using static IP");
    Ethernet.begin(mac, ip);
  }
  delay(2000);

  Serial.print("UNO3 IP: ");
  Serial.println(Ethernet.localIP());

  client.setServer(mqtt_server, 1883);

  // Read initial states
  lastLift5 = !digitalRead(lift5Pin); // pressed=LOW => ON
  lastLift6 = !digitalRead(lift6Pin);

  // Try initial MQTT connect and publish once
  if (client.connect("UNO3_OTS")) {
    publishLift("test/relay/uno3/lift5", lastLift5);
    publishLift("test/relay/uno3/lift6", lastLift6);
    client.publish("test/relay/uno3/status", "ALIVE", true);
  }
}

void loop() {
  // ---------- Ethernet LED update (added only) ----------
  updateEthernetLed();

  if (!client.connected()) reconnectMQTT();
  client.loop();

  // Read buttons (pressed = ON)
  bool cur5 = !digitalRead(lift5Pin);
  bool cur6 = !digitalRead(lift6Pin);

  // Lift 5
  if (cur5 != lastLift5) {
    lastLift5 = cur5;
    if (client.connected()) publishLift("test/relay/uno3/lift5", cur5);
    Serial.print("UNO3 Lift5: "); Serial.println(cur5 ? "ON" : "OFF");
  }

  // Lift 6
  if (cur6 != lastLift6) {
    lastLift6 = cur6;
    if (client.connected()) publishLift("test/relay/uno3/lift6", cur6);
    Serial.print("UNO3 Lift6: "); Serial.println(cur6 ? "ON" : "OFF");
  }

  // Heartbeat (so ESP32 shows Active even if no button pressed)
  if (client.connected() && millis() - lastHeartbeat >= HEARTBEAT_MS) {
    lastHeartbeat = millis();
    client.publish("test/relay/uno3/status", "ALIVE", true);
  }

  delay(CHECK_DELAY);
}
