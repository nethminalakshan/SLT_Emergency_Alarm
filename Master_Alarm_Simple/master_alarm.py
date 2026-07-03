#!/usr/bin/env python3
import os
import sys
import time
import signal
import logging
from influxdb_client import InfluxDBClient

# --- CONFIGURATION ---
INFLUXDB_URL         = "http://124.43.179.232:8086"
INFLUXDB_ORG         = "SLT"
INFLUXDB_BUCKET      = "Lift_Alarm_Status"
INFLUXDB_MEASUREMENT = "master_alarm"
INFLUXDB_FIELD       = "alarm"
INFLUXDB_TOKEN       = "uuMjeruz55R_yGlhcMI1r2dZVGqUzcxSorC_aBu0jRrQXQ7cZX-OStRHoOTmG-1Gr4_Z7kKcwsPzSQINweI-UA=="

BUZZER_1_PIN  = 12   # BCM GPIO 12
BUZZER_2_PIN  = 16   # BCM GPIO 16
ACTIVE_HIGH   = True # Set to False if your 5V relay triggers on LOW
POLL_INTERVAL = 1

# --- LOGGING ---
logging.basicConfig(level=logging.INFO, format="%(asctime)s — %(message)s")

# --- GPIO SETUP ---
try:
    import RPi.GPIO as GPIO
    GPIO.setwarnings(False)
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(BUZZER_1_PIN, GPIO.OUT)
    GPIO.setup(BUZZER_2_PIN, GPIO.OUT)
    logging.info(f"GPIO ready on pins {BUZZER_1_PIN} and {BUZZER_2_PIN}")
except ImportError:
    GPIO = None
    logging.warning("RPi.GPIO not found. Running in simulation mode.")

running = True
def shutdown(signum, frame):
    global running
    running = False
signal.signal(signal.SIGINT, shutdown)
signal.signal(signal.SIGTERM, shutdown)

def set_buzzers(on):
    level = on if ACTIVE_HIGH else not on
    if GPIO:
        GPIO.output(BUZZER_1_PIN, level)
        GPIO.output(BUZZER_2_PIN, level)
    logging.info("🔴 BUZZERS ON" if on else "🟢 BUZZERS OFF")

def main():
    if not INFLUXDB_TOKEN:
        logging.error("INFLUXDB_TOKEN is empty! Please set it in the configuration.")
        sys.exit(1)

    query = (
        f'from(bucket: "{INFLUXDB_BUCKET}") '
        f'|> range(start: -5m) '
        f'|> filter(fn: (r) => r._measurement == "{INFLUXDB_MEASUREMENT}" and r._field == "{INFLUXDB_FIELD}") '
        f'|> last()'
    )

    client = InfluxDBClient(url=INFLUXDB_URL, token=INFLUXDB_TOKEN, org=INFLUXDB_ORG, timeout=10000)
    query_api = client.query_api()
    
    set_buzzers(False)
    last_state = False

    logging.info(f"Connecting to InfluxDB at {INFLUXDB_URL}...")
    
    while running:
        try:
            tables = query_api.query(query)
            current_state = False
            
            for table in tables:
                for record in table.records:
                    val = record.get_value()
                    if isinstance(val, str):
                        current_state = val.lower() in ("true", "1", "yes")
                    else:
                        current_state = bool(val)

            if current_state != last_state:
                set_buzzers(current_state)
                last_state = current_state

        except Exception as e:
            err_msg = str(e).lower()
            if "401" in err_msg or "unauthorized" in err_msg:
                logging.error(f"InfluxDB Auth Error! Check INFLUXDB_TOKEN for bucket '{INFLUXDB_BUCKET}'.")
                time.sleep(10) # Wait longer if token is bad
            else:
                logging.error(f"Connection error: {e}. Reconnecting...")
                time.sleep(2)
        
        time.sleep(POLL_INTERVAL)

    set_buzzers(False)
    if GPIO:
        GPIO.cleanup()
    client.close()
    logging.info("Master Alarm stopped.")

if __name__ == "__main__":
    main()
