#!/bin/bash
# SLT Master Alarm — Simple Installer
set -e

INSTALL_DIR="/opt/slt-master-alarm"
SERVICE="slt-master-alarm"

echo "========================================"
echo "  SLT Master Alarm — Installer"
echo "========================================"

# Must be root
if [ "$EUID" -ne 0 ]; then
    echo "ERROR: Run with sudo"; exit 1
fi

# 1. System deps
echo "[1/5] Installing system packages..."
apt-get update -qq
apt-get install -y -qq python3 python3-venv python3-rpi.gpio > /dev/null 2>&1
echo "  ✅ Done"

# 2. Copy files
echo "[2/5] Copying files to $INSTALL_DIR..."
mkdir -p "$INSTALL_DIR"
cp "$(dirname "$0")/master_alarm.py" "$INSTALL_DIR/"
chown -R pi:pi "$INSTALL_DIR"
echo "  ✅ Done"

# 3. Python venv + deps
echo "[3/5] Setting up Python environment..."
python3 -m venv "$INSTALL_DIR/venv" --system-site-packages
"$INSTALL_DIR/venv/bin/pip" install --upgrade pip -q
"$INSTALL_DIR/venv/bin/pip" install "influxdb-client>=1.36.0" -q
echo "  ✅ Done"

# 4. Systemd service
echo "[4/5] Installing systemd service..."
cp "$(dirname "$0")/slt-master-alarm.service" "/etc/systemd/system/${SERVICE}.service"

read -rp "  Enter InfluxDB API token (or Enter to skip): " TOKEN
if [ -n "$TOKEN" ]; then
    sed -i "s|Environment=INFLUXDB_TOKEN=your-token-here|Environment=INFLUXDB_TOKEN=${TOKEN}|" "/etc/systemd/system/${SERVICE}.service"
    echo "  ✅ Token set"
else
    echo "  ⚠️  Set token later: sudo nano /etc/systemd/system/${SERVICE}.service"
fi

systemctl daemon-reload
systemctl enable "$SERVICE"
echo "  ✅ Service enabled"

# 5. Done
echo ""
echo "[5/5] ✅ INSTALL COMPLETE"
echo ""
echo "  Start:  sudo systemctl start $SERVICE"
echo "  Status: sudo systemctl status $SERVICE"
echo "  Logs:   journalctl -u $SERVICE -f"
echo ""
