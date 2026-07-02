"""
SLT Lift Master Alarm System — Alarm Monitor
==============================================
Core polling loop that continuously reads the master alarm status from
InfluxDB and controls the buzzer relays accordingly.

This module orchestrates the InfluxDB client and GPIO controller,
handling state transitions, connection recovery, and graceful shutdown.

Usage:
    from alarm_monitor import AlarmMonitor
    monitor = AlarmMonitor(config, logger)
    monitor.run()  # Blocks — runs until SIGTERM/SIGINT
"""

import time
import signal
import logging
from typing import Optional

from .influxdb_client_wrapper import InfluxDBClientWrapper
from .gpio_controller import GPIOController


class AlarmMonitor:
    """
    Main alarm monitoring loop.

    Polls InfluxDB at a configurable interval, compares the alarm state
    to the previous state, and triggers GPIO changes only on transitions.

    Features:
      - State-change detection (only acts on alarm transitions)
      - Automatic InfluxDB reconnection on connection loss
      - Signal handling for clean shutdown (SIGTERM, SIGINT)
      - Continuous 24/7 operation with comprehensive error handling
    """

    def __init__(self, config: dict, logger: logging.Logger):
        """
        Initialize the alarm monitor.

        Args:
            config: Full application configuration dictionary
            logger: Application logger instance
        """
        self.logger = logger
        self._config = config
        self._poll_interval = config["polling"]["interval_seconds"]

        # Track alarm state (None = unknown/initial)
        self._current_alarm_state: Optional[bool] = None

        # Running flag — set to False by signal handlers to stop the loop
        self._running = True

        # Initialize sub-components
        self._influxdb = InfluxDBClientWrapper(config, logger)
        self._gpio = GPIOController(config, logger)

        # Register signal handlers for graceful shutdown
        signal.signal(signal.SIGTERM, self._handle_shutdown_signal)
        signal.signal(signal.SIGINT, self._handle_shutdown_signal)

        self.logger.info("AlarmMonitor initialized.")

    def _handle_shutdown_signal(self, signum, frame):
        """
        Handle SIGTERM and SIGINT for graceful shutdown.

        Sets the running flag to False, which causes the main loop
        to exit cleanly on the next iteration.
        """
        sig_name = signal.Signals(signum).name
        self.logger.info(
            f"Received {sig_name} — initiating graceful shutdown..."
        )
        self._running = False

    def run(self):
        """
        Start the main alarm monitoring loop.

        This method blocks and runs continuously until a shutdown signal
        is received. The loop:
          1. Ensures InfluxDB connection is active
          2. Queries the master alarm status
          3. Compares to previous state and triggers GPIO on change
          4. Sleeps for the configured interval
          5. Repeats

        On connection failure, the loop enters reconnection mode with
        exponential backoff, maintaining the current alarm state.
        """
        self.logger.info("=" * 60)
        self.logger.info("SLT LIFT MASTER ALARM SYSTEM — STARTING")
        self.logger.info("=" * 60)
        self.logger.info(
            f"Polling interval: {self._poll_interval}s | "
            f"InfluxDB: {self._config['influxdb']['url']} | "
            f"Bucket: {self._config['influxdb']['bucket']}"
        )

        # Initial connection
        if not self._influxdb.connect():
            self.logger.critical("Failed to establish initial InfluxDB connection.")
            self._cleanup()
            return

        # Consecutive error counter for logging throttling
        consecutive_errors = 0

        try:
            while self._running:
                try:
                    # --- Query InfluxDB ---
                    alarm_value = self._influxdb.get_master_alarm()

                    if alarm_value is None:
                        # Query failed or no data
                        consecutive_errors += 1

                        if not self._influxdb.is_connected:
                            self.logger.warning(
                                "Connection lost — attempting to reconnect..."
                            )
                            self._influxdb.connect()
                            consecutive_errors = 0

                        elif consecutive_errors <= 3 or consecutive_errors % 30 == 0:
                            # Log first few errors, then throttle
                            self.logger.warning(
                                f"No alarm data received "
                                f"(consecutive: {consecutive_errors})"
                            )

                    else:
                        # Reset error counter on success
                        if consecutive_errors > 0:
                            self.logger.info(
                                f"Connection recovered after "
                                f"{consecutive_errors} failed queries."
                            )
                        consecutive_errors = 0

                        # --- State Change Detection ---
                        self._process_alarm_state(alarm_value)

                except Exception as e:
                    consecutive_errors += 1
                    if consecutive_errors <= 3 or consecutive_errors % 30 == 0:
                        self.logger.error(
                            f"Error in monitoring loop: {e} "
                            f"(consecutive: {consecutive_errors})"
                        )

                # --- Sleep Until Next Poll ---
                # Use short sleep intervals to remain responsive to signals
                sleep_remaining = self._poll_interval
                while sleep_remaining > 0 and self._running:
                    sleep_step = min(sleep_remaining, 0.5)
                    time.sleep(sleep_step)
                    sleep_remaining -= sleep_step

        finally:
            self._cleanup()

    def _process_alarm_state(self, new_state: bool):
        """
        Process a new alarm state value and trigger GPIO if changed.

        Only acts on state *transitions* — repeated identical states
        are silently ignored (no GPIO writes, no log spam).

        Args:
            new_state: The latest alarm boolean from InfluxDB
        """
        if new_state == self._current_alarm_state:
            # No change — nothing to do
            return

        previous = self._current_alarm_state
        self._current_alarm_state = new_state

        if new_state:
            self.logger.warning(
                f"⚠️  ALARM STATE CHANGED: "
                f"{'INACTIVE' if previous is False else 'UNKNOWN'} → ACTIVE"
            )
            self._gpio.activate()
        else:
            self.logger.info(
                f"✅ ALARM STATE CHANGED: "
                f"{'ACTIVE' if previous is True else 'UNKNOWN'} → INACTIVE"
            )
            self._gpio.deactivate()

    def _cleanup(self):
        """
        Perform clean shutdown of all resources.

        Ensures GPIO pins are released and InfluxDB client is closed,
        even if errors occur during cleanup.
        """
        self.logger.info("Shutting down alarm monitor...")

        # Deactivate buzzers first
        try:
            self._gpio.deactivate()
        except Exception as e:
            self.logger.warning(f"Error deactivating GPIO: {e}")

        # Clean up GPIO
        try:
            self._gpio.cleanup()
        except Exception as e:
            self.logger.warning(f"Error cleaning up GPIO: {e}")

        # Close InfluxDB connection
        try:
            self._influxdb.close()
        except Exception as e:
            self.logger.warning(f"Error closing InfluxDB: {e}")

        self.logger.info("=" * 60)
        self.logger.info("SLT LIFT MASTER ALARM SYSTEM — STOPPED")
        self.logger.info("=" * 60)
