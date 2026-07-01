#include <SPI.h>
#include <Ethernet.h>

// ============================================================
//  UNO3 — OTS Side (Lift 5 & 6)
//  Direct HTTP POST → InfluxDB (no MQTT, no ESP32)
// ============================================================

// ---------- Ethernet ----------
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x03 };
IPAddress ip(192, 168, 1, 103);
EthernetClient ethClient;

// ---------- InfluxDB ----------
// >>> CHANGE THESE TO MATCH YOUR INFLUXDB SERVER <<<
const char INFLUX_HOST[] = "124.43.179.232";
const int  INFLUX_PORT   = 8086;
const char INFLUX_ORG[]  = "SLT";
const char INFLUX_BUCKET[] = "Lift_Emergency_Alarm";
const char INFLUX_TOKEN[]  = "jsEgn9UpR2yTjXaJpSNdwOEow1rnzqMDXPlBuriQaFiq9Mj9X0UIBgu0RN1_kmQMnrCvDojdgM3TYboVSo0D-Q==";

// ---------- Node Identity ----------
const char NODE_TAG[] = "node3";
const char LIFT_A[]   = "lift5";
const char LIFT_B[]   = "lift6";

// ---------- Buttons ----------
const int liftAPin = A0;    // NOTE: A0 for lift5 (was different from other nodes)
const int liftBPin = A4;

// ---------- LEDs ----------
const int LED_POWER = 6;
const int LED_ETH   = 7;

// ---------- Ethernet LED blink ----------
const unsigned long ETH_BLINK_MS = 500;
unsigned long lastEthBlink = 0;
bool ethLedState = false;

// ---------- Timings ----------
const unsigned long CHECK_DELAY   = 100;
const unsigned long HEARTBEAT_MS  = 3000;   // send current state every 3s

// ---------- State ----------
bool lastA = false;
bool lastB = false;
unsigned long lastHeartbeat = 0;

// ============================================================
//  Send one lift data point to InfluxDB
// ============================================================
bool sendLiftState(const char* lift, int state) {
  if (!ethClient.connect(INFLUX_HOST, INFLUX_PORT)) {
    Serial.println(F("InfluxDB connect failed"));
    return false;
  }

  int safeState = (state ? 1 : 0);  // clamp to strict 0 or 1
  char payload[64];
  sprintf(payload, "lift_status,node=%s,lift=%s state=%di",
          NODE_TAG, lift, safeState);

  int len = strlen(payload);

  ethClient.print(F("POST /api/v2/write?org="));
  ethClient.print(INFLUX_ORG);
  ethClient.print(F("&bucket="));
  ethClient.print(INFLUX_BUCKET);
  ethClient.println(F("&precision=s HTTP/1.1"));

  ethClient.print(F("Host: "));
  ethClient.println(INFLUX_HOST);

  ethClient.print(F("Authorization: Token "));
  ethClient.println(INFLUX_TOKEN);

  ethClient.println(F("Content-Type: text/plain"));

  ethClient.print(F("Content-Length: "));
  ethClient.println(len);

  ethClient.println(F("Connection: close"));
  ethClient.println();
  ethClient.print(payload);

  unsigned long t = millis();
  while (!ethClient.available() && millis() - t < 1000) {}

  bool ok = false;
  if (ethClient.available()) {
    char respBuf[32];
    int idx = 0;
    while (ethClient.available() && idx < 31) {
      char c = ethClient.read();
      if (c == '\n') break;
      respBuf[idx++] = c;
    }
    respBuf[idx] = '\0';

    if (strstr(respBuf, "204")) {
      ok = true;
    } else {
      Serial.print(F("InfluxDB error: "));
      Serial.println(respBuf);
    }
  }

  while (ethClient.available()) ethClient.read();
  ethClient.stop();

  return ok;
}

// ============================================================
//  Send both lifts (heartbeat)
// ============================================================
void sendBothLifts() {
  if (!ethClient.connect(INFLUX_HOST, INFLUX_PORT)) {
    Serial.println(F("InfluxDB heartbeat connect failed"));
    return;
  }

  int safeA = (lastA ? 1 : 0);  // clamp to strict 0 or 1
  int safeB = (lastB ? 1 : 0);
  char payload[128];
  sprintf(payload,
    "lift_status,node=%s,lift=%s state=%di\n"
    "lift_status,node=%s,lift=%s state=%di",
    NODE_TAG, LIFT_A, safeA,
    NODE_TAG, LIFT_B, safeB);

  int len = strlen(payload);

  ethClient.print(F("POST /api/v2/write?org="));
  ethClient.print(INFLUX_ORG);
  ethClient.print(F("&bucket="));
  ethClient.print(INFLUX_BUCKET);
  ethClient.println(F("&precision=s HTTP/1.1"));

  ethClient.print(F("Host: "));
  ethClient.println(INFLUX_HOST);

  ethClient.print(F("Authorization: Token "));
  ethClient.println(INFLUX_TOKEN);

  ethClient.println(F("Content-Type: text/plain"));

  ethClient.print(F("Content-Length: "));
  ethClient.println(len);

  ethClient.println(F("Connection: close"));
  ethClient.println();
  ethClient.print(payload);

  unsigned long t = millis();
  while (!ethClient.available() && millis() - t < 1000) {}
  while (ethClient.available()) ethClient.read();
  ethClient.stop();

  Serial.println(F("Heartbeat sent"));
}

// ---------- Ethernet LED ----------
void updateEthernetLed() {
  if (Ethernet.linkStatus() == LinkON) {
    digitalWrite(LED_ETH, HIGH);
    return;
  }
  if (millis() - lastEthBlink >= ETH_BLINK_MS) {
    lastEthBlink = millis();
    ethLedState = !ethLedState;
    digitalWrite(LED_ETH, ethLedState);
  }
}

// ============================================================
void setup() {
  Serial.begin(9600);

  pinMode(LED_POWER, OUTPUT);
  pinMode(LED_ETH, OUTPUT);
  digitalWrite(LED_POWER, HIGH);
  digitalWrite(LED_ETH, LOW);

  pinMode(liftAPin, INPUT_PULLUP);
  pinMode(liftBPin, INPUT_PULLUP);

  Serial.println(F("Starting Ethernet..."));
  if (Ethernet.begin(mac) == 0) {
    Serial.println(F("DHCP failed, using static IP"));
    Ethernet.begin(mac, ip);
  }
  delay(2000);

  Serial.print(F("UNO3 OTS IP: "));
  Serial.println(Ethernet.localIP());

  lastA = !digitalRead(liftAPin);
  lastB = !digitalRead(liftBPin);

  sendBothLifts();
}

// ============================================================
void loop() {
  updateEthernetLed();

  bool curA = !digitalRead(liftAPin);
  bool curB = !digitalRead(liftBPin);

  // ---------- Lift 5 (state change only) ----------
  if (curA != lastA) {
    lastA = curA;
    sendLiftState(LIFT_A, (int)curA);
    Serial.print(F("Lift5: "));
    Serial.println(curA ? F("ON") : F("OFF"));
  }

  // ---------- Lift 6 (state change only) ----------
  if (curB != lastB) {
    lastB = curB;
    sendLiftState(LIFT_B, (int)curB);
    Serial.print(F("Lift6: "));
    Serial.println(curB ? F("ON") : F("OFF"));
  }

  // ---------- Heartbeat ----------
  if (millis() - lastHeartbeat >= HEARTBEAT_MS) {
    lastHeartbeat = millis();
    sendBothLifts();
  }

  delay(CHECK_DELAY);
}
