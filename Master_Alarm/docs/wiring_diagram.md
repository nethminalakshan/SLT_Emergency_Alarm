# 🔌 Wiring Diagram — Master Alarm Hardware

## Components Required

| Component | Quantity | Specification | Purpose |
|-----------|----------|---------------|---------|
| Raspberry Pi 4/5 | 1 | 2GB+ RAM | Main controller |
| 5V Relay Module | 2 | With optocoupler (SRD-05VDC-SL-C) | Isolate RPi from buzzer circuit |
| 12V/24V Buzzer | 2 | Industrial-grade, loud | Emergency alarm output |
| Power Supply (RPi) | 1 | 5V 3A USB-C | Power the Raspberry Pi |
| Power Supply (Buzzers) | 1 | 12V or 24V DC (per buzzer spec) | Power the buzzers via relays |
| Jumper Wires | 6+ | Female-to-Female | GPIO to relay connections |
| Ethernet Cable | 1 | Cat5e or Cat6 | Network connectivity |

---

## Raspberry Pi GPIO Pinout (BCM Mode)

```
                    Raspberry Pi GPIO Header
                    ┌────────────────────┐
                    │  3V3  (1) (2)  5V  │
                    │ GPIO2 (3) (4)  5V  │ ◄── 5V Power for Relays
                    │ GPIO3 (5) (6)  GND │ ◄── Common Ground
                    │ GPIO4 (7) (8) TX   │
                    │  GND  (9) (10) RX  │
            ┌──────▶│GPIO17(11)(12)GPIO18│
            │       │GPIO27(13)(14) GND  │ ◄── Relay 2 GND
            │  ┌───▶│GPIO22(15)(16)GPIO23│
            │  │    │  3V3 (17)(18)GPIO24│
            │  │    │GPIO10(19)(20) GND  │
            │  │    │ GPIO9(21)(22)GPIO25│
            │  │    │GPIO11(23)(24)GPIO8 │
            │  │    │  GND (25)(26)GPIO7 │
            │  │    │ GPIO0(27)(28)GPIO1 │
            │  │    │ GPIO5(29)(30) GND  │
            │  │    │ GPIO6(31)(32)GPIO12│
            │  │    │GPIO13(33)(34) GND  │
            │  │    │GPIO19(35)(36)GPIO16│
            │  │    │GPIO26(37)(38)GPIO20│
            │  │    │  GND (39)(40)GPIO21│
            │  │    └────────────────────┘
            │  │
            │  │    Used Pins:
            │  │    ─────────
            │  └─── Pin 13 (GPIO27) → Relay 2 IN
            └────── Pin 11 (GPIO17) → Relay 1 IN
                    Pin 2  or 4     → 5V (Relay VCC)
                    Pin 6  or 14    → GND (Relay GND)
```

---

## Wiring Diagram — Relay Module Connections

```
                     ┌─────────────────────────────────────────┐
                     │            RASPBERRY PI                  │
                     │                                          │
                     │  Pin 11 (GPIO17) ─────────┐             │
                     │  Pin 13 (GPIO27) ────────┐│             │
                     │  Pin 2  (5V)     ──────┐ ││             │
                     │  Pin 6  (GND)    ────┐ │ ││             │
                     └──────────────────────┼─┼─┼┼─────────────┘
                                            │ │ ││
                     ┌──────────────────────┼─┼─┼┼─────────────┐
                     │  RELAY MODULE 1      │ │ ││              │
                     │                      │ │ ││              │
                     │  GND ◄───────────────┘ │ ││              │
                     │  VCC ◄─────────────────┘ ││              │
                     │  IN  ◄───────────────────┘│              │
                     │                           │              │
                     │  ┌─────────────────┐      │              │
                     │  │    RELAY 1       │      │              │
                     │  │                 │      │              │
                     │  │  COM ──── + ────┼──────┼─── Buzzer 1 +│
                     │  │  NO  ──── ← ───┼──┐   │              │
                     │  │  NC           │   │   │              │
                     │  └─────────────────┘  │   │              │
                     └───────────────────────┼───┼──────────────┘
                                             │   │
                     ┌───────────────────────┼───┼──────────────┐
                     │  RELAY MODULE 2       │   │              │
                     │                       │   │              │
                     │  GND ◄── (shared)     │   │              │
                     │  VCC ◄── (shared)     │   │              │
                     │  IN  ◄────────────────┼───┘              │
                     │                       │                  │
                     │  ┌─────────────────┐  │                  │
                     │  │    RELAY 2       │  │                  │
                     │  │                 │  │                  │
                     │  │  COM ──── + ────┼──┼─── Buzzer 2 +   │
                     │  │  NO  ──── ← ───┼──┘                  │
                     │  │  NC           │                      │
                     │  └─────────────────┘                     │
                     └──────────────────────────────────────────┘

                     ┌──────────────────────────────────────────┐
                     │  EXTERNAL POWER SUPPLY                    │
                     │  (12V or 24V DC — per buzzer spec)       │
                     │                                          │
                     │  (+) ──── Relay 1 COM ──── Buzzer 1 (+) │
                     │  (+) ──── Relay 2 COM ──── Buzzer 2 (+) │
                     │  (−) ──── Buzzer 1 (−)                   │
                     │  (−) ──── Buzzer 2 (−)                   │
                     └──────────────────────────────────────────┘
```

---

## Connection Table

| RPi Pin | Function | Connects To |
|---------|----------|-------------|
| Pin 11 (GPIO17) | Relay 1 signal | Relay Module 1 → IN |
| Pin 13 (GPIO27) | Relay 2 signal | Relay Module 2 → IN |
| Pin 2 or 4 (5V) | Relay power | Relay Module 1 & 2 → VCC |
| Pin 6 or 14 (GND) | Common ground | Relay Module 1 & 2 → GND |

| Relay Terminal | Connects To |
|---------------|-------------|
| Relay 1 COM | External power supply (+) |
| Relay 1 NO | Buzzer 1 (+) terminal |
| Relay 2 COM | External power supply (+) |
| Relay 2 NO | Buzzer 2 (+) terminal |

| Buzzer Terminal | Connects To |
|----------------|-------------|
| Buzzer 1 (−) | External power supply (−) |
| Buzzer 2 (−) | External power supply (−) |

---

## Relay Module Detail — SRD-05VDC-SL-C

```
    Relay Module (top view)
    ┌─────────────────────────────┐
    │                             │
    │   [LED]  [Optocoupler]      │
    │                             │
    │   ┌───────────────────┐     │
    │   │      RELAY        │     │
    │   │   SRD-05VDC-SL-C  │     │
    │   └───────────────────┘     │
    │                             │
    │  ┌──┐ ┌──┐ ┌──┐            │  Screw Terminals:
    │  │NO│ │COM│ │NC│            │    NO  = Normally Open
    │  └──┘ └──┘ └──┘            │    COM = Common
    │                             │    NC  = Normally Closed
    │  ┌──┐ ┌──┐ ┌──┐            │
    │  │IN│ │GND│ │VCC│           │  Pin Header:
    │  └──┘ └──┘ └──┘            │    IN  = Signal input (from GPIO)
    │                             │    GND = Ground
    └─────────────────────────────┘    VCC = 5V power
```

### Relay Logic

- **Active-HIGH mode** (default in config):
  - GPIO HIGH → Relay energized → NO closes → Buzzer ON
  - GPIO LOW → Relay de-energized → NO opens → Buzzer OFF

- **Active-LOW mode** (set `active_high: false` in config):
  - GPIO LOW → Relay energized → NO closes → Buzzer ON
  - GPIO HIGH → Relay de-energized → NO opens → Buzzer OFF

> ⚠️ **Check your relay module!** If it has an optocoupler and the relay LED turns ON when IN is LOW, you have an active-LOW module. Set `active_high: false` in `config.yaml`.

---

## Safety Notes

1. **Use relay modules** — never connect buzzers directly to GPIO pins. The RPi GPIO can only source 16mA per pin; relays can switch several amps.

2. **Separate power supplies** — the buzzer power supply should be independent from the RPi power. This prevents buzzer current from causing voltage drops that crash the Pi.

3. **Use NO (Normally Open)** terminals — if the RPi loses power or crashes, the buzzers will be OFF (fail-safe).

4. **Flyback diodes** — most relay modules include built-in flyback diodes. If using bare relays, add a 1N4007 diode across the relay coil.

5. **Industrial buzzers** — use 12V or 24V industrial buzzers rated for continuous operation. Ensure the decibel rating meets building requirements.

6. **Strain relief** — secure all wires with cable ties or conduit to prevent loose connections in an industrial environment.
