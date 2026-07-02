# 🚨 SLT Lift Master Alarm System — Raspberry Pi

> **SLT Mobitel** — Building-Wide Emergency Alarm Buzzer Controller  
> Reads the master alarm boolean from InfluxDB and drives two relay-controlled buzzers.

---

## Quick Start

### 1. Hardware Setup

Connect the Raspberry Pi GPIO to two relay modules and buzzers. See [docs/wiring_diagram.md](docs/wiring_diagram.md) for detailed instructions.

| RPi Pin | → | Device |
|---------|---|--------|
| GPIO17 (Pin 11) | → | Relay 1 → Buzzer 1 |
| GPIO27 (Pin 13) | → | Relay 2 → Buzzer 2 |
| 5V (Pin 2) | → | Relay VCC |
| GND (Pin 6) | → | Relay GND |

### 2. Install

```bash
# Transfer files to Raspberry Pi, then:
cd Master_Alarm
chmod +x scripts/install.sh
sudo ./scripts/install.sh
```

### 3. Configure Token

Set your InfluxDB API token in the systemd service file:
```bash
sudo nano /etc/systemd/system/slt-master-alarm.service
# Find: Environment=INFLUXDB_TOKEN=your-influxdb-api-token-here
# Replace with your actual token
sudo systemctl daemon-reload
```

### 4. Test

```bash
# Test GPIO wiring
sudo /opt/slt-master-alarm/venv/bin/python /opt/slt-master-alarm/scripts/test_gpio.py

# Test application manually
export INFLUXDB_TOKEN='your-token'
cd /opt/slt-master-alarm
sudo -E venv/bin/python -u src/main.py
```

### 5. Deploy

```bash
sudo systemctl start slt-master-alarm
sudo systemctl status slt-master-alarm
journalctl -u slt-master-alarm -f
```

---

## How It Works

```
Software Team writes              Raspberry Pi reads
master_alarm boolean              master_alarm boolean
to InfluxDB                       from InfluxDB
        │                                 │
        ▼                                 ▼
┌─────────────────┐              ┌─────────────────┐
│  InfluxDB v2     │◄─── HTTP ───│  Python App      │
│                  │   (1s poll)  │                  │
│  Bucket:         │              │  alarm == true   │──▶ GPIO17 ON → Buzzer 1
│  Lift_Alarm_     │              │                  │──▶ GPIO27 ON → Buzzer 2
│  Status          │              │                  │
│                  │              │  alarm == false   │──▶ GPIO17 OFF
│  Measurement:    │              │                  │──▶ GPIO27 OFF
│  master_alarm    │              │                  │
└──────────────────┘              └──────────────────┘
```

---

## Project Structure

```
Master_Alarm/
├── README.md                          ← You are here
├── config/
│   └── config.yaml                   ← All configurable parameters
├── src/
│   ├── __init__.py
│   ├── main.py                       ← Application entry point
│   ├── config_loader.py              ← YAML config parser + validation
│   ├── influxdb_client_wrapper.py    ← InfluxDB connection & query
│   ├── gpio_controller.py            ← GPIO relay control
│   ├── alarm_monitor.py              ← Main polling loop & state management
│   └── logger_setup.py               ← Centralized logging
├── service/
│   └── slt-master-alarm.service      ← systemd auto-start service
├── scripts/
│   ├── install.sh                    ← Automated setup script
│   └── test_gpio.py                  ← GPIO hardware test
├── docs/
│   ├── architecture.md               ← System architecture & diagrams
│   ├── wiring_diagram.md             ← Hardware wiring guide
│   ├── testing_guide.md              ← Full test procedure
│   ├── deployment_guide.md           ← Production deployment steps
│   └── future_improvements.md        ← Roadmap & enhancements
├── requirements.txt                  ← Python dependencies
└── .env.example                      ← Environment variable template
```

---

## InfluxDB Configuration

| Parameter | Value |
|-----------|-------|
| **URL** | `http://124.43.179.232:8086` |
| **Organisation** | `SLT` |
| **Bucket** | `Lift_Alarm_Status` |
| **Measurement** | `master_alarm` |
| **Field** | `alarm` |
| **Value Type** | Boolean (`true` / `false`) |

### Flux Query

```flux
from(bucket: "Lift_Alarm_Status")
  |> range(start: -5m)
  |> filter(fn: (r) => r._measurement == "master_alarm")
  |> filter(fn: (r) => r._field == "alarm")
  |> last()
```

---

## Key Features

| Feature | Description |
|---------|-------------|
| **1-Second Polling** | Reads master alarm every second for near-real-time response |
| **State Change Detection** | Only triggers GPIO on alarm transitions (reduces relay wear) |
| **Auto-Reconnection** | Exponential backoff (5s → 10s → 20s → 40s → 60s cap) |
| **Never Crashes** | Comprehensive exception handling — survives network outages |
| **Auto-Start on Boot** | systemd service with `Restart=always` |
| **Dual Logging** | Console (for systemd journal) + rotating log file |
| **Configurable** | All parameters in one YAML file |
| **Simulation Mode** | Runs on non-RPi systems for development/testing |

---

## Documentation

| Document | Description |
|----------|-------------|
| [Architecture](docs/architecture.md) | System overview, data flow, state machine |
| [Wiring Diagram](docs/wiring_diagram.md) | GPIO pinout, relay wiring, safety notes |
| [Testing Guide](docs/testing_guide.md) | 7 test procedures with sign-off checklist |
| [Deployment Guide](docs/deployment_guide.md) | Step-by-step production deployment |
| [Future Improvements](docs/future_improvements.md) | Roadmap: email, SMS, dashboard, watchdog |

---

## Service Management

```bash
sudo systemctl start slt-master-alarm        # Start
sudo systemctl stop slt-master-alarm         # Stop
sudo systemctl restart slt-master-alarm      # Restart
sudo systemctl status slt-master-alarm       # Status
journalctl -u slt-master-alarm -f            # Live logs
journalctl -u slt-master-alarm --since today # Today's logs
```

---

## Requirements

### Hardware
- Raspberry Pi 4 or 5
- 2× 5V Relay modules (with optocoupler)
- 2× Industrial buzzers (12V/24V)
- Ethernet connection

### Software
- Raspberry Pi OS (Bullseye or Bookworm)
- Python 3.9+
- `influxdb-client`, `PyYAML`, `RPi.GPIO`

---

## Contact

For questions about the Master Alarm system, contact the SLT Mobitel IoT / Building Management team.
