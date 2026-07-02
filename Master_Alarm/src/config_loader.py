"""
SLT Lift Master Alarm System — Configuration Loader
=====================================================
Loads and validates the YAML configuration file.
Merges the InfluxDB API token from the INFLUXDB_TOKEN environment variable.

Usage:
    from config_loader import load_config
    config = load_config("/path/to/config.yaml")
"""

import os
import sys
import yaml


# Required top-level sections in config.yaml
REQUIRED_SECTIONS = ["influxdb", "gpio", "polling", "reconnection", "logging"]

# Required keys within each section
REQUIRED_KEYS = {
    "influxdb": ["url", "org", "bucket", "measurement", "field"],
    "gpio": ["buzzer_1_pin", "buzzer_2_pin", "active_high"],
    "polling": ["interval_seconds", "query_range"],
    "reconnection": ["retry_delay_seconds", "backoff_multiplier", "max_delay_seconds"],
    "logging": ["level", "file"],
}


def load_config(config_path: str) -> dict:
    """
    Load, validate, and return the application configuration.

    Steps:
      1. Read and parse the YAML file
      2. Validate that all required sections and keys are present
      3. Inject the InfluxDB token from the INFLUXDB_TOKEN environment variable
      4. Validate GPIO pin numbers are valid BCM pins

    Args:
        config_path: Absolute or relative path to config.yaml

    Returns:
        dict: Validated configuration dictionary

    Raises:
        FileNotFoundError: If config file does not exist
        ValueError: If required sections/keys are missing or invalid
        SystemExit: If INFLUXDB_TOKEN environment variable is not set
    """
    # --- Load YAML File ---
    if not os.path.exists(config_path):
        raise FileNotFoundError(f"Configuration file not found: {config_path}")

    with open(config_path, "r", encoding="utf-8") as f:
        config = yaml.safe_load(f)

    if not config or not isinstance(config, dict):
        raise ValueError(f"Configuration file is empty or invalid: {config_path}")

    # --- Validate Required Sections ---
    for section in REQUIRED_SECTIONS:
        if section not in config:
            raise ValueError(
                f"Missing required configuration section: '{section}'. "
                f"Check your config.yaml file."
            )

    # --- Validate Required Keys Within Sections ---
    for section, keys in REQUIRED_KEYS.items():
        for key in keys:
            if key not in config[section]:
                raise ValueError(
                    f"Missing required key '{key}' in section '{section}'. "
                    f"Check your config.yaml file."
                )

    # --- Inject InfluxDB Token from Environment Variable ---
    token = os.environ.get("INFLUXDB_TOKEN")
    if not token:
        print(
            "ERROR: INFLUXDB_TOKEN environment variable is not set.\n"
            "Set it with:\n"
            "  export INFLUXDB_TOKEN='your-token-here'\n"
            "Or add it to the systemd service file.\n"
            "See .env.example for reference.",
            file=sys.stderr
        )
        sys.exit(1)

    config["influxdb"]["token"] = token

    # --- Validate GPIO Pin Numbers ---
    valid_bcm_pins = set(range(0, 28))  # BCM GPIO 0-27 on Raspberry Pi
    for pin_key in ["buzzer_1_pin", "buzzer_2_pin"]:
        pin = config["gpio"][pin_key]
        if not isinstance(pin, int) or pin not in valid_bcm_pins:
            raise ValueError(
                f"Invalid GPIO pin '{pin}' for '{pin_key}'. "
                f"Must be a BCM pin number between 0 and 27."
            )

    # Ensure the two pins are different
    if config["gpio"]["buzzer_1_pin"] == config["gpio"]["buzzer_2_pin"]:
        raise ValueError(
            "buzzer_1_pin and buzzer_2_pin must be different GPIO pins. "
            f"Both are set to GPIO{config['gpio']['buzzer_1_pin']}."
        )

    # --- Validate Polling Interval ---
    interval = config["polling"]["interval_seconds"]
    if not isinstance(interval, (int, float)) or interval < 0.1:
        raise ValueError(
            f"polling.interval_seconds must be at least 0.1 seconds, got: {interval}"
        )

    return config
