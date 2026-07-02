# 🔮 Future Improvements — Master Alarm System

## Phase 2 Enhancements

### 1. Email Alert Notifications

Send email alerts to the building management team when the master alarm changes state.

**Implementation:**
- Add `smtplib` integration to `alarm_monitor.py`
- Configure SMTP server, recipients, and templates in `config.yaml`
- Send alerts on alarm activation and deactivation
- Include timestamp, alarm duration, and source information

**Config addition:**
```yaml
notifications:
  email:
    enabled: true
    smtp_server: "smtp.sltmobitel.lk"
    smtp_port: 587
    sender: "alarm@slt.lk"
    recipients:
      - "building-ops@slt.lk"
      - "security@slt.lk"
    subject_prefix: "[SLT ALARM]"
```

---

### 2. SMS Alert Notifications

Send SMS alerts for critical alarm events via an SMS gateway API.

**Implementation:**
- Integrate with Dialog/Mobitel SMS API or Twilio
- Rate-limit SMS to prevent flooding (max 1 SMS per alarm event)
- Include cooldown period between repeated alarms

---

### 3. Web Dashboard

A lightweight web dashboard running on the Raspberry Pi showing:
- Current alarm status (active/inactive)
- Alarm history (last 24 hours)
- InfluxDB connection status
- Buzzer status (GPIO state)
- System health (uptime, CPU, memory)

**Implementation:**
- Use Flask or FastAPI
- Real-time updates via WebSocket or Server-Sent Events
- Dark-mode industrial UI (matching the V1.2 dashboard style)
- Accessible on the local network: `http://<rpi-ip>:8080`

---

### 4. Hardware Watchdog Timer

Enable the Raspberry Pi's built-in hardware watchdog to automatically reboot the system if the application hangs or the OS freezes.

**Implementation:**
```bash
# Enable hardware watchdog
sudo apt install watchdog
sudo systemctl enable watchdog

# Configure /etc/watchdog.conf
max-load-1 = 24
watchdog-device = /dev/watchdog
watchdog-timeout = 15
```

Add watchdog petting to `alarm_monitor.py` — if the monitoring loop stops, the watchdog triggers a reboot.

---

### 5. UPS (Uninterruptible Power Supply) Integration

Ensure the Raspberry Pi stays running during power outages.

**Options:**
- **PiJuice** — RPi HAT with battery backup
- **External UPS** — standard 12V UPS powering both RPi and buzzers
- **USB power bank** — simple but limited capacity

**Software integration:**
- Monitor UPS status (battery level, charging state)
- Log power events
- Graceful shutdown on low battery

---

### 6. Redundant Raspberry Pi (High Availability)

Deploy a second Raspberry Pi as a hot standby:
- Both RPis poll InfluxDB independently
- Primary RPi controls the buzzers
- If the primary goes offline, the secondary takes over
- Heartbeat monitoring between the two RPis

---

### 7. Alarm Escalation

Implement a tiered alarm escalation strategy:

| Time Since Alarm | Action |
|-----------------|--------|
| 0–5 minutes | Buzzers ON |
| 5–10 minutes | Buzzers ON + Email to building ops |
| 10–15 minutes | Buzzers ON + SMS to security team |
| 15+ minutes | Buzzers ON + Phone call to on-call engineer |

---

### 8. Prometheus/Grafana Monitoring

Export application metrics to Prometheus for centralized monitoring:
- Alarm state (gauge)
- InfluxDB query latency (histogram)
- Connection failures (counter)
- GPIO state (gauge)

Visualize with Grafana dashboards alongside the existing InfluxDB data.

---

### 9. InfluxDB Flux Task for Master Alarm

If the software team hasn't created the master alarm aggregation yet, it can be implemented as an InfluxDB Flux Task:

```flux
option task = {
    name: "Generate Master Alarm",
    every: 5s
}

// Check if ANY lift has alarm state = 1
alarm_active = from(bucket: "Lift_Emergency_Alarm")
    |> range(start: -1m)
    |> filter(fn: (r) => r._measurement == "lift_status")
    |> filter(fn: (r) => r._field == "state")
    |> last()
    |> filter(fn: (r) => r._value == 1)
    |> count()
    |> map(fn: (r) => ({r with _value: r._value > 0}))

// Write the master alarm boolean
alarm_active
    |> set(key: "_measurement", value: "master_alarm")
    |> set(key: "_field", value: "alarm")
    |> to(bucket: "Lift_Alarm_Status", org: "SLT")
```

---

### 10. Remote Configuration Management

Allow configuration changes without SSH access:
- Store config in InfluxDB or a central config server
- RPi periodically checks for config updates
- Apply changes without restarting the service

---

## Priority Matrix

| Improvement | Impact | Effort | Priority |
|-------------|--------|--------|----------|
| Hardware Watchdog | 🟢 High | 🟢 Low | ⭐ P1 |
| Email Alerts | 🟢 High | 🟡 Medium | ⭐ P1 |
| UPS Integration | 🟢 High | 🟡 Medium | ⭐ P1 |
| Alarm Escalation | 🟢 High | 🟡 Medium | ⭐ P2 |
| Web Dashboard | 🟡 Medium | 🟡 Medium | ⭐ P2 |
| SMS Alerts | 🟡 Medium | 🟡 Medium | ⭐ P2 |
| Flux Task (Master Alarm) | 🟢 High | 🟢 Low | ⭐ P1 |
| Redundant RPi | 🟡 Medium | 🔴 High | ⭐ P3 |
| Prometheus/Grafana | 🟢 Low | 🟡 Medium | ⭐ P3 |
| Remote Config | 🟢 Low | 🔴 High | ⭐ P4 |
