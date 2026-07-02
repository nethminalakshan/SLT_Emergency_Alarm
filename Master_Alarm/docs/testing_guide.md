# 🧪 Testing Guide — Master Alarm System

## Pre-Deployment Testing Checklist

Complete all tests below before deploying the system to production.

---

## Test 1: GPIO Hardware Test

**Purpose:** Verify relay modules and buzzers respond to GPIO signals.

**Steps:**
```bash
# 1. Connect to the Raspberry Pi
ssh pi@<rpi-ip-address>

# 2. Navigate to the project directory
cd /opt/slt-master-alarm

# 3. Run the GPIO test script
sudo venv/bin/python scripts/test_gpio.py
```

**Expected Results:**
- [x] Buzzer 1 sounds for 2 seconds, then stops
- [x] Buzzer 2 sounds for 2 seconds, then stops
- [x] Both buzzers sound together for 3 seconds, then stop
- [x] Rapid toggle test completes 5 cycles
- [x] GPIO cleanup message appears

**If a buzzer doesn't sound:**
1. Check the relay module LED — does it light up during the test?
2. If LED lights but no buzzer → check buzzer wiring and external power supply
3. If LED doesn't light → check GPIO wire connections and relay VCC/GND
4. Try changing `ACTIVE_HIGH = False` in the test script

---

## Test 2: InfluxDB Connectivity

**Purpose:** Verify the RPi can reach the InfluxDB server and query data.

**Steps:**
```bash
# 1. Test basic network connectivity
ping -c 3 124.43.179.232

# 2. Test InfluxDB HTTP endpoint
curl -s http://124.43.179.232:8086/health | python3 -m json.tool

# 3. Test API token authentication
curl -s -H "Authorization: Token YOUR_TOKEN_HERE" \
  "http://124.43.179.232:8086/api/v2/buckets" | python3 -m json.tool
```

**Expected Results:**
- [x] Ping succeeds (3 packets received)
- [x] Health endpoint returns `{"status": "pass"}`
- [x] Buckets endpoint returns a list including `Lift_Alarm_Status`

---

## Test 3: Manual Application Run

**Purpose:** Verify the full application works end-to-end before enabling the systemd service.

**Steps:**
```bash
# 1. Set the InfluxDB token
export INFLUXDB_TOKEN='your-actual-token-here'

# 2. Run the application manually
cd /opt/slt-master-alarm
sudo -E venv/bin/python -u src/main.py
```

**Expected Console Output:**
```
============================================================
  SLT LIFT MASTER ALARM SYSTEM
  Sri Lanka Telecom — Building Emergency Monitoring
============================================================
Loading configuration from: /opt/slt-master-alarm/config/config.yaml
2026-07-02 12:00:00 — INFO     — slt_master_alarm — Configuration loaded successfully.
2026-07-02 12:00:00 — INFO     — slt_master_alarm — ✅ GPIO initialized — ...
2026-07-02 12:00:00 — INFO     — slt_master_alarm — Connecting to InfluxDB...
2026-07-02 12:00:01 — INFO     — slt_master_alarm — ✅ Connected to InfluxDB successfully.
```

**Verify alarm control:**
1. Manually write `alarm=true` to InfluxDB (see Test 4 below)
2. Observe both buzzers turn ON
3. Write `alarm=false` → buzzers turn OFF
4. Press `Ctrl+C` → clean shutdown message appears

---

## Test 4: Manual InfluxDB Data Write

**Purpose:** Write test alarm values to trigger the buzzer system.

**Activate alarm (set `alarm=true`):**
```bash
curl -s -X POST "http://124.43.179.232:8086/api/v2/write?org=SLT&bucket=Lift_Alarm_Status&precision=s" \
  -H "Authorization: Token YOUR_TOKEN_HERE" \
  -H "Content-Type: text/plain" \
  -d "master_alarm alarm=true"
```

**Deactivate alarm (set `alarm=false`):**
```bash
curl -s -X POST "http://124.43.179.232:8086/api/v2/write?org=SLT&bucket=Lift_Alarm_Status&precision=s" \
  -H "Authorization: Token YOUR_TOKEN_HERE" \
  -H "Content-Type: text/plain" \
  -d "master_alarm alarm=false"
```

**Verify data in InfluxDB UI:**
```flux
from(bucket: "Lift_Alarm_Status")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "master_alarm")
  |> filter(fn: (r) => r._field == "alarm")
  |> last()
```

---

## Test 5: Connection Loss & Recovery

**Purpose:** Verify the system handles network outages without crashing.

**Steps:**
1. Start the application (`python -u src/main.py`)
2. Confirm it's connected and polling
3. Disconnect the Ethernet cable from the RPi
4. Observe log output — should show connection errors and retry messages
5. Wait 30 seconds — the application should NOT crash
6. Reconnect the Ethernet cable
7. Observe log output — should show "Connected to InfluxDB successfully"
8. Verify alarm polling resumes

**Expected Log Pattern:**
```
❌ InfluxDB connection failed (attempt 1): ...
Retrying in 5 seconds...
❌ InfluxDB connection failed (attempt 2): ...
Retrying in 10 seconds...
...
✅ Connected to InfluxDB successfully.
```

---

## Test 6: systemd Service

**Purpose:** Verify the service starts on boot and manages the application lifecycle.

**Steps:**
```bash
# 1. Start the service
sudo systemctl start slt-master-alarm

# 2. Check status
sudo systemctl status slt-master-alarm

# 3. View live logs
journalctl -u slt-master-alarm -f

# 4. Test restart
sudo systemctl restart slt-master-alarm

# 5. Test auto-restart on crash (kill the process)
sudo kill -9 $(pgrep -f "src/main.py")
# Wait 10 seconds...
sudo systemctl status slt-master-alarm
# Should show "active (running)" again

# 6. Test boot persistence
sudo reboot
# After reboot:
sudo systemctl status slt-master-alarm
# Should show "active (running)"
```

---

## Test 7: Long-Running Stability (Soak Test)

**Purpose:** Verify 24/7 reliability under sustained operation.

**Steps:**
1. Start the service: `sudo systemctl start slt-master-alarm`
2. Let it run for **24 hours minimum**
3. Periodically trigger alarm state changes (true/false)
4. Check logs for any errors: `journalctl -u slt-master-alarm --since "24 hours ago" | grep -i error`
5. Check memory usage: `ps aux | grep main.py` — should be stable (not growing)
6. Check CPU usage: `top -p $(pgrep -f main.py)` — should be near 0%

**Acceptance Criteria:**
- [ ] No crashes in 24 hours
- [ ] Memory usage stable (no leak)
- [ ] CPU usage < 1%
- [ ] All alarm state changes correctly reflected on buzzers
- [ ] Log file rotation working (check `/var/log/slt-master-alarm/`)

---

## Final Sign-Off

| Test | Result | Date | Tester |
|------|--------|------|--------|
| GPIO Hardware | ☐ Pass / ☐ Fail | | |
| InfluxDB Connectivity | ☐ Pass / ☐ Fail | | |
| Manual Application Run | ☐ Pass / ☐ Fail | | |
| Manual Data Write | ☐ Pass / ☐ Fail | | |
| Connection Loss & Recovery | ☐ Pass / ☐ Fail | | |
| systemd Service | ☐ Pass / ☐ Fail | | |
| 24-Hour Soak Test | ☐ Pass / ☐ Fail | | |
