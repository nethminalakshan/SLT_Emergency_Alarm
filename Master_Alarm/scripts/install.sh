#!/bin/bash
# =============================================================================
# SLT Lift Master Alarm System — Automated Installation Script
# =============================================================================
# This script sets up the Raspberry Pi for running the master alarm system.
#
# Usage:
#   chmod +x scripts/install.sh
#   sudo ./scripts/install.sh
#
# What it does:
#   1. Installs system dependencies
#   2. Creates the installation directory (/opt/slt-master-alarm)
#   3. Copies project files
#   4. Creates a Python virtual environment
#   5. Installs Python packages
#   6. Creates the log directory
#   7. Installs and enables the systemd service
#   8. Prompts for the InfluxDB API token
# =============================================================================

set -e  # Exit on any error

# --- Colors for terminal output ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'  # No Color

# --- Configuration ---
INSTALL_DIR="/opt/slt-master-alarm"
LOG_DIR="/var/log/slt-master-alarm"
SERVICE_NAME="slt-master-alarm"
SERVICE_FILE="/etc/systemd/system/${SERVICE_NAME}.service"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# =============================================================================
echo -e "${CYAN}"
echo "============================================================"
echo "  SLT LIFT MASTER ALARM SYSTEM — INSTALLER"
echo "  Sri Lanka Telecom — Building Emergency Monitoring"
echo "============================================================"
echo -e "${NC}"

# --- Check if running as root ---
if [ "$EUID" -ne 0 ]; then
    echo -e "${RED}ERROR: This script must be run as root (sudo).${NC}"
    echo "Usage: sudo ./scripts/install.sh"
    exit 1
fi

# =============================================================================
# Step 1: Install System Dependencies
# =============================================================================
echo -e "${YELLOW}[1/8] Installing system dependencies...${NC}"
apt-get update -qq
apt-get install -y -qq \
    python3 \
    python3-venv \
    python3-pip \
    python3-rpi.gpio \
    python3-dev \
    > /dev/null 2>&1

echo -e "${GREEN}  ✅ System dependencies installed.${NC}"

# =============================================================================
# Step 2: Create Installation Directory
# =============================================================================
echo -e "${YELLOW}[2/8] Creating installation directory: ${INSTALL_DIR}${NC}"

if [ -d "$INSTALL_DIR" ]; then
    echo -e "${YELLOW}  ⚠️  Directory exists. Backing up existing installation...${NC}"
    BACKUP_DIR="${INSTALL_DIR}.backup.$(date +%Y%m%d%H%M%S)"
    mv "$INSTALL_DIR" "$BACKUP_DIR"
    echo -e "${GREEN}  ✅ Backup saved to: ${BACKUP_DIR}${NC}"
fi

mkdir -p "$INSTALL_DIR"
echo -e "${GREEN}  ✅ Installation directory created.${NC}"

# =============================================================================
# Step 3: Copy Project Files
# =============================================================================
echo -e "${YELLOW}[3/8] Copying project files...${NC}"
cp -r "$PROJECT_DIR/src" "$INSTALL_DIR/"
cp -r "$PROJECT_DIR/config" "$INSTALL_DIR/"
cp -r "$PROJECT_DIR/scripts" "$INSTALL_DIR/"
cp "$PROJECT_DIR/requirements.txt" "$INSTALL_DIR/"

# Set ownership
chown -R pi:pi "$INSTALL_DIR"
echo -e "${GREEN}  ✅ Project files copied to ${INSTALL_DIR}${NC}"

# =============================================================================
# Step 4: Create Python Virtual Environment
# =============================================================================
echo -e "${YELLOW}[4/8] Creating Python virtual environment...${NC}"
python3 -m venv "$INSTALL_DIR/venv" --system-site-packages
echo -e "${GREEN}  ✅ Virtual environment created.${NC}"

# =============================================================================
# Step 5: Install Python Packages
# =============================================================================
echo -e "${YELLOW}[5/8] Installing Python packages...${NC}"
"$INSTALL_DIR/venv/bin/pip" install --upgrade pip -q
"$INSTALL_DIR/venv/bin/pip" install -r "$INSTALL_DIR/requirements.txt" -q
echo -e "${GREEN}  ✅ Python packages installed.${NC}"

# Show installed packages
echo -e "  Installed packages:"
"$INSTALL_DIR/venv/bin/pip" list --format=columns 2>/dev/null | grep -E "influxdb|PyYAML|RPi" || true

# =============================================================================
# Step 6: Create Log Directory
# =============================================================================
echo -e "${YELLOW}[6/8] Creating log directory: ${LOG_DIR}${NC}"
mkdir -p "$LOG_DIR"
chown pi:pi "$LOG_DIR"
chmod 755 "$LOG_DIR"
echo -e "${GREEN}  ✅ Log directory created.${NC}"

# =============================================================================
# Step 7: Install systemd Service
# =============================================================================
echo -e "${YELLOW}[7/8] Installing systemd service...${NC}"
cp "$PROJECT_DIR/service/${SERVICE_NAME}.service" "$SERVICE_FILE"

# --- Prompt for InfluxDB Token ---
echo ""
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${YELLOW}  InfluxDB API Token Required${NC}"
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
read -rp "  Enter your InfluxDB API token (or press Enter to skip): " INFLUX_TOKEN

if [ -n "$INFLUX_TOKEN" ]; then
    # Replace placeholder token in service file
    sed -i "s|Environment=INFLUXDB_TOKEN=your-influxdb-api-token-here|Environment=INFLUXDB_TOKEN=${INFLUX_TOKEN}|" "$SERVICE_FILE"
    echo -e "${GREEN}  ✅ Token configured in service file.${NC}"
else
    echo -e "${YELLOW}  ⚠️  Token skipped. You MUST set it manually before starting the service:${NC}"
    echo -e "     sudo nano ${SERVICE_FILE}"
    echo -e "     Find: Environment=INFLUXDB_TOKEN=your-influxdb-api-token-here"
    echo -e "     Replace with your actual token."
fi

# Reload and enable the service
systemctl daemon-reload
systemctl enable "$SERVICE_NAME"
echo -e "${GREEN}  ✅ Service installed and enabled (will start on boot).${NC}"

# =============================================================================
# Step 8: Summary
# =============================================================================
echo ""
echo -e "${CYAN}============================================================${NC}"
echo -e "${GREEN}  ✅ INSTALLATION COMPLETE${NC}"
echo -e "${CYAN}============================================================${NC}"
echo ""
echo -e "  ${CYAN}Installation directory:${NC}  ${INSTALL_DIR}"
echo -e "  ${CYAN}Log directory:${NC}          ${LOG_DIR}"
echo -e "  ${CYAN}Service file:${NC}           ${SERVICE_FILE}"
echo -e "  ${CYAN}Config file:${NC}            ${INSTALL_DIR}/config/config.yaml"
echo ""
echo -e "  ${YELLOW}Next Steps:${NC}"
echo ""
if [ -z "$INFLUX_TOKEN" ]; then
    echo -e "  1. Set the InfluxDB token in the service file:"
    echo -e "     ${CYAN}sudo nano ${SERVICE_FILE}${NC}"
    echo ""
fi
echo -e "  2. Test the GPIO hardware:"
echo -e "     ${CYAN}sudo ${INSTALL_DIR}/venv/bin/python ${INSTALL_DIR}/scripts/test_gpio.py${NC}"
echo ""
echo -e "  3. Test the application manually:"
echo -e "     ${CYAN}export INFLUXDB_TOKEN='your-token'${NC}"
echo -e "     ${CYAN}cd ${INSTALL_DIR} && sudo venv/bin/python -u src/main.py${NC}"
echo ""
echo -e "  4. Start the service:"
echo -e "     ${CYAN}sudo systemctl start ${SERVICE_NAME}${NC}"
echo ""
echo -e "  5. Check service status:"
echo -e "     ${CYAN}sudo systemctl status ${SERVICE_NAME}${NC}"
echo ""
echo -e "  6. View live logs:"
echo -e "     ${CYAN}journalctl -u ${SERVICE_NAME} -f${NC}"
echo ""
echo -e "${CYAN}============================================================${NC}"
