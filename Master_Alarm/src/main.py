#!/usr/bin/env python3
"""
SLT Lift Master Alarm System — Main Entry Point
=================================================
Industrial Lift Emergency Alarm System for Sri Lanka Telecom.

Reads the master alarm boolean from InfluxDB and controls two
relay-driven buzzers via Raspberry Pi GPIO pins.

This script:
  1. Loads configuration from config/config.yaml
  2. Initializes the logging system
  3. Creates and starts the AlarmMonitor
  4. Runs continuously until SIGTERM/SIGINT

Usage:
    # Direct execution:
    export INFLUXDB_TOKEN='your-token-here'
    python -u src/main.py

    # Via systemd service:
    sudo systemctl start slt-master-alarm

Author: SLT Mobitel IoT / Building Management Team
"""

import os
import sys

# Ensure the project root is in the Python path
# This allows imports to work both when running directly and via systemd
script_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.dirname(script_dir)
sys.path.insert(0, project_root)

from src.config_loader import load_config
from src.logger_setup import setup_logging
from src.alarm_monitor import AlarmMonitor


# Default config path (relative to project root)
DEFAULT_CONFIG_PATH = os.path.join(project_root, "config", "config.yaml")


def main():
    """
    Application entry point.

    Loads configuration, sets up logging, and starts the alarm monitor.
    Handles fatal startup errors gracefully.
    """
    print("=" * 60)
    print("  SLT LIFT MASTER ALARM SYSTEM")
    print("  Sri Lanka Telecom — Building Emergency Monitoring")
    print("=" * 60)

    # --- Determine Config Path ---
    config_path = os.environ.get("ALARM_CONFIG_PATH", DEFAULT_CONFIG_PATH)
    print(f"Loading configuration from: {config_path}")

    # --- Load Configuration ---
    try:
        config = load_config(config_path)
    except FileNotFoundError as e:
        print(f"FATAL: {e}", file=sys.stderr)
        sys.exit(1)
    except ValueError as e:
        print(f"FATAL: Configuration error — {e}", file=sys.stderr)
        sys.exit(1)

    # --- Initialize Logging ---
    logger = setup_logging(config["logging"])
    logger.info("Configuration loaded successfully.")

    # Log configuration summary (without sensitive data)
    logger.info(
        f"InfluxDB: {config['influxdb']['url']} | "
        f"Org: {config['influxdb']['org']} | "
        f"Bucket: {config['influxdb']['bucket']}"
    )
    logger.info(
        f"GPIO: Buzzer 1 = GPIO{config['gpio']['buzzer_1_pin']}, "
        f"Buzzer 2 = GPIO{config['gpio']['buzzer_2_pin']} | "
        f"Logic: {'active-HIGH' if config['gpio']['active_high'] else 'active-LOW'}"
    )
    logger.info(
        f"Polling: {config['polling']['interval_seconds']}s interval | "
        f"Query range: {config['polling']['query_range']}"
    )

    # --- Start Alarm Monitor ---
    try:
        monitor = AlarmMonitor(config, logger)
        monitor.run()
    except KeyboardInterrupt:
        logger.info("Interrupted by user (Ctrl+C).")
    except Exception as e:
        logger.critical(f"Fatal error: {e}", exc_info=True)
        sys.exit(1)

    logger.info("Application exited cleanly.")


if __name__ == "__main__":
    main()
