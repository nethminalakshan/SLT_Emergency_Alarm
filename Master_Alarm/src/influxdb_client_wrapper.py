"""
SLT Lift Master Alarm System — InfluxDB Client Wrapper
========================================================
Manages the connection to InfluxDB v2 and queries the master alarm status.
Handles connection failures with exponential backoff retry logic.

Usage:
    from influxdb_client_wrapper import InfluxDBClientWrapper
    client = InfluxDBClientWrapper(config, logger)
    alarm_state = client.get_master_alarm()
"""

import time
import logging
from typing import Optional

from influxdb_client import InfluxDBClient
from influxdb_client.client.exceptions import InfluxDBError


class InfluxDBClientWrapper:
    """
    Wrapper around the InfluxDB v2 Python client.

    Provides:
      - Connection management with health checks
      - Master alarm query execution
      - Automatic reconnection with exponential backoff
      - Comprehensive error logging
    """

    def __init__(self, config: dict, logger: logging.Logger):
        """
        Initialize the InfluxDB client wrapper.

        Args:
            config: Full application configuration dictionary
            logger: Application logger instance
        """
        self.logger = logger

        # InfluxDB connection parameters
        self._url = config["influxdb"]["url"]
        self._token = config["influxdb"]["token"]
        self._org = config["influxdb"]["org"]
        self._bucket = config["influxdb"]["bucket"]
        self._measurement = config["influxdb"]["measurement"]
        self._field = config["influxdb"]["field"]
        self._query_range = config["polling"]["query_range"]

        # Reconnection parameters
        self._retry_delay = config["reconnection"]["retry_delay_seconds"]
        self._backoff_multiplier = config["reconnection"]["backoff_multiplier"]
        self._max_delay = config["reconnection"]["max_delay_seconds"]
        self._max_retries = config["reconnection"].get("max_retries", 0)

        # Build the Flux query
        self._flux_query = self._build_flux_query()

        # Client instance (initialized on connect)
        self._client: Optional[InfluxDBClient] = None
        self._query_api = None
        self._connected = False

    def _build_flux_query(self) -> str:
        """
        Build the Flux query string to retrieve the latest master alarm value.

        Returns:
            str: Flux query string
        """
        query = (
            f'from(bucket: "{self._bucket}")\n'
            f"  |> range(start: {self._query_range})\n"
            f'  |> filter(fn: (r) => r._measurement == "{self._measurement}")\n'
            f'  |> filter(fn: (r) => r._field == "{self._field}")\n'
            f"  |> last()"
        )
        self.logger.debug(f"Flux query built:\n{query}")
        return query

    def connect(self) -> bool:
        """
        Establish a connection to InfluxDB with retry logic.

        Retries indefinitely (or up to max_retries) with exponential backoff
        until the connection succeeds. This ensures the application survives
        temporary network outages or server restarts.

        Returns:
            bool: True when connection is established
        """
        attempt = 0
        current_delay = self._retry_delay

        while True:
            attempt += 1

            try:
                # Close existing client if any
                if self._client:
                    try:
                        self._client.close()
                    except Exception:
                        pass

                self.logger.info(
                    f"Connecting to InfluxDB at {self._url} "
                    f"(attempt {attempt})..."
                )

                # Create new client
                self._client = InfluxDBClient(
                    url=self._url,
                    token=self._token,
                    org=self._org,
                    timeout=10000  # 10 second timeout
                )

                # Verify connection with health check
                health = self._client.health()
                if health.status == "pass":
                    self._query_api = self._client.query_api()
                    self._connected = True
                    self.logger.info(
                        f"✅ Connected to InfluxDB successfully. "
                        f"Server version: {health.version}"
                    )
                    return True
                else:
                    raise ConnectionError(
                        f"InfluxDB health check failed: {health.message}"
                    )

            except Exception as e:
                self._connected = False
                self.logger.error(
                    f"❌ InfluxDB connection failed (attempt {attempt}): {e}"
                )

                # Check max retries (0 = infinite)
                if self._max_retries > 0 and attempt >= self._max_retries:
                    self.logger.critical(
                        f"Maximum connection retries ({self._max_retries}) "
                        f"exceeded. Giving up."
                    )
                    return False

                self.logger.info(
                    f"Retrying in {current_delay} seconds..."
                )
                time.sleep(current_delay)

                # Exponential backoff with cap
                current_delay = min(
                    current_delay * self._backoff_multiplier,
                    self._max_delay
                )

    def get_master_alarm(self) -> Optional[bool]:
        """
        Query InfluxDB for the latest master alarm value.

        Returns:
            bool: True if alarm is active, False if alarm is inactive
            None: If query fails or no data is found

        The caller should handle None by maintaining the previous alarm state
        and triggering reconnection if needed.
        """
        if not self._connected or not self._query_api:
            self.logger.warning("Not connected to InfluxDB. Cannot query.")
            return None

        try:
            tables = self._query_api.query(self._flux_query)

            # Extract the alarm value from query results
            for table in tables:
                for record in table.records:
                    value = record.get_value()
                    self.logger.debug(
                        f"Raw alarm value from InfluxDB: {value} "
                        f"(type: {type(value).__name__})"
                    )
                    return self._parse_alarm_value(value)

            # No data found in the query range
            self.logger.warning(
                f"No master_alarm data found in the last "
                f"{self._query_range}. Is the software team writing "
                f"to the '{self._bucket}' bucket?"
            )
            return None

        except InfluxDBError as e:
            self.logger.error(f"InfluxDB query error: {e}")
            self._connected = False
            return None

        except Exception as e:
            self.logger.error(f"Unexpected error querying InfluxDB: {e}")
            self._connected = False
            return None

    def _parse_alarm_value(self, value) -> bool:
        """
        Parse the alarm value from InfluxDB into a Python boolean.

        Handles multiple possible data types:
          - bool: True/False (expected)
          - int/float: 1/0
          - str: "true"/"false", "1"/"0"

        Args:
            value: Raw value from InfluxDB

        Returns:
            bool: Parsed alarm state
        """
        if isinstance(value, bool):
            return value
        if isinstance(value, (int, float)):
            return bool(value)
        if isinstance(value, str):
            return value.lower() in ("true", "1", "yes", "on")

        self.logger.warning(
            f"Unexpected alarm value type: {type(value).__name__} = {value}. "
            f"Treating as False."
        )
        return False

    @property
    def is_connected(self) -> bool:
        """Check if the client is currently connected."""
        return self._connected

    def close(self):
        """Close the InfluxDB client connection gracefully."""
        if self._client:
            try:
                self._client.close()
                self.logger.info("InfluxDB client closed.")
            except Exception as e:
                self.logger.warning(f"Error closing InfluxDB client: {e}")
            finally:
                self._client = None
                self._query_api = None
                self._connected = False
