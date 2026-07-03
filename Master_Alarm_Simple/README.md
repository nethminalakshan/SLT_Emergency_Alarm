# 🚨 SLT Lift Master Alarm — Simple

Reads the `master_alarm` boolean from InfluxDB and drives two buzzer relays via Raspberry Pi GPIO.

## Wiring

| RPi Pin | → | Device |
|---------|---|--------|
| GPIO12 (Pin 32) | → | Relay 1 → Buzzer 1 |
| GPIO16 (Pin 36) | → | Relay 2 → Buzzer 2 |
| 5V (Pin 2) | → | Relay VCC |
| GND (Pin 6) | → | Relay GND |

## Install (on Raspberry Pi)

```bash
cd Master_Alarm_Simple
chmod +x install.sh
sudo ./install.sh
```

The installer will:
1. Install system packages (`python3-venv`, `python3-rpi.gpio`)
2. Copy `master_alarm.py` to `/opt/slt-master-alarm/`
3. Create a Python venv and install `influxdb-client`
4. Install the systemd service (prompts for your InfluxDB token)
5. Enable auto-start on boot

## Run

```bash
sudo systemctl start slt-master-alarm      # Start
sudo systemctl status slt-master-alarm     # Check status
journalctl -u slt-master-alarm -f          # Live logs
sudo systemctl stop slt-master-alarm       # Stop
```

## Configuration

Edit the constants at the top of `master_alarm.py`:

| Constant | Default | Description |
|----------|---------|-------------|
| `INFLUXDB_URL` | `http://124.43.179.232:8086` | InfluxDB server |
| `INFLUXDB_ORG` | `SLT` | Organisation |
| `INFLUXDB_BUCKET` | `Lift_Alarm_Status` | Bucket name |
| `BUZZER_1_PIN` | `12` | BCM pin for buzzer 1 |
| `BUZZER_2_PIN` | `16` | BCM pin for buzzer 2 |
| `ACTIVE_HIGH` | `True` | `True` for active-HIGH relays |
| `POLL_INTERVAL` | `1` | Seconds between polls |

The InfluxDB token is set via the `INFLUXDB_TOKEN` environment variable (configured in the systemd service file).

## How It Works

```
InfluxDB ──(HTTP 1s poll)──▶ master_alarm.py ──▶ GPIO17 → Buzzer 1
                                               └──▶ GPIO27 → Buzzer 2

alarm == true  → buzzers ON
alarm == false → buzzers OFF
```

## Files

```
Master_Alarm_Simple/
├── master_alarm.py            ← The entire application (single file)
├── install.sh                 ← One-command installer
├── slt-master-alarm.service   ← systemd auto-start service
└── README.md                  ← This file
```
