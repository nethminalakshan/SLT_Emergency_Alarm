# 🚨 SLT Lift Master Alarm — Simple

Reads the `master_alarm` boolean from InfluxDB and drives two buzzer relays via Raspberry Pi GPIO.

## Wiring

| RPi Pin | → | Device |
|---------|---|--------|
| GPIO12 (Pin 32) | → | Relay 1 → Buzzer 1 |
| GPIO16 (Pin 36) | → | Relay 2 → Buzzer 2 |
| 5V (Pin 2) | → | Relay VCC |
| GND (Pin 6) | → | Relay GND |

## How to Run

You only need **one file** (`master_alarm.py`) to run this.

1. Ensure the required InfluxDB client library is installed:
   ```bash
   pip3 install influxdb-client
   ```

2. Run the script:
   ```bash
   python3 master_alarm.py
   ```

## Configuration

Everything is configured directly inside `master_alarm.py` at the top of the file:

- **INFLUXDB_URL**: The InfluxDB server (default `http://124.43.179.232:8086`)
- **INFLUXDB_TOKEN**: The API token with read access to the bucket.
- **BUZZER_1_PIN** / **BUZZER_2_PIN**: The BCM GPIO pins to use.
- **ACTIVE_HIGH**: Set to `True` for standard active-high relays, or `False` if your relay triggers on LOW (common for 5V relays).

## How It Works

```
InfluxDB ──(HTTP 1s poll)──▶ master_alarm.py ──▶ GPIO12 → Buzzer 1
                                               └──▶ GPIO16 → Buzzer 2

alarm == true  → buzzers ON
alarm == false → buzzers OFF
```
