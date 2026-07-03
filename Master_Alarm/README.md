# 🚨 SLT Lift Master Alarm

A lightweight, single-file Python script that reads the `master_alarm` status from InfluxDB and toggles two buzzer relays via Raspberry Pi GPIO.

## 📌 Wiring Guide

Connect your relay modules to the Raspberry Pi as follows:

| Raspberry Pi Pin | → | Relay / Buzzer |
|------------------|---|----------------|
| **GPIO 12 (Pin 32)** | → | Relay 1 (Buzzer 1) |
| **GPIO 16 (Pin 36)** | → | Relay 2 (Buzzer 2) |
| **5V (Pin 2 or 4)**  | → | Relay VCC (Power) |
| **GND (Pin 6 or 9)** | → | Relay GND (Ground) |

## 🚀 How to Run

You only need the `master_alarm.py` file. Everything is self-contained.

1. **Install the required library:**
   ```bash
   pip3 install influxdb-client
   ```

2. **Run the script:**
   ```bash
   python3 master_alarm.py
   ```

## ⚙️ Configuration

All settings are configured directly at the top of the `master_alarm.py` file. You do not need to set any environment variables.

```python
# Inside master_alarm.py:
INFLUXDB_URL         = "http://124.43.179.232:8086"
INFLUXDB_TOKEN       = "uuMjeruz..."  # Your API token is already set here
BUZZER_1_PIN         = 12
BUZZER_2_PIN         = 16
ACTIVE_HIGH          = True   # Change to False if your relays trigger on LOW
```

## 🔍 How It Works

1. The script polls your InfluxDB server every 1 second.
2. It fetches the latest `alarm` boolean value from the `Lift_Alarm_Status` bucket.
3. If `alarm == true`, it turns the buzzers **ON**.
4. If `alarm == false`, it turns the buzzers **OFF**.
5. It includes built-in retry logic to automatically handle network drops or InfluxDB restarts.
