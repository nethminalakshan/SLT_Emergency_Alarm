#!/usr/bin/env python3
"""
SLT Lift Master Alarm — Simple Single-File Version
Polls InfluxDB for the master_alarm boolean and drives two buzzer relays via GPIO.
"""

import os
import sys
import time
import signal
import logging

from influxdb_client import InfluxDBClient
from influxdb_client.client.exceptions import InfluxDBError
from urllib3.exceptions import HTTPError

# =============================================================================
# CONFIGURATION — Edit these values to match your setup
# =============================================================================
INFLUXDB_URL        = "http://124.43.179.232:8086"
INFLUXDB_ORG        = "SLT"
INFLUXDB_BUCKET     = "Lift_Alarm_Status"
INFLUXDB_MEASUREMENT = "master_alarm"
INFLUXDB_FIELD      = "alarm"

BUZZER_1_PIN = 12   # BCM GPIO12 (Pin 32) → Relay 1 → Buzzer 1
BUZZER_2_PIN = 16   # BCM GPIO16 (Pin 36) → Relay 2 → Buzzer 2
ACTIVE_HIGH  = True # True = GPIO HIGH activates relay

POLL_INTERVAL = 1   # Seconds between InfluxDB queries
RETRY_DELAY   = 5   # Seconds before reconnect attempt
MAX_AUTH_RETRY_DELAY = 300  # Max backoff for auth errors (5 min)
# =============================================================================

# --- Logging (console only — systemd journal captures it) ---
logging.basicConfig(
    format="%(asctime)s — %(levelname)-8s — %(name)s — %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
    level=logging.INFO,
)
log = logging.getLogger("slt_master_alarm")


class AuthError(Exception):
    """Raised when InfluxDB returns 401/403 — token is invalid or lacks permissions."""
    pass


# --- GPIO setup ---
try:
    import RPi.GPIO as GPIO
    GPIO.setwarnings(False)
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(BUZZER_1_PIN, GPIO.OUT)
    GPIO.setup(BUZZER_2_PIN, GPIO.OUT)
    SIMULATION = False
    log.info("GPIO ready — pins %d, %d", BUZZER_1_PIN, BUZZER_2_PIN)
except (ImportError, RuntimeError):
    GPIO = None
    SIMULATION = True
    log.warning("RPi.GPIO not available — SIMULATION mode")

# --- State ---
running = True
alarm_active = None  # None = unknown, True/False = last known state


def set_buzzers(on: bool):
    """Turn both buzzer relays ON or OFF."""
    level = on if ACTIVE_HIGH else (not on)
    if not SIMULATION:
        GPIO.output(BUZZER_1_PIN, level)
        GPIO.output(BUZZER_2_PIN, level)
    tag = "ON" if on else "OFF"
    log.info("🔴 BUZZERS %s" if on else "🟢 BUZZERS %s", tag)


def build_query() -> str:
    return (
        f'from(bucket: "{INFLUXDB_BUCKET}")\n'
        f"  |> range(start: -5m)\n"
        f'  |> filter(fn: (r) => r._measurement == "{INFLUXDB_MEASUREMENT}")\n'
        f'  |> filter(fn: (r) => r._field == "{INFLUXDB_FIELD}")\n'
        f"  |> last()"
    )


def _is_auth_error(exc: Exception) -> bool:
    """Check if an exception is an authentication/authorization error."""
    # InfluxDB client raises ApiException with a .status attribute
    status = getattr(exc, "status", None)
    if status in (401, 403):
        return True
    err_str = str(exc).lower()
    if "(401)" in err_str or "(403)" in err_str:
        return True
    if "unauthorized" in err_str or "forbidden" in err_str:
        return True
    return False


def query_alarm(query_api, flux_query) -> bool | None:
    """Query InfluxDB and return True/False/None.

    Raises AuthError for 401/403 so the caller can handle it differently
    from transient network errors.
    """
    try:
        tables = query_api.query(flux_query)
        for table in tables:
            for record in table.records:
                val = record.get_value()
                if isinstance(val, bool):
                    return val
                if isinstance(val, (int, float)):
                    return bool(val)
                if isinstance(val, str):
                    return val.lower() in ("true", "1", "yes")
                return False
        return None  # no data
    except Exception as e:
        if _is_auth_error(e):
            raise AuthError(str(e)) from e
        log.error("InfluxDB query error: %s", e)
        raise  # re-raise so the poll loop reconnects


def shutdown(signum, _frame):
    global running
    log.info("Received %s — shutting down", signal.Signals(signum).name)
    running = False


def connect_influxdb(token: str):
    """Create an InfluxDB client and verify connectivity.

    Returns (client, query_api) on success.
    Raises on failure.
    """
    client = InfluxDBClient(
        url=INFLUXDB_URL, token=token, org=INFLUXDB_ORG, timeout=10_000
    )
    health = client.health()
    if health.status != "pass":
        client.close()
        raise ConnectionError(health.message)
    query_api = client.query_api()
    log.info("✅ Connected to InfluxDB successfully. Server version: v%s", health.version)
    return client, query_api


def main():
    global alarm_active

    # --- Token check ---
    token = os.environ.get("INFLUXDB_TOKEN")
    if not token:
        log.critical("INFLUXDB_TOKEN env var not set. Exiting.")
        sys.exit(1)

    # --- Signal handlers ---
    signal.signal(signal.SIGTERM, shutdown)
    signal.signal(signal.SIGINT, shutdown)

    # --- Ensure buzzers start OFF ---
    set_buzzers(False)
    alarm_active = False

    flux_query = build_query()
    log.info("Master Alarm started — polling %s every %ds", INFLUXDB_URL, POLL_INTERVAL)

    consecutive_auth_failures = 0

    while running:
        # --- Connect ---
        client = None
        try:
            log.info("Connecting to InfluxDB at %s (attempt %d)...",
                      INFLUXDB_URL, consecutive_auth_failures + 1)
            client, query_api = connect_influxdb(token)
        except Exception as e:
            log.error("Connection failed: %s — retrying in %ds", e, RETRY_DELAY)
            time.sleep(RETRY_DELAY)
            continue

        # --- Poll loop ---
        try:
            while running:
                val = query_alarm(query_api, flux_query)
                # Reset auth failure counter on successful query
                consecutive_auth_failures = 0
                if val is not None and val != alarm_active:
                    alarm_active = val
                    set_buzzers(val)
                time.sleep(POLL_INTERVAL)

        except AuthError as e:
            consecutive_auth_failures += 1
            backoff = min(RETRY_DELAY * (2 ** consecutive_auth_failures),
                          MAX_AUTH_RETRY_DELAY)
            log.error("🔑 Authentication failed (attempt %d): %s",
                      consecutive_auth_failures, e)
            log.error("Token is invalid or lacks read access to bucket '%s'. "
                      "Please check your INFLUXDB_TOKEN.", INFLUXDB_BUCKET)
            log.warning("Retrying in %ds (will keep trying in case token is "
                        "updated via env reload)...", backoff)
            time.sleep(backoff)

        except Exception as e:
            consecutive_auth_failures = 0
            log.warning("Connection lost — attempting to reconnect...")

        finally:
            if client:
                try:
                    client.close()
                except Exception:
                    pass

    # --- Cleanup ---
    set_buzzers(False)
    if not SIMULATION:
        GPIO.cleanup()
    log.info("Master Alarm stopped.")


if __name__ == "__main__":
    main()
