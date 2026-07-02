# 🏗 System Architecture — Master Alarm

## Overview

The Master Alarm system is an extension of the existing SLT Lift Emergency Alarm V1.3 architecture. It adds a Raspberry Pi that reads a **single master alarm boolean** from InfluxDB and drives **two buzzers** located in different parts of the building.

---

## End-to-End Data Flow

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         EXISTING SYSTEM (V1.3)                         │
│                                                                         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐                 │
│  │ UNO1 (Lotus) │  │ UNO2 (Duke)  │  │ UNO3 (OTS)   │                 │
│  │ Lift 1 & 2   │  │ Lift 3 & 4   │  │ Lift 5 & 6   │                 │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘                 │
│         │                 │                 │                           │
│         └─────── HTTP POST ─┼─── HTTP POST ──┘                         │
│                             │                                           │
│                    ┌────────▼────────┐                                  │
│                    │   InfluxDB v2    │                                  │
│                    │ 124.43.179.232   │                                  │
│                    │                  │                                  │
│                    │ Bucket:          │                                  │
│                    │ Lift_Emergency_  │  ← Individual lift states       │
│                    │ Alarm            │    (6 lifts, state 0/1)         │
│                    │                  │                                  │
│                    │ Bucket:          │                                  │
│                    │ Lift_Alarm_      │  ← Master alarm boolean         │
│                    │ Status           │    (true/false)                  │
│                    └────────┬────────┘                                  │
│                             │                                           │
└─────────────────────────────┼───────────────────────────────────────────┘
                              │
              ┌───────────────┼───────────────┐
              │               │               │
              │    Software Team analyzes     │
              │    all 6 lifts and writes     │
              │    master_alarm boolean       │
              │                               │
              └───────────────┼───────────────┘
                              │
                    ┌─────────▼─────────┐
                    │                    │
                    │  NEW: Raspberry Pi │
                    │  Master Alarm      │
                    │  Monitor           │
                    │                    │
                    │  Reads:            │
                    │  master_alarm →    │
                    │  true/false        │
                    │                    │
                    │  Controls:         │
                    │  GPIO17 → Relay 1  │
                    │  GPIO27 → Relay 2  │
                    │                    │
                    └───┬──────────┬─────┘
                        │          │
               ┌────────▼──┐  ┌───▼────────┐
               │  Buzzer 1  │  │  Buzzer 2  │
               │ (Location  │  │ (Location  │
               │    A)      │  │    B)      │
               └────────────┘  └────────────┘
```

---

## Component Interaction Diagram

```
┌──────────────────────────────────────────────────────────┐
│                    Raspberry Pi                           │
│                                                          │
│  ┌─────────┐    ┌──────────────┐    ┌───────────────┐   │
│  │  main.py │───▶│ config_      │    │ logger_       │   │
│  │  (entry) │    │ loader.py    │    │ setup.py      │   │
│  └────┬─────┘    └──────────────┘    └───────────────┘   │
│       │                                                   │
│       ▼                                                   │
│  ┌──────────────────────────────────────┐                │
│  │           alarm_monitor.py            │                │
│  │                                       │                │
│  │  ┌─ Poll Loop (every 1s) ──────────┐ │                │
│  │  │                                  │ │                │
│  │  │  1. Query InfluxDB               │ │                │
│  │  │  2. Compare with previous state  │ │                │
│  │  │  3. If changed → update GPIO     │ │                │
│  │  │  4. Sleep                        │ │                │
│  │  │                                  │ │                │
│  │  └──────────────────────────────────┘ │                │
│  │                                       │                │
│  │  Uses:                                │                │
│  │  ┌─────────────────┐  ┌────────────┐ │                │
│  │  │ influxdb_client_ │  │ gpio_      │ │                │
│  │  │ wrapper.py       │  │ controller │ │                │
│  │  └────────┬─────────┘  │ .py        │ │                │
│  │           │             └─────┬──────┘ │                │
│  └───────────┼───────────────────┼────────┘                │
│              │                   │                          │
└──────────────┼───────────────────┼──────────────────────────┘
               │                   │
     ┌─────────▼──────┐    ┌──────▼──────────┐
     │   InfluxDB v2   │    │   Relay Modules  │
     │   (Remote)      │    │   GPIO17, GPIO27 │
     └─────────────────┘    └─────────────────┘
```

---

## State Machine

```
                    ┌──────────────┐
                    │   STARTUP    │
                    └──────┬───────┘
                           │
                   Connect to InfluxDB
                   (with retries)
                           │
                    ┌──────▼───────┐
              ┌────▶│   POLLING    │◀────┐
              │     └──────┬───────┘     │
              │            │             │
              │     Query InfluxDB       │
              │            │             │
              │    ┌───────▼────────┐    │
              │    │ alarm == true? │    │
              │    └───┬───────┬────┘    │
              │        │       │         │
              │    YES │       │ NO      │
              │        │       │         │
              │   ┌────▼──┐ ┌─▼──────┐  │
              │   │GPIO ON│ │GPIO OFF│  │
              │   └───┬───┘ └───┬────┘  │
              │       │         │        │
              │       └────┬────┘        │
              │            │             │
              │     Sleep 1 second       │
              │            │             │
              └────────────┘             │
                                         │
              Connection Lost?           │
              ┌────────────┐             │
              │ RECONNECT  │─── Success ─┘
              │ (backoff)  │
              └────────────┘
```

---

## InfluxDB Buckets

| Bucket | Written By | Read By | Data |
|--------|-----------|---------|------|
| `Lift_Emergency_Alarm` | Arduino UNO nodes | Software team dashboards | Individual lift states (6 lifts) |
| `Lift_Alarm_Status` | Software team (master alarm task) | Raspberry Pi | Single master alarm boolean |

---

## Network Topology

| Device | IP Address | Role |
|--------|-----------|------|
| UNO1 (Lotus) | `192.168.1.101` | Lift 1 & 2 monitoring |
| UNO2 (Duke) | `192.168.1.102` | Lift 3 & 4 monitoring |
| UNO3 (OTS) | `192.168.1.103` | Lift 5 & 6 monitoring |
| InfluxDB Server | `124.43.179.232:8086` | Time-series database |
| Raspberry Pi | DHCP or static (your choice) | Master alarm buzzer controller |
