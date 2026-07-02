# 🚀 Deployment Guide — Master Alarm System

## Prerequisites

Before starting, ensure you have:

- [ ] Raspberry Pi 4 or 5 with Raspberry Pi OS installed
- [ ] Two relay modules (5V, with optocoupler)
- [ ] Two industrial buzzers (12V or 24V)
- [ ] External power supply for buzzers
- [ ] Ethernet cable connected to the SLT building network
- [ ] InfluxDB API token with read access to `Lift_Alarm_Status` bucket
- [ ] SSH access to the Raspberry Pi

---

## Step 1: Raspberry Pi Initial Setup

### 1.1 Install Raspberry Pi OS

Use the Raspberry Pi Imager to flash **Raspberry Pi OS Lite (64-bit)** — no desktop environment needed for a headless server.

```bash
# Enable SSH during flashing (or create empty 'ssh' file in /boot)
# Set hostname to: slt-master-alarm
# Set username/password: pi / <your-secure-password>
# Configure WiFi if needed (Ethernet preferred)
```

### 1.2 First Boot Configuration

```bash
# Connect via SSH
ssh pi@slt-master-alarm.local

# Update the system
sudo apt update && sudo apt upgrade -y

# Set timezone to Sri Lanka
sudo timedatectl set-timezone Asia/Colombo

# Verify time is correct
date
```

### 1.3 Enable GPIO Access

```bash
# Add 'pi' user to the gpio group
sudo usermod -aG gpio pi

# Verify
groups pi
# Should include: gpio
```

---

## Step 2: Hardware Wiring

Refer to the [Wiring Diagram](wiring_diagram.md) for detailed instructions.

**Quick wiring summary:**

| RPi Pin | → | Destination |
|---------|---|-------------|
| Pin 11 (GPIO17) | → | Relay 1 IN |
| Pin 13 (GPIO27) | → | Relay 2 IN |
| Pin 2 (5V) | → | Relay 1 & 2 VCC |
| Pin 6 (GND) | → | Relay 1 & 2 GND |
| Relay 1 NO + COM | → | Buzzer 1 circuit |
| Relay 2 NO + COM | → | Buzzer 2 circuit |

---

## Step 3: Transfer Project Files

### Option A: Copy from USB drive

```bash
# Mount USB
sudo mount /dev/sda1 /mnt

# Copy the Master_Alarm folder
cp -r /mnt/Master_Alarm /home/pi/Master_Alarm

# Unmount
sudo umount /mnt
```

### Option B: Clone from Git repository

```bash
cd /home/pi
git clone <your-repo-url> SLT_Emergency_Alarm
cd SLT_Emergency_Alarm/Master_Alarm
```

### Option C: SCP from development machine

```bash
# From your development machine:
scp -r Master_Alarm/ pi@slt-master-alarm.local:/home/pi/
```

---

## Step 4: Run the Installer

```bash
cd /home/pi/Master_Alarm
chmod +x scripts/install.sh
sudo ./scripts/install.sh
```

The installer will:
1. ✅ Install system dependencies
2. ✅ Create `/opt/slt-master-alarm/`
3. ✅ Copy project files
4. ✅ Create Python virtual environment
5. ✅ Install pip packages
6. ✅ Create `/var/log/slt-master-alarm/`
7. ✅ Install and enable systemd service
8. ✅ Prompt for InfluxDB token

---

## Step 5: Configure the InfluxDB Token

If you skipped the token during installation:

```bash
sudo nano /etc/systemd/system/slt-master-alarm.service
```

Find this line:
```ini
Environment=INFLUXDB_TOKEN=your-influxdb-api-token-here
```

Replace with your actual token:
```ini
Environment=INFLUXDB_TOKEN=jsEgn9UpR2yTjXaJpSNdwOEow1rnzqMDXPlBuriQaFiq9Mj9X0UIBgu0RN1_...
```

Reload systemd:
```bash
sudo systemctl daemon-reload
```

---

## Step 6: Test Before Going Live

### 6.1 Test GPIO

```bash
sudo /opt/slt-master-alarm/venv/bin/python /opt/slt-master-alarm/scripts/test_gpio.py
```

### 6.2 Test Manual Run

```bash
export INFLUXDB_TOKEN='your-token'
cd /opt/slt-master-alarm
sudo -E venv/bin/python -u src/main.py
```

Verify:
- InfluxDB connection succeeds
- Polling starts
- Triggering alarm=true activates buzzers
- Triggering alarm=false deactivates buzzers
- Ctrl+C cleanly shuts down

See the [Testing Guide](testing_guide.md) for the full test procedure.

---

## Step 7: Start the Service

```bash
# Start the service
sudo systemctl start slt-master-alarm

# Verify it's running
sudo systemctl status slt-master-alarm

# View live logs
journalctl -u slt-master-alarm -f
```

---

## Step 8: Verify Boot Persistence

```bash
# Reboot the Pi
sudo reboot

# After reboot, check the service started automatically
sudo systemctl status slt-master-alarm
```

---

## Service Management Commands

| Command | Description |
|---------|-------------|
| `sudo systemctl start slt-master-alarm` | Start the service |
| `sudo systemctl stop slt-master-alarm` | Stop the service |
| `sudo systemctl restart slt-master-alarm` | Restart the service |
| `sudo systemctl status slt-master-alarm` | Check service status |
| `sudo systemctl enable slt-master-alarm` | Enable auto-start on boot |
| `sudo systemctl disable slt-master-alarm` | Disable auto-start on boot |
| `journalctl -u slt-master-alarm -f` | Follow live logs |
| `journalctl -u slt-master-alarm --since "1 hour ago"` | View recent logs |
| `journalctl -u slt-master-alarm --since today` | View today's logs |

---

## Log File Management

Logs are stored in two locations:

| Location | Type | Managed By |
|----------|------|-----------|
| `/var/log/slt-master-alarm/alarm.log` | Application log | Rotating file handler (10MB × 5 files) |
| `journalctl -u slt-master-alarm` | System journal | systemd journal |

### View application log file:
```bash
tail -f /var/log/slt-master-alarm/alarm.log
```

### Check log file sizes:
```bash
ls -lh /var/log/slt-master-alarm/
```

---

## Configuration Changes

To modify settings after deployment:

```bash
# 1. Edit the config file
sudo nano /opt/slt-master-alarm/config/config.yaml

# 2. Restart the service to apply changes
sudo systemctl restart slt-master-alarm

# 3. Verify the new config is loaded
journalctl -u slt-master-alarm -n 20
```

---

## Updating the Application

```bash
# 1. Stop the service
sudo systemctl stop slt-master-alarm

# 2. Back up the current installation
sudo cp -r /opt/slt-master-alarm /opt/slt-master-alarm.backup

# 3. Copy new source files (preserve config and venv)
sudo cp -r /path/to/new/src/* /opt/slt-master-alarm/src/

# 4. Install any new dependencies
sudo /opt/slt-master-alarm/venv/bin/pip install -r /path/to/new/requirements.txt

# 5. Restart the service
sudo systemctl start slt-master-alarm

# 6. Verify
sudo systemctl status slt-master-alarm
```

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Service won't start | Check `journalctl -u slt-master-alarm -n 50` for errors |
| "INFLUXDB_TOKEN not set" | Set token in `/etc/systemd/system/slt-master-alarm.service` then `sudo systemctl daemon-reload` |
| "Cannot connect to InfluxDB" | Check network: `ping 124.43.179.232` and `curl http://124.43.179.232:8086/health` |
| Buzzers not sounding | Run `test_gpio.py` to verify wiring; check relay VCC power |
| High CPU usage | Check `polling.interval_seconds` in config (should be ≥1) |
| Log file full | Check rotation: `ls -la /var/log/slt-master-alarm/` |
| GPIO permission denied | Add user to gpio group: `sudo usermod -aG gpio pi` |
