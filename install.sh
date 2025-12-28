#!/bin/bash
#
# P25 Hotspot - Automated Installation Script
# For radxrf.com P25 Trunking Network
#
# Usage:
#   curl -sSL https://raw.githubusercontent.com/YOUR_USERNAME/p25-hotspot/main/install.sh | sudo bash
#

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Installation directory
INSTALL_DIR="/opt/p25-hotspot"
CONFIG_FILE="/etc/p25-hotspot.yaml"

# Print colored messages
print_info() {
    echo -e "${CYAN}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Banner
clear
echo -e "${GREEN}"
echo "============================================================"
echo "  P25 Hotspot Installation Script"
echo "  radxrf.com P25 Trunking Network"
echo "============================================================"
echo -e "${NC}"

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    print_error "Please run as root (use sudo)"
    exit 1
fi

# Detect OS
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
    VER=$VERSION_ID
else
    print_error "Cannot detect OS"
    exit 1
fi

print_info "Detected OS: $OS $VER"

# Check if Raspberry Pi
if ! grep -q "Raspberry Pi" /proc/cpuinfo 2>/dev/null && ! grep -q "BCM" /proc/cpuinfo 2>/dev/null; then
    print_warning "This doesn't appear to be a Raspberry Pi"
    read -p "Continue anyway? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Update system
print_info "Updating system packages..."
apt update
apt upgrade -y

# Install dependencies
print_info "Installing build dependencies..."
apt install -y \
    build-essential \
    cmake \
    git \
    libyaml-cpp-dev \
    python3 \
    python3-pip \
    python3-venv

print_success "Dependencies installed"

# Clone or update repository
if [ -d "$INSTALL_DIR" ]; then
    print_warning "Installation directory already exists"
    read -p "Remove and reinstall? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        print_info "Removing old installation..."
        systemctl stop p25-hotspot 2>/dev/null || true
        systemctl stop p25-hotspot-web 2>/dev/null || true
        rm -rf "$INSTALL_DIR"
    else
        print_info "Updating existing installation..."
        cd "$INSTALL_DIR"
        git pull
    fi
fi

if [ ! -d "$INSTALL_DIR" ]; then
    print_info "Cloning repository..."
    git clone https://github.com/YOUR_USERNAME/p25-hotspot.git "$INSTALL_DIR"
fi

cd "$INSTALL_DIR"

# Build C++ hotspot
print_info "Building P25 hotspot software..."
mkdir -p build
cd build
cmake ..
make -j$(nproc)

if [ ! -f "p25-hotspot" ]; then
    print_error "Build failed - binary not created"
    exit 1
fi

print_success "Build successful"

# Install binary
print_info "Installing binary..."
cp p25-hotspot /usr/local/bin/
chmod +x /usr/local/bin/p25-hotspot

# Install systemd service
print_info "Installing systemd service..."
cp "$INSTALL_DIR/p25-hotspot.service" /etc/systemd/system/
systemctl daemon-reload

# Create default config if doesn't exist
if [ ! -f "$CONFIG_FILE" ]; then
    print_info "Creating default configuration..."
    cp "$INSTALL_DIR/config.example.yaml" "$CONFIG_FILE"
    print_warning "Configuration file created at $CONFIG_FILE"
    print_warning "YOU MUST EDIT THIS FILE before starting the service!"
else
    print_info "Existing configuration preserved at $CONFIG_FILE"
fi

# Install web interface
print_info "Installing web interface..."
cd "$INSTALL_DIR/web"

pip3 install -r requirements.txt

# Install web service
cp p25-hotspot-web.service /etc/systemd/system/
systemctl daemon-reload

print_success "Web interface installed"

# Enable GPIO UART if on Raspberry Pi
if grep -q "Raspberry Pi" /proc/cpuinfo 2>/dev/null; then
    print_info "Checking UART configuration..."

    if ! grep -q "^enable_uart=1" /boot/config.txt 2>/dev/null && \
       ! grep -q "^enable_uart=1" /boot/firmware/config.txt 2>/dev/null; then

        print_warning "GPIO UART is not enabled"
        read -p "Enable UART for GPIO modem? (y/N): " -n 1 -r
        echo

        if [[ $REPLY =~ ^[Yy]$ ]]; then
            # Try both possible locations
            if [ -f /boot/config.txt ]; then
                echo "enable_uart=1" >> /boot/config.txt
            elif [ -f /boot/firmware/config.txt ]; then
                echo "enable_uart=1" >> /boot/firmware/config.txt
            fi
            print_success "UART enabled - reboot required"
        fi
    else
        print_success "UART already enabled"
    fi
fi

# Configuration wizard
echo ""
echo -e "${GREEN}============================================================${NC}"
echo -e "${GREEN}  Installation Complete!${NC}"
echo -e "${GREEN}============================================================${NC}"
echo ""

print_warning "IMPORTANT: You must configure the hotspot before it will work!"
echo ""

read -p "Run configuration wizard now? (Y/n): " -n 1 -r
echo

if [[ ! $REPLY =~ ^[Nn]$ ]]; then
    # Simple interactive configuration
    echo ""
    echo -e "${CYAN}--- Reflector Configuration ---${NC}"

    read -p "Reflector IP address [199.201.221.8]: " REFLECTOR_IP
    REFLECTOR_IP=${REFLECTOR_IP:-199.201.221.8}

    read -p "Reflector port [41000]: " REFLECTOR_PORT
    REFLECTOR_PORT=${REFLECTOR_PORT:-41000}

    read -p "Your Radio ID: " RADIO_ID
    while [ -z "$RADIO_ID" ]; do
        echo "Radio ID is required!"
        read -p "Your Radio ID: " RADIO_ID
    done

    read -p "Your Callsign: " CALLSIGN
    while [ -z "$CALLSIGN" ]; do
        echo "Callsign is required!"
        read -p "Your Callsign: " CALLSIGN
    done
    CALLSIGN=$(echo "$CALLSIGN" | tr '[:lower:]' '[:upper:]')

    read -sp "Reflector Password: " PASSWORD
    echo
    while [ -z "$PASSWORD" ]; do
        echo "Password is required!"
        read -sp "Reflector Password: " PASSWORD
        echo
    done

    echo ""
    echo -e "${CYAN}--- Modem Configuration ---${NC}"

    echo "Select modem type:"
    echo "1) GPIO UART (/dev/ttyAMA0) - For MMDVM_HS, ZUMspot on GPIO"
    echo "2) USB (/dev/ttyUSB0) - For USB modems"
    echo "3) Custom"
    read -p "Choice [1]: " MODEM_CHOICE
    MODEM_CHOICE=${MODEM_CHOICE:-1}

    case $MODEM_CHOICE in
        1)
            MODEM_PORT="/dev/ttyAMA0"
            ;;
        2)
            MODEM_PORT="/dev/ttyUSB0"
            ;;
        3)
            read -p "Serial port: " MODEM_PORT
            ;;
        *)
            MODEM_PORT="/dev/ttyAMA0"
            ;;
    esac

    read -p "RX Frequency (MHz) [449.000]: " RX_FREQ_MHZ
    RX_FREQ_MHZ=${RX_FREQ_MHZ:-449.000}
    RX_FREQ_HZ=$(echo "$RX_FREQ_MHZ * 1000000" | bc | cut -d. -f1)

    read -p "TX Frequency (MHz) [444.000]: " TX_FREQ_MHZ
    TX_FREQ_MHZ=${TX_FREQ_MHZ:-444.000}
    TX_FREQ_HZ=$(echo "$TX_FREQ_MHZ * 1000000" | bc | cut -d. -f1)

    # Write configuration
    cat > "$CONFIG_FILE" << EOF
# P25 Hotspot Configuration
# Generated by install script

reflector:
  address: "$REFLECTOR_IP"
  port: $REFLECTOR_PORT
  radio_id: $RADIO_ID
  password: "$PASSWORD"
  callsign: "$CALLSIGN"
  keepalive_interval: 5

modem:
  port: "$MODEM_PORT"
  baud: 115200
  rx_frequency: $RX_FREQ_HZ
  tx_frequency: $TX_FREQ_HZ
  tx_power: 50
  rx_offset: 0
  tx_offset: 0
  rf_level: 100
  rx_dc_offset: 0
  tx_dc_offset: 0

p25:
  nac: 0x293
  enabled: true
  trunking: true

logging:
  level: "INFO"
  file: "/var/log/p25-hotspot.log"
  console: true
  max_size_mb: 10
  max_files: 5
EOF

    chmod 600 "$CONFIG_FILE"

    print_success "Configuration saved to $CONFIG_FILE"
fi

# Enable and start services
echo ""
read -p "Enable and start services now? (Y/n): " -n 1 -r
echo

if [[ ! $REPLY =~ ^[Nn]$ ]]; then
    print_info "Enabling services..."
    systemctl enable p25-hotspot
    systemctl enable p25-hotspot-web

    print_info "Starting services..."
    systemctl start p25-hotspot
    systemctl start p25-hotspot-web

    sleep 2

    # Check status
    if systemctl is-active --quiet p25-hotspot; then
        print_success "P25 Hotspot service is running"
    else
        print_error "P25 Hotspot service failed to start"
        print_info "Check logs: journalctl -u p25-hotspot -n 50"
    fi

    if systemctl is-active --quiet p25-hotspot-web; then
        print_success "Web interface is running"
    else
        print_warning "Web interface failed to start (not critical)"
    fi
fi

# Final instructions
echo ""
echo -e "${GREEN}============================================================${NC}"
echo -e "${GREEN}  Installation Complete!${NC}"
echo -e "${GREEN}============================================================${NC}"
echo ""
echo "Next steps:"
echo ""
echo "1. Web Interface:"
echo "   http://$(hostname -I | awk '{print $1}'):8080"
echo "   or"
echo "   http://$(hostname).local:8080"
echo ""
echo "2. Check Status:"
echo "   sudo systemctl status p25-hotspot"
echo "   sudo systemctl status p25-hotspot-web"
echo ""
echo "3. View Logs:"
echo "   sudo journalctl -u p25-hotspot -f"
echo ""
echo "4. Edit Configuration:"
echo "   sudo nano $CONFIG_FILE"
echo "   (or use web interface)"
echo ""
echo "5. Restart After Changes:"
echo "   sudo systemctl restart p25-hotspot"
echo ""

if grep -q "^enable_uart=1" /boot/config.txt 2>/dev/null || \
   grep -q "^enable_uart=1" /boot/firmware/config.txt 2>/dev/null; then
    if ! grep -q "^enable_uart=1" /boot/config.txt.bak 2>/dev/null && \
       ! grep -q "^enable_uart=1" /boot/firmware/config.txt.bak 2>/dev/null; then
        print_warning "UART was just enabled - reboot required!"
        echo ""
        read -p "Reboot now? (y/N): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            print_info "Rebooting..."
            reboot
        else
            print_warning "Remember to reboot before using GPIO modem!"
        fi
    fi
fi

echo ""
print_success "73 and good DX!"
echo ""
