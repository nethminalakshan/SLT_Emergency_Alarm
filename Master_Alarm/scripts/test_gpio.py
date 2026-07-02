#!/usr/bin/env python3
"""
SLT Lift Master Alarm System — GPIO Hardware Test
===================================================
Standalone test script to verify relay and buzzer wiring.

This script cycles through the GPIO pins to confirm:
  1. Both relay modules respond to GPIO signals
  2. Both buzzers sound when relays are activated
  3. Correct pin-to-relay mapping

Usage:
    sudo python3 scripts/test_gpio.py

Expected behavior:
    - Buzzer 1 (GPIO17) activates for 2 seconds, then deactivates
    - Buzzer 2 (GPIO27) activates for 2 seconds, then deactivates
    - Both buzzers activate together for 3 seconds, then deactivate
    - GPIO pins are cleaned up at exit
"""

import sys
import time

try:
    import RPi.GPIO as GPIO
except (ImportError, RuntimeError):
    print("ERROR: RPi.GPIO is not available.")
    print("This script must be run on a Raspberry Pi.")
    print("Install with: sudo apt install python3-rpi.gpio")
    sys.exit(1)


# --- Pin Configuration ---
BUZZER_1_PIN = 17    # BCM GPIO17 → Physical Pin 11
BUZZER_2_PIN = 27    # BCM GPIO27 → Physical Pin 13
ACTIVE_HIGH = True   # Set to False for active-LOW relay modules


def setup():
    """Initialize GPIO pins."""
    GPIO.setwarnings(False)
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(BUZZER_1_PIN, GPIO.OUT)
    GPIO.setup(BUZZER_2_PIN, GPIO.OUT)

    # Start with both OFF
    off_state = GPIO.LOW if ACTIVE_HIGH else GPIO.HIGH
    GPIO.output(BUZZER_1_PIN, off_state)
    GPIO.output(BUZZER_2_PIN, off_state)


def activate_pin(pin):
    """Turn ON a relay."""
    state = GPIO.HIGH if ACTIVE_HIGH else GPIO.LOW
    GPIO.output(pin, state)


def deactivate_pin(pin):
    """Turn OFF a relay."""
    state = GPIO.LOW if ACTIVE_HIGH else GPIO.HIGH
    GPIO.output(pin, state)


def run_test():
    """Execute the GPIO test sequence."""
    print("=" * 50)
    print("  SLT MASTER ALARM — GPIO TEST")
    print("=" * 50)
    print(f"  Buzzer 1: GPIO{BUZZER_1_PIN} (Physical Pin 11)")
    print(f"  Buzzer 2: GPIO{BUZZER_2_PIN} (Physical Pin 13)")
    print(f"  Logic: {'Active-HIGH' if ACTIVE_HIGH else 'Active-LOW'}")
    print("=" * 50)
    print()

    try:
        setup()
        print("GPIO initialized. Starting test sequence...\n")
        time.sleep(1)

        # --- Test 1: Buzzer 1 Only ---
        print("📢 TEST 1: Buzzer 1 (GPIO17) — ON for 2 seconds...")
        activate_pin(BUZZER_1_PIN)
        time.sleep(2)
        deactivate_pin(BUZZER_1_PIN)
        print("   Buzzer 1 OFF.")
        print("   ✅ Did you hear Buzzer 1? (located at ___)")
        print()
        time.sleep(1)

        # --- Test 2: Buzzer 2 Only ---
        print("📢 TEST 2: Buzzer 2 (GPIO27) — ON for 2 seconds...")
        activate_pin(BUZZER_2_PIN)
        time.sleep(2)
        deactivate_pin(BUZZER_2_PIN)
        print("   Buzzer 2 OFF.")
        print("   ✅ Did you hear Buzzer 2? (located at ___)")
        print()
        time.sleep(1)

        # --- Test 3: Both Buzzers Together ---
        print("📢 TEST 3: Both Buzzers — ON for 3 seconds...")
        activate_pin(BUZZER_1_PIN)
        activate_pin(BUZZER_2_PIN)
        time.sleep(3)
        deactivate_pin(BUZZER_1_PIN)
        deactivate_pin(BUZZER_2_PIN)
        print("   Both Buzzers OFF.")
        print("   ✅ Did you hear BOTH buzzers?")
        print()
        time.sleep(1)

        # --- Test 4: Rapid Toggle ---
        print("📢 TEST 4: Rapid toggle (5 cycles, 0.5s each)...")
        for i in range(5):
            activate_pin(BUZZER_1_PIN)
            activate_pin(BUZZER_2_PIN)
            time.sleep(0.5)
            deactivate_pin(BUZZER_1_PIN)
            deactivate_pin(BUZZER_2_PIN)
            time.sleep(0.5)
            print(f"   Cycle {i + 1}/5 complete.")
        print("   ✅ Rapid toggle test complete.")
        print()

        # --- Done ---
        print("=" * 50)
        print("  ✅ ALL GPIO TESTS COMPLETE")
        print("=" * 50)
        print()
        print("  If all buzzers responded correctly, the wiring is good!")
        print("  If a buzzer did NOT sound:")
        print("    1. Check the relay module power supply (5V)")
        print("    2. Check the GPIO wire connections")
        print("    3. Check the buzzer wiring from the relay output")
        print("    4. Try setting ACTIVE_HIGH = False in this script")
        print()

    except KeyboardInterrupt:
        print("\n\nTest interrupted by user.")
    finally:
        GPIO.cleanup()
        print("GPIO cleaned up.")


if __name__ == "__main__":
    run_test()
