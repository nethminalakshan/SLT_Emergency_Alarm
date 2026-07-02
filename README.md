# 🚨 SLT Lift Emergency Alarm System

> **SLT Mobitel** — Building Lift Emergency Alarm Monitoring System  
> **Current Version:** V1.3 (without ESP32) + Master Alarm (Raspberry Pi)  
> **Last Updated:** July 2026

---

## 📖 Table of Contents

1. [Project Overview](#-project-overview)
2. [System Architecture](#-system-architecture)
3. [Hardware Components](#-hardware-components)
4. [Version History](#-version-history)
5. [Current Version (V1.3) — Step-by-Step Setup Guide](#-current-version-v13--step-by-step-setup-guide)
6. [Database (InfluxDB) Details](#-database-influxdb-details)
7. [Network Configuration](#-network-configuration)
8. [MQTT Topics Reference (V1.0 – V1.2)](#-mqtt-topics-reference-v10--v12)
9. [Master Alarm System (Raspberry Pi)](#-master-alarm-system-raspberry-pi)
10. [Folder Structure](#-folder-structure)
11. [Troubleshooting](#-troubleshooting)

---

## 🏗 Project Overview

This system monitors **6 emergency alarm switches** across **3 lift zones** in the SLT Mobitel building. When a lift emergency button is pressed, the alarm state is detected by Arduino UNO boards and transmitted to a central database for real-time monitoring and historical logging.

### Zones & Lifts

| Zone | Location | Lifts | Arduino Node |
|------|----------|-------|-------------|
| Zone 1 | **Lotus Side** | Lift 1, Lift 2 | UNO1 (node1) |
| Zone 2 | **Duke Side** | Lift 3, Lift 4 | UNO2 (node2) |
| Zone 3 | **OTS** | Lift 5, Lift 6 | UNO3 (node3) |

### How It Works

1. A lift emergency button is pressed → the connected Arduino UNO detects the state change.
2. The UNO sends the alarm state (`ON` / `OFF` as `1` / `0`) to the **InfluxDB** database via HTTP POST.
3. The database stores each event with a timestamp for real-time monitoring and historical analysis.

---

## 🏛 System Architecture

### V1.3 Architecture (Current — Simplified, No ESP32)

```
┌────────────────────────────────────────────────────────────────────┐
│                         SLT Building                              │
│                                                                    │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐            │
│  │ UNO1 (Lotus) │  │ UNO2 (Duke)  │  │ UNO3 (OTS)   │            │
│  │  Lift 1 & 2  │  │  Lift 3 & 4  │  │  Lift 5 & 6  │            │
│  │  192.168.1.101│  │  192.168.1.102│  │  192.168.1.103│            │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘            │
│         │                 │                 │                      │
│         └─────────────────┼─────────────────┘                      │
│                           │  Ethernet (LAN)                        │
│                           │                                        │
│                  ┌────────▼────────┐                               │
│                  │   LAN Switch     │                               │
│                  └────────┬────────┘                               │
│                           │                                        │
└───────────────────────────┼────────────────────────────────────────┘
                            │
                   ┌────────▼────────┐
                   │   InfluxDB v2    │
                   │ 124.43.179.232   │
                   │    Port: 8086    │
                   └─────────────────┘
```

Each Arduino UNO sends data **directly** to InfluxDB via HTTP POST — **no MQTT broker, no ESP32** intermediary.

---

## 🔧 Hardware Components

### Per UNO Node (×3 total)

| Component | Qty | Description |
|-----------|-----|-------------|
| Arduino UNO | 1 | Main controller |
| W5100/W5500 Ethernet Shield | 1 | For LAN connectivity |
| Emergency alarm switch | 2 | Connected to analog input pins |
| Power LED | 1 | Pin 6 — always ON |
| Ethernet status LED | 1 | Pin 7 — solid = link OK, blink = link down |

### Pin Assignments

| Node | Lift A Pin | Lift B Pin | Power LED | Ethernet LED |
|------|-----------|-----------|-----------|-------------|
| UNO1 (Lotus) | A5 (Lift 1) | A4 (Lift 2) | D6 | D7 |
| UNO2 (Duke) | A5 (Lift 3) | A4 (Lift 4) | D6 | D7 |
| UNO3 (OTS) | A0 (Lift 5) | A4 (Lift 6) | D6 | D7 |

> ⚠️ **Note:** UNO3 (OTS) uses pin **A0** for Lift 5 instead of A5 used by the other nodes.

---

## 📜 Version History

### V1.0 — Initial Prototype (without DB)

📂 Location: `Previous Versions/V1.0 (without DB)/`

**Architecture:** UNO → MQTT → ESP32 → Web Dashboard

| What It Had | Details |
|-------------|---------|
| **UNO nodes** | 3× Arduino UNO with Ethernet shields |
| **ESP32** | Central hub — WiFi, web server, MQTT subscriber, buzzer |
| **Communication** | MQTT via public broker (`broker.hivemq.com`) |
| **Dashboard** | ESP32-hosted HTML page with polling (`/state` JSON endpoint, refreshed every 2 seconds) |
| **Alarm** | Buzzer on ESP32 (intermittent: 1s ON, 2s OFF) |
| **Database** | ❌ None — no data persistence |
| **UNO health** | Heartbeat every 10s; ESP32 showed "Active"/"Deactive" per UNO |

**Key Files:**
- `uno/uno.ino` — UNO code (single file for all 3 nodes with different config)
- `uno/esp/esp.ino` — ESP32 firmware with built-in web dashboard

**Limitations:**
- No historical data — once you refreshed or power-cycled, all state was lost
- Dashboard used full-page polling (not real-time)
- Depended on public MQTT broker (internet dependency)

---

### V1.1 — Added Firebase Database

📂 Location: `Previous Versions/V1.1/`

**Architecture:** UNO → MQTT → ESP32 → Firebase Realtime Database

| What Changed | Details |
|-------------|---------|
| **Database** | ✅ Firebase Realtime Database added |
| **ESP32** | Now pushes MQTT events to Firebase via HTTP PUT |
| **Dashboard** | Simple status page (MQTT message, LED state, Firebase status) |
| **Firebase URL** | `https://data-base-310ff-default-rtdb.asia-southeast1.firebasedatabase.app` |
| **Data format** | `{"state": "ON/OFF", "timestamp": <millis>}` stored under `/relay_log/<millis>.json` |

**Key Files:**
- `uno code.txt` — UNO code (single relay on pin A5, voltage threshold detection at 4.5V)
- `ESP code.txt` — ESP32 firmware with Firebase integration

**Key Differences from V1.0:**
- UNO used voltage-based detection (4.5V threshold on A5) instead of button-style input
- Static IP: ESP32 at `192.168.1.13`
- Single relay topic: `test/relay` (only 1 relay monitored)
- Firebase data was stored but no structured querying or dashboards

---

### V1.2 — InfluxDB + WebSocket Dashboard

📂 Location: `Previous Versions/V!.2 (With DB)/`

**Architecture:** UNO → MQTT → ESP32 → InfluxDB + WebSocket Dashboard

| What Changed | Details |
|-------------|---------|
| **Database** | ✅ Switched to **InfluxDB v2** (time-series, much better for IoT) |
| **Dashboard** | ✅ Premium dark-mode WebSocket dashboard with live event log |
| **Communication** | MQTT (UNO→ESP32) + WebSocket (ESP32→Browser) |
| **ESP32 Libraries** | ESPAsyncWebServer, AsyncTCP, ArduinoJson, PubSubClient |
| **NTP Time** | ✅ Accurate timestamps via NTP (`pool.ntp.org`, UTC+5:30) |
| **UNO nodes** | 3 separate UNO code files (one per zone) |

**Key Files:**
- `ESP/esp_websocket/esp_websocket.ino` — ESP32 firmware (WebSocket + InfluxDB)
- `ESP/esp_websocket/index.html.h` — Dashboard HTML stored in PROGMEM
- `ESP/InfluxDB_Setup_Guide.md` — Full InfluxDB setup instructions
- `Uno/lift1&2 uno 1 Lotus.txt` — UNO1 firmware (MQTT)
- `Uno/lift3&4 uno2 Duke.txt` — UNO2 firmware (MQTT)
- `Uno/lift5&6 uno3 OTS.txt` — UNO3 firmware (MQTT)

**V1.2 InfluxDB Config (old — replaced in V1.3):**

| Parameter | Value |
|-----------|-------|
| Host | `124.43.179.232` |
| Port | `8086` |
| Org | `SLT` |
| Bucket | `lift_alarms` |
| Measurement | `zone_status` |

**Dashboard Features:**
- Real-time WebSocket updates (no polling)
- Glassmorphism dark theme with Inter font
- Live event log with timestamped entries
- UNO node health badges (Active/Inactive)
- Per-lift RESET buttons via WebSocket commands
- SLTMobitel branding

---

### V1.3 — Direct UNO-to-InfluxDB (Current Version ✅)

📂 Location: `V1.3 (without ESP32)/`

**Architecture:** UNO → InfluxDB (Direct HTTP POST — no ESP32, no MQTT)

| What Changed | Details |
|-------------|---------|
| **ESP32** | ❌ **Removed entirely** — no more ESP32 or MQTT |
| **Communication** | Arduino UNO posts directly to InfluxDB via Ethernet |
| **Database** | InfluxDB v2 (same server, new bucket name) |
| **Heartbeat** | Every 3 seconds (was 10s in earlier versions) |
| **Libraries** | Only `SPI.h` + `Ethernet.h` — no MQTT/PubSubClient needed |
| **Buzzer/Dashboard** | ❌ Removed (no ESP32 to host them) |

**Why the change?**
- Removed the ESP32 as a single point of failure
- Eliminated internet dependency (no public MQTT broker)
- Simpler architecture — fewer moving parts
- Each UNO is independently responsible for logging

---

## 🚀 Current Version (V1.3) — Step-by-Step Setup Guide

### Prerequisites

| Item | Details |
|------|---------|
| Arduino IDE | Version 1.8+ or 2.x |
| Arduino UNO boards | 3 units |
| Ethernet shields | W5100 or W5500 |
| LAN network | All devices on same subnet |
| InfluxDB v2 server | Already running at `124.43.179.232:8086` |

### Step 1: Install Arduino IDE

1. Download from [arduino.cc](https://www.arduino.cc/en/software)
2. Install and launch the IDE

### Step 2: Install Required Libraries

Open Arduino IDE → **Sketch** → **Include Library** → **Manage Libraries...**

Install these libraries:
- ✅ `Ethernet` (built-in — should already be available)
- ✅ `SPI` (built-in — should already be available)

> 💡 V1.3 does **not** need PubSubClient, WiFi, or any MQTT library.

### Step 3: Open the Correct Sketch

Each UNO node has its **own** sketch. Open the correct one for the node you are programming:

| Node | Sketch File |
|------|-------------|
| UNO1 (Lotus Side, Lifts 1-2) | `V1.3 (without ESP32)/Uno/Lotus_lift_1_2/Lotus_lift_1_2.ino` |
| UNO2 (Duke Side, Lifts 3-4) | `V1.3 (without ESP32)/Uno/Duke_lift_3_4/Duke_lift_3_4.ino` |
| UNO3 (OTS, Lifts 5-6) | `V1.3 (without ESP32)/Uno/OTS_lift_5_6/OTS_lift_5_6.ino` |

### Step 4: Review Configuration (Already Set)

The InfluxDB connection details are already configured in each sketch:

```cpp
const char INFLUX_HOST[]   = "124.43.179.232";
const int  INFLUX_PORT     = 8086;
const char INFLUX_ORG[]    = "SLT";
const char INFLUX_BUCKET[] = "Lift_Emergency_Alarm";
const char INFLUX_TOKEN[]  = "jsEgn9UpR2yTjXaJpSNdwOEow1rnzqMDXPlBuriQaFiq9Mj9X0UIBgu0RN1_...";
```

> ⚠️ **Only change these if the InfluxDB server moves or a new token is generated.**

### Step 5: Connect Hardware

1. Stack the **Ethernet Shield** on top of the Arduino UNO
2. Connect **emergency alarm switches**:
   - **Lift A switch** → to the correct analog pin (see pin table above) and GND
   - **Lift B switch** → to pin A4 and GND
3. Connect **LEDs** (optional):
   - Power LED → pin D6 (with resistor to GND)
   - Ethernet LED → pin D7 (with resistor to GND)
4. Connect Ethernet cable from the shield to your **LAN switch**
5. Power the UNO via USB or external 9V supply

### Step 6: Upload the Sketch

1. Connect the UNO to your PC via USB
2. In Arduino IDE:
   - **Tools** → **Board** → **Arduino UNO**
   - **Tools** → **Port** → Select the correct COM port
3. Click **Upload** (→ button)
4. Open **Serial Monitor** (🔍 button) at **9600 baud**
5. You should see:
   ```
   Starting Ethernet...
   UNO1 Lotus IP: 192.168.1.101
   Heartbeat sent
   ```

### Step 7: Verify Data in InfluxDB

1. Open InfluxDB UI in browser: `http://124.43.179.232:8086`
2. Go to **Data Explorer** → **Script Editor**
3. Run this query:
   ```flux
   from(bucket: "Lift_Emergency_Alarm")
     |> range(start: -1h)
     |> filter(fn: (r) => r._measurement == "lift_status")
   ```
4. You should see data points with `state` values of `0` or `1`

### Step 8: Repeat for All 3 Nodes

Repeat Steps 3–7 for each of the 3 Arduino UNO nodes (Lotus, Duke, OTS).

---

## 💾 Database (InfluxDB) Details

### Connection Information (V1.3 — Current)

| Parameter | Value |
|-----------|-------|
| **Database Type** | InfluxDB v2 |
| **Server IP** | `124.43.179.232` |
| **Port** | `8086` |
| **Organisation** | `SLT` |
| **Bucket** | `Lift_Emergency_Alarm` |
| **API Token** | `jsEgn9UpR2yTjXaJpSNdwOEow1rnzqMDXPlBuriQaFiq9Mj9X0UIBgu0RN1_...` |
| **Web UI URL** | `http://124.43.179.232:8086` |
| **API Write Endpoint** | `POST /api/v2/write?org=SLT&bucket=Lift_Emergency_Alarm&precision=s` |

### Data Schema (V1.3)

```
Bucket: Lift_Emergency_Alarm
  └── Measurement: lift_status
        ├── Tags (indexed, for fast filtering):
        │     ├── node = "node1" | "node2" | "node3"
        │     └── lift = "lift1" | "lift2" | "lift3" | "lift4" | "lift5" | "lift6"
        │
        └── Fields (stored values):
              └── state = 0i (OFF/OK) | 1i (ON/ALARM)
```

### Data Point Examples

```
lift_status,node=node1,lift=lift1 state=1i    ← Lift 1 alarm ON
lift_status,node=node1,lift=lift1 state=0i    ← Lift 1 alarm OFF
lift_status,node=node2,lift=lift3 state=1i    ← Lift 3 alarm ON
lift_status,node=node3,lift=lift6 state=0i    ← Lift 6 alarm OFF
```

### Node-to-Lift Mapping in Database

| Tag `node` | Tag `lift` | Physical Lift | Location |
|------------|-----------|---------------|----------|
| `node1` | `lift1` | Lift 1 | Lotus Side |
| `node1` | `lift2` | Lift 2 | Lotus Side |
| `node2` | `lift3` | Lift 3 | Duke Side |
| `node2` | `lift4` | Lift 4 | Duke Side |
| `node3` | `lift5` | Lift 5 | OTS |
| `node3` | `lift6` | Lift 6 | OTS |

### Useful Flux Queries

**Get the latest status of ALL lifts:**
```flux
from(bucket: "Lift_Emergency_Alarm")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "lift_status")
  |> last()
```

**Get all alarm events (state=1) in the last 24 hours:**
```flux
from(bucket: "Lift_Emergency_Alarm")
  |> range(start: -24h)
  |> filter(fn: (r) => r._measurement == "lift_status")
  |> filter(fn: (r) => r._value == 1)
```

**Get status of a specific lift (e.g., Lift 3):**
```flux
from(bucket: "Lift_Emergency_Alarm")
  |> range(start: -1d)
  |> filter(fn: (r) => r._measurement == "lift_status")
  |> filter(fn: (r) => r.lift == "lift3")
  |> last()
```

**Get all data from Lotus Side (node1):**
```flux
from(bucket: "Lift_Emergency_Alarm")
  |> range(start: -1d)
  |> filter(fn: (r) => r._measurement == "lift_status")
  |> filter(fn: (r) => r.node == "node1")
```

### Previous Database Configurations

| Version | DB Type | Bucket/Path | Measurement | Org |
|---------|---------|------------|-------------|-----|
| V1.0 | ❌ None | — | — | — |
| V1.1 | Firebase RTDB | `/relay_log/<millis>` | — | — |
| V1.2 | InfluxDB v2 | `lift_alarms` | `zone_status` | `SLT` |
| **V1.3** | **InfluxDB v2** | **`Lift_Emergency_Alarm`** | **`lift_status`** | **`SLT`** |

---

## 🌐 Network Configuration

### Static IP Assignments

| Device | IP Address | MAC Address |
|--------|-----------|-------------|
| UNO1 (Lotus) | `192.168.1.101` | `DE:AD:BE:EF:FE:01` |
| UNO2 (Duke) | `192.168.1.102` | `DE:AD:BE:EF:FE:02` |
| UNO3 (OTS) | `192.168.1.103` | `DE:AD:BE:EF:FE:03` |
| InfluxDB Server | `124.43.179.232` | — |

> All UNOs first attempt DHCP. If DHCP fails, they fall back to the static IPs above.

---

## 📡 MQTT Topics Reference (V1.0 – V1.2)

> ℹ️ MQTT is **NOT used in V1.3**. This is for reference only if you need to understand older versions.

| Topic | Publisher | Payload | Description |
|-------|-----------|---------|-------------|
| `test/relay/uno1/lift1` | UNO1 | `ON` / `OFF` | Lift 1 alarm state |
| `test/relay/uno1/lift2` | UNO1 | `ON` / `OFF` | Lift 2 alarm state |
| `test/relay/uno2/lift3` | UNO2 | `ON` / `OFF` | Lift 3 alarm state |
| `test/relay/uno2/lift4` | UNO2 | `ON` / `OFF` | Lift 4 alarm state |
| `test/relay/uno3/lift5` | UNO3 | `ON` / `OFF` | Lift 5 alarm state |
| `test/relay/uno3/lift6` | UNO3 | `ON` / `OFF` | Lift 6 alarm state |
| `test/relay/uno{1-3}/status` | UNO{1-3} | `ALIVE` | Heartbeat (every 10s) |

**MQTT Broker:** `broker.hivemq.com` (public, port 1883)

---

## 🔔 Master Alarm System (Raspberry Pi)

> 📂 Location: `Master_Alarm/` — [Full documentation](Master_Alarm/README.md)

The Master Alarm system is a Raspberry Pi-based subsystem that reads a **single master alarm boolean** from InfluxDB and drives **two relay-controlled buzzers** located in different parts of the building.

### How It Works

1. The software team analyzes the 6 individual lift alarm states in InfluxDB
2. They write a single `master_alarm` boolean (`true`/`false`) to the `Lift_Alarm_Status` bucket
3. The Raspberry Pi polls this boolean every 1 second
4. If `alarm == true` → both buzzers activate via GPIO relays
5. If `alarm == false` → both buzzers deactivate

### Hardware

| Component | Details |
|-----------|---------|
| Controller | Raspberry Pi 4 or 5 |
| GPIO17 (Pin 11) | Relay 1 → Buzzer 1 |
| GPIO27 (Pin 13) | Relay 2 → Buzzer 2 |

### Quick Commands

```bash
# Install
cd Master_Alarm && sudo ./scripts/install.sh

# Start service
sudo systemctl start slt-master-alarm

# View logs
journalctl -u slt-master-alarm -f
```

### Key Features

- 1-second polling with state-change detection
- Automatic reconnection with exponential backoff
- systemd auto-start on boot (`Restart=always`)
- Dual logging (journal + rotating file)
- Comprehensive error handling — never crashes

For complete setup instructions, see [Master_Alarm/README.md](Master_Alarm/README.md).

---

## 📁 Folder Structure

```
SLT_Emergency_Alarm/
│
├── README.md                              ← You are here
│
├── Master_Alarm/                          ← 🔔 MASTER ALARM (Raspberry Pi)
│   ├── README.md                          ← Setup & deployment guide
│   ├── config/
│   │   └── config.yaml                   ← All configurable parameters
│   ├── src/
│   │   ├── main.py                       ← Application entry point
│   │   ├── config_loader.py              ← YAML config + validation
│   │   ├── influxdb_client_wrapper.py    ← InfluxDB connection & query
│   │   ├── gpio_controller.py            ← GPIO relay control
│   │   ├── alarm_monitor.py              ← Main polling loop
│   │   └── logger_setup.py               ← Centralized logging
│   ├── service/
│   │   └── slt-master-alarm.service      ← systemd auto-start
│   ├── scripts/
│   │   ├── install.sh                    ← Automated installer
│   │   └── test_gpio.py                  ← GPIO hardware test
│   ├── docs/
│   │   ├── architecture.md               ← System architecture
│   │   ├── wiring_diagram.md             ← Hardware wiring guide
│   │   ├── testing_guide.md              ← Test procedures
│   │   ├── deployment_guide.md           ← Production deployment
│   │   └── future_improvements.md        ← Roadmap
│   ├── requirements.txt                  ← Python dependencies
│   └── .env.example                      ← Environment variable template
│
├── V1.3 (without ESP32)/                  ← 🟢 CURRENT VERSION (Arduino)
│   └── Uno/
│       ├── db_details                     ← InfluxDB connection details
│       ├── Lotus_lift_1_2/
│       │   └── Lotus_lift_1_2.ino         ← UNO1 firmware
│       ├── Duke_lift_3_4/
│       │   └── Duke_lift_3_4.ino          ← UNO2 firmware
│       └── OTS_lift_5_6/
│           └── OTS_lift_5_6.ino           ← UNO3 firmware
│
└── Previous Versions/
    ├── V1.0 (without DB)/                 ← First prototype, no database
    │   └── uno/
    │       ├── uno.ino                    ← UNO firmware (MQTT)
    │       └── esp/
    │           └── esp.ino                ← ESP32 firmware (MQTT + Web Dashboard)
    │
    ├── V1.1/                              ← Firebase database attempt
    │   ├── uno code.txt                   ← UNO firmware (single relay)
    │   └── ESP code.txt                   ← ESP32 firmware (MQTT + Firebase)
    │
    └── V!.2 (With DB)/                    ← InfluxDB + WebSocket dashboard
        ├── ESP/
        │   ├── InfluxDB_Setup_Guide.md    ← InfluxDB setup instructions
        │   └── esp_websocket/
        │       ├── esp_websocket.ino      ← ESP32 firmware (WebSocket + InfluxDB)
        │       └── index.html.h           ← Dashboard HTML (PROGMEM)
        └── Uno/
            ├── lift1&2 uno 1 Lotus.txt    ← UNO1 firmware (MQTT)
            ├── lift3&4 uno2 Duke.txt      ← UNO2 firmware (MQTT)
            └── lift5&6 uno3 OTS.txt       ← UNO3 firmware (MQTT)
```

---

## 🔍 Troubleshooting

### UNO not connecting to InfluxDB

| Check | How |
|-------|-----|
| Ethernet cable connected? | Check LED on Ethernet shield — should be lit |
| IP assigned? | Serial monitor shows IP address on boot |
| InfluxDB reachable? | From a PC on same network: `ping 124.43.179.232` |
| Port open? | Browser: `http://124.43.179.232:8086` should show InfluxDB UI |
| Token valid? | Check token matches in `db_details` file and sketch |

### Serial Monitor shows "InfluxDB connect failed"

- The UNO cannot reach the InfluxDB server
- Check the Ethernet cable, LAN switch, and that the server is running
- Verify the IP address `124.43.179.232` is correct

### Serial Monitor shows "InfluxDB error: ..." (not 204)

- HTTP 401: Token is invalid or expired → generate a new API token
- HTTP 404: Bucket name is wrong → check `Lift_Emergency_Alarm` spelling
- HTTP 400: Malformed line protocol → check the payload format

### No data in InfluxDB Data Explorer

- Make sure you're querying the correct bucket: `Lift_Emergency_Alarm`
- Make sure the time range is recent enough (start: `-1h` or `-24h`)
- Press a lift button and check Serial Monitor for "Heartbeat sent"

---

## 👨‍💻 For New Developers — Quick Checklist

- [ ] Read this README fully
- [ ] Access the InfluxDB Web UI at `http://124.43.179.232:8086`
- [ ] Install Arduino IDE
- [ ] Open a V1.3 sketch and understand the code structure
- [ ] Connect a UNO with Ethernet shield
- [ ] Upload the sketch and verify via Serial Monitor
- [ ] Confirm data appears in InfluxDB Data Explorer
- [ ] Understand the `db_details` file for credentials

---

## 📞 Contact

For questions about this project, contact the SLT Mobitel IoT / Building Management team.

---

*This README was created to help new team members understand the full project history, architecture, and setup process.*
