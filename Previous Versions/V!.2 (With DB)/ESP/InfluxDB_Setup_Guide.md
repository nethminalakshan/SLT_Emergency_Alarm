# InfluxDB v2 Setup Guide
## SLT Mobitel — Lift Emergency Alarm System

This guide walks you through installing InfluxDB v2, creating the required database resources, and configuring the ESP32 firmware to write alarm events.

---

## 1. Install InfluxDB v2

Choose the method that matches your server OS. InfluxDB runs on **the PC / server** on the same network as the ESP32.

---

### Option A — Windows (Recommended for a local PC server)

1. Download the **InfluxDB v2** Windows zip from the official releases page:
   ```
   https://portal.influxdata.com/downloads/
   ```
   Select → **InfluxDB** → **v2.x** → **Windows Binaries (amd64)**

2. Extract the zip to a permanent folder, e.g.:
   ```
   C:\influxdb\
   ```

3. Open **PowerShell as Administrator** and navigate to the folder:
   ```powershell
   cd C:\influxdb
   .\influxd.exe
   ```
   Leave this window open — InfluxDB runs on port **8086** by default.

4. *(Optional)* Run InfluxDB as a Windows Service so it starts automatically on boot:
   ```powershell
   # Download NSSM from https://nssm.cc/ first
   nssm install InfluxDB "C:\influxdb\influxd.exe"
   nssm start InfluxDB
   ```

---

### Option B — Linux (Ubuntu / Debian)

```bash
# Import InfluxData GPG key
wget -q https://repos.influxdata.com/influxdata-archive_compat.key
echo '393e8779c89ac8d958f81f942f9ad7fb82a25e133faddaf92e15b16e6ac9ce4c influxdata-archive_compat.key' \
  | sha256sum -c && \
  cat influxdata-archive_compat.key | gpg --dearmor \
  | sudo tee /etc/apt/trusted.gpg.d/influxdata-archive_compat.gpg > /dev/null

echo 'deb [signed-by=/etc/apt/trusted.gpg.d/influxdata-archive_compat.gpg] https://repos.influxdata.com/debian stable main' \
  | sudo tee /etc/apt/sources.list.d/influxdata.list

sudo apt update && sudo apt install influxdb2
sudo systemctl start influxdb
sudo systemctl enable influxdb
```

---

### Option C — Docker (any OS)

```bash
docker run -d \
  --name influxdb \
  -p 8086:8086 \
  -v influxdb-data:/var/lib/influxdb2 \
  influxdb:2
```

---

## 2. Initial Setup via the Web UI

1. Open your browser:
   ```
   http://localhost:8086
   ```
   *(Replace `localhost` with your server's LAN IP if accessing from another machine.)*

2. Click **"Get Started"**.

3. Fill in the setup form:

   | Field | Value to enter |
   |---|---|
   | **Username** | `admin` *(or your choice)* |
   | **Password** | A strong password |
   | **Initial Organisation Name** | `SLTMobitel` |
   | **Initial Bucket Name** | `lift_alarms` |

4. Click **"Continue"** → **"Quick Start"**.

5. **Copy the Operator API Token shown on the next screen and save it now.**
   This is the token you will paste into the ESP32 firmware.

> **IMPORTANT:** The token is only shown once during setup. If you miss it, follow step 3 to generate a new one.

---

## 3. Create (or Retrieve) an API Token

1. In the InfluxDB UI, click **"Load Data"** (left sidebar) → **"API Tokens"**.
2. Click **"+ Generate API Token"**:
   - **All Access Token** — easy for development / local use
   - **Read/Write Token** — recommended for production (more secure)
3. Give it a description, e.g. `ESP32 Lift Alarm Writer`.
4. Click **"Save"** and **copy the token**.

---

## 4. Find Your Server IP Address

The ESP32 needs the **local IP** of the InfluxDB machine.

**Windows:**
```powershell
ipconfig
# Look for:  IPv4 Address . . . : 192.168.x.x
```

**Linux:**
```bash
ip addr show | grep "inet "
```

> **TIP:** Set a **static IP** on the InfluxDB server so the ESP32 never loses its target.

---

## 5. Configure the ESP32 Firmware

Open `esp_websocket.ino` and fill in the InfluxDB constants near the top of the file:

```cpp
// ── InfluxDB v2 configuration ──────────────────────────────────
const char* INFLUX_HOST   = "192.168.1.100";          // <- Your server LAN IP
const int   INFLUX_PORT   = 8086;                     // <- Default port (do not change)
const char* INFLUX_ORG    = "SLTMobitel";             // <- Org name from step 2
const char* INFLUX_BUCKET = "lift_alarms";            // <- Bucket name from step 2
const char* INFLUX_TOKEN  = "paste_your_token_here==";  // <- Token from step 3

// NTP configuration
const char* NTP_SERVER   = "pool.ntp.org";
const long  GMT_OFFSET_S = 19800L;  // UTC+5:30 for Sri Lanka (5.5 x 3600)
const int   DAYLIGHT_S   = 0;
```

Flash the updated sketch to the ESP32 (**Arduino IDE → Upload**).

---

## 6. Verify Data is Being Written

### Method A — Serial Monitor

Open the Arduino IDE Serial Monitor at **115200 baud**.
When a lift alarm triggers via MQTT or is reset via the dashboard, you should see:

```
[MQTT] topic=test/relay/uno3/lift6  msg=ON
[DB] Line Protocol: zone_status,zone=OTS,uno=UNO3 lift1="ALARM",lift2="OK" 1719384300000000000
[DB] Write OK (HTTP 204) → bucket=lift_alarms
```

**HTTP 204 = success.** InfluxDB returns 204 No Content when a write is accepted.

---

### Method B — InfluxDB Data Explorer (UI)

1. In the InfluxDB UI click **"Data Explorer"** (left sidebar).
2. Click **"Script Editor"** tab and paste:

```flux
from(bucket: "lift_alarms")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "zone_status")
```

3. Click **"▶ Run"** — a table/graph with alarm states per lift and zone will appear.

---

## 7. Data Schema Reference

Your database is structured to map the alarm states hierarchically under the `lift_alarms` bucket:

```
Bucket: lift_alarms
  └── Measurement: zone_status
        ├── Tag: zone = "Lotus Side", uno = "UNO1"
        │     ├── Field: lift1 = "OK" | "ALARM"  (Lift 1)
        │     └── Field: lift2 = "OK" | "ALARM"  (Lift 2)
        ├── Tag: zone = "Duke Side",  uno = "UNO2"
        │     ├── Field: lift1 = "OK" | "ALARM"  (Lift 3)
        │     └── Field: lift2 = "OK" | "ALARM"  (Lift 4)
        └── Tag: zone = "OTS",        uno = "UNO3"
              ├── Field: lift1 = "OK" | "ALARM"  (Lift 5)
              └── Field: lift2 = "OK" | "ALARM"  (Lift 6)
```

| Type | Key | Example Value | Description |
|---|---|---|---|
| **Tag** | `zone` | `Lotus\ Side` | Zone name (spaces are backslash-escaped) |
| **Tag** | `uno` | `UNO1` | Associated Arduino node name |
| **Field** | `lift1` | `"OK"` | Status of the first lift in the zone (e.g. Lift 1, 3, or 5) |
| **Field** | `lift2` | `"ALARM"` | Status of the second lift in the zone (e.g. Lift 2, 4, or 6) |
| **Timestamp** | — | nanoseconds | Unix nanoseconds, NTP-synced |

---

## 8. Useful Flux Queries

**All lift status fields grouped by zone:**
```flux
from(bucket: "lift_alarms")
  |> range(start: -24h)
  |> filter(fn: (r) => r._measurement == "zone_status")
  |> last()
```

**Get the current status of all OTS lifts:**
```flux
from(bucket: "lift_alarms")
  |> range(start: -1d)
  |> filter(fn: (r) => r._measurement == "zone_status" and r.zone == "OTS")
  |> last()
```

---

## Architecture Summary

```
ESP32 (MQTT subscribe)
  |
  +-- receives change (e.g. Lift 6 is ON)
  |
  v
Builds Line Protocol string:
  zone_status,zone=OTS,uno=UNO3 lift1="ALARM",lift2="OK" <timestamp>
  |
  +-- HTTP POST /api/v2/write?org=SLTMobitel&bucket=lift_alarms&precision=ns
  |
  v
InfluxDB (port 8086)
```
