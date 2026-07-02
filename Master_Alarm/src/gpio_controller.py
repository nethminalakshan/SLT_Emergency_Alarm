"""
SLT Lift Master Alarm System — GPIO Controller
================================================
Manages Raspberry Pi GPIO pins for controlling relay modules
that drive emergency buzzers.

Supports both active-HIGH and active-LOW relay modules via configuration.

Usage:
    from gpio_controller import GPIOController
    gpio = GPIOController(config, logger)
    gpio.activate()    # Turn ON both buzzers
    gpio.deactivate()  # Turn OFF both buzzers
    gpio.cleanup()     # Release GPIO resources
"""

import logging
from typing import Optional

try:
    import RPi.GPIO as GPIO
    GPIO_AVAILABLE = True
except (ImportError, RuntimeError):
    # Allow importing on non-RPi systems (for development/testing)
    GPIO = None
    GPIO_AVAILABLE = False


class GPIOController:
    """
    Controls two relay-driven buzzers via Raspberry Pi GPIO pins.

    Features:
      - BCM pin numbering mode
      - Configurable active-HIGH or active-LOW relay logic
      - Safe initialization (pins start in OFF state)
      - Clean shutdown with GPIO.cleanup()
      - Development mode when RPi.GPIO is unavailable
    """

    def __init__(self, config: dict, logger: logging.Logger):
        """
        Initialize GPIO pins for buzzer relay control.

        Args:
            config: Full application configuration dictionary
            logger: Application logger instance
        """
        self.logger = logger

        # Pin assignments from config
        self._pin_1 = config["gpio"]["buzzer_1_pin"]
        self._pin_2 = config["gpio"]["buzzer_2_pin"]
        self._active_high = config["gpio"]["active_high"]

        # Track current state
        self._active = False
        self._initialized = False

        # Initialize GPIO
        self._setup_gpio()

    def _setup_gpio(self):
        """
        Configure GPIO pins as outputs and set them to the OFF state.
        Falls back to simulation mode if RPi.GPIO is not available.
        """
        if not GPIO_AVAILABLE:
            self.logger.warning(
                "⚠️  RPi.GPIO not available — running in SIMULATION mode. "
                "GPIO commands will be logged but not executed. "
                "This is normal on non-Raspberry Pi systems."
            )
            self._initialized = False
            return

        try:
            # Suppress GPIO warnings (e.g., channel already in use)
            GPIO.setwarnings(False)

            # Use BCM (Broadcom) pin numbering
            GPIO.setmode(GPIO.BCM)

            # Configure both pins as outputs
            GPIO.setup(self._pin_1, GPIO.OUT)
            GPIO.setup(self._pin_2, GPIO.OUT)

            # Set initial state to OFF
            self._write_off()

            self._initialized = True
            self.logger.info(
                f"✅ GPIO initialized — "
                f"Buzzer 1: GPIO{self._pin_1}, "
                f"Buzzer 2: GPIO{self._pin_2}, "
                f"Logic: {'active-HIGH' if self._active_high else 'active-LOW'}"
            )

        except Exception as e:
            self.logger.error(f"❌ GPIO initialization failed: {e}")
            self._initialized = False

    def _write_on(self):
        """Write the ON state to both GPIO pins based on relay logic."""
        if self._active_high:
            GPIO.output(self._pin_1, GPIO.HIGH)
            GPIO.output(self._pin_2, GPIO.HIGH)
        else:
            # Active-LOW: relay activates when pin is LOW
            GPIO.output(self._pin_1, GPIO.LOW)
            GPIO.output(self._pin_2, GPIO.LOW)

    def _write_off(self):
        """Write the OFF state to both GPIO pins based on relay logic."""
        if self._active_high:
            GPIO.output(self._pin_1, GPIO.LOW)
            GPIO.output(self._pin_2, GPIO.LOW)
        else:
            # Active-LOW: relay deactivates when pin is HIGH
            GPIO.output(self._pin_1, GPIO.HIGH)
            GPIO.output(self._pin_2, GPIO.HIGH)

    def activate(self):
        """
        Turn ON both buzzer relays (alarm active).

        Only writes to GPIO if the state has actually changed,
        reducing unnecessary relay switching and wear.
        """
        if self._active:
            return  # Already active — no action needed

        self._active = True

        if self._initialized:
            try:
                self._write_on()
                self.logger.info(
                    "🔴 ALARM ON — GPIO{} and GPIO{} activated".format(
                        self._pin_1, self._pin_2
                    )
                )
            except Exception as e:
                self.logger.error(f"Failed to activate GPIO: {e}")
        else:
            self.logger.info(
                "[SIMULATION] 🔴 ALARM ON — "
                f"GPIO{self._pin_1} and GPIO{self._pin_2} would be activated"
            )

    def deactivate(self):
        """
        Turn OFF both buzzer relays (alarm cleared).

        Only writes to GPIO if the state has actually changed.
        """
        if not self._active:
            return  # Already inactive — no action needed

        self._active = False

        if self._initialized:
            try:
                self._write_off()
                self.logger.info(
                    "🟢 ALARM OFF — GPIO{} and GPIO{} deactivated".format(
                        self._pin_1, self._pin_2
                    )
                )
            except Exception as e:
                self.logger.error(f"Failed to deactivate GPIO: {e}")
        else:
            self.logger.info(
                "[SIMULATION] 🟢 ALARM OFF — "
                f"GPIO{self._pin_1} and GPIO{self._pin_2} would be deactivated"
            )

    @property
    def is_active(self) -> bool:
        """Check if the buzzers are currently active."""
        return self._active

    @property
    def is_initialized(self) -> bool:
        """Check if GPIO was successfully initialized."""
        return self._initialized

    def cleanup(self):
        """
        Release GPIO resources safely.

        Called on application shutdown to ensure GPIO pins are reset
        and resources are freed. This prevents pins from being left
        in an undefined state.
        """
        if self._initialized:
            try:
                # Ensure buzzers are OFF before cleanup
                self._write_off()
                GPIO.cleanup()
                self.logger.info("GPIO cleaned up successfully.")
            except Exception as e:
                self.logger.warning(f"Error during GPIO cleanup: {e}")
            finally:
                self._initialized = False
                self._active = False
        else:
            self.logger.info("[SIMULATION] GPIO cleanup — no real pins to release.")
