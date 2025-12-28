# P25 Hotspot - Installation Guide

## Prerequisites

### Hardware
- Raspberry Pi 3, 4, 5, or Zero 2 W
- MMDVM modem board (MMDVM_HS, ZUMspot, MMDVM Repeater Builder board, etc.)
- MicroSD card (8GB minimum)
- P25-capable radio

### Software
- Raspberry Pi OS (Bullseye or newer)
- Internet connection

## Installation Steps

### Step 1: Install Raspberry Pi OS

1. Download Raspberry Pi Imager: https://www.raspberrypi.com/software/
2. Flash Raspberry Pi OS Lite (64-bit recommended) to SD card
3. Boot the Raspberry Pi
4. Run initial setup: `sudo raspi-config`
   - Set hostname
   - Enable SSH (if needed)
   - Set timezone
   - Expand filesystem

### Step 2: Install Dependencies

```bash
sudo apt update
sudo apt upgrade -y
sudo apt install -y build-essential cmake git libyaml-cpp-dev
```

### Step 3: Download/Copy Source Code

**Option A: From USB/SCP**
```bash
# Copy the p25-hotspot folder to the Pi
scp -r p25-hotspot pi@raspberrypi.local:/home/pi/
```

**Option B: From Git (if you've pushed it)**
```bash
cd /opt
sudo git clone <your-repo-url> p25-hotspot
```

**Option C: Manual copy**
Copy the entire `p25-hotspot` directory to `/opt/p25-hotspot`

### Step 4: Build

```bash
cd /opt/p25-hotspot
mkdir build
cd build
cmake ..
make -j4

# This will create the 'p25-hotspot' binary
```

### Step 5: Install

```bash
# From the build directory
sudo make install

# This installs:
# - /usr/local/bin/p25-hotspot (binary)
# - /etc/p25-hotspot.yaml.example (example config)
```

### Step 6: Configure

```bash
# Copy example config
sudo cp /etc/p25-hotspot.yaml.example /etc/p25-hotspot.yaml

# Edit configuration
sudo nano /etc/p25-hotspot.yaml
```

**Required settings to change:**

```yaml
reflector:
  address: "199.201.221.8"           # Your reflector server IP
  radio_id: YOUR_RADIO_ID            # Your DMR ID / P25 ID
  password: "your_password"          # Password from reflector admin
  callsign: "YOUR_CALL"              # Your callsign

modem:
  port: "/dev/ttyAMA0"               # Serial port for MMDVM
                                     # RPi GPIO: /dev/ttyAMA0
                                     # USB modem: /dev/ttyUSB0 or /dev/ttyACM0
  rx_frequency: 449000000            # RX frequency in Hz (449.000 MHz)
  tx_frequency: 444000000            # TX frequency in Hz (444.000 MHz)
```

### Step 7: Find Your Modem Serial Port

```bash
# List all serial devices
ls -l /dev/tty*

# Common locations:
# - Raspberry Pi GPIO (built-in): /dev/ttyAMA0 or /dev/ttyS0
# - USB modem: /dev/ttyUSB0 or /dev/ttyACM0

# To test which one, look for the device when you plug/unplug the modem:
sudo dmesg | grep tty
```

**For Raspberry Pi GPIO (MMDVM_HS, etc.):**

You need to disable console on serial and enable UART:

```bash
sudo raspi-config
# 3. Interface Options
# P6. Serial Port
# - Login shell over serial: NO
# - Serial port hardware: YES
# Finish and reboot

# Verify UART is enabled
cat /boot/config.txt | grep enable_uart
# Should show: enable_uart=1
```

### Step 8: Test Run (Manual)

```bash
# Run manually to test
sudo /usr/local/bin/p25-hotspot /etc/p25-hotspot.yaml
```

You should see:
```
============================================================
  P25 Hotspot Software v1.0.0
  Built for radxrf.com P25 Trunking Network
============================================================

Using config file: /etc/p25-hotspot.yaml
[INFO] Configuration loaded from /etc/p25-hotspot.yaml
[INFO] Opening modem on /dev/ttyAMA0
[INFO] Serial port opened successfully
[INFO] Modem version: MMDVM
[INFO] Modem initialized successfully
[INFO] Connecting to reflector...
[INFO] Connected to reflector at 199.201.221.8:41000
[INFO] Authenticating with reflector...
[INFO] Radio ID: 123456 (N0CALL)
[INFO] ✓ Authentication successful!
[INFO] ✓ P25 Hotspot Running
============================================================
```

Press Ctrl+C to stop.

### Step 9: Install as Service

```bash
# Copy service file
sudo cp /opt/p25-hotspot/p25-hotspot.service /etc/systemd/system/

# Reload systemd
sudo systemctl daemon-reload

# Enable service (start on boot)
sudo systemctl enable p25-hotspot

# Start service
sudo systemctl start p25-hotspot

# Check status
sudo systemctl status p25-hotspot

# View logs
sudo journalctl -u p25-hotspot -f
```

### Step 10: Configure Your Radio

Set your P25 radio to:
- **Frequency**: Match modem RX/TX frequencies
- **NAC**: 0x293 (or whatever you set in config)
- **Mode**: P25 Phase 1
- **Radio ID**: Same as configured in reflector

Key up and you should hear yourself through the reflector!

## Troubleshooting

### Modem not found
```bash
# Check serial port exists
ls -l /dev/ttyAMA0

# Check permissions
sudo usermod -a -G dialout $USER
# Then logout and back in

# For GPIO UART, ensure it's enabled
cat /boot/config.txt | grep enable_uart
```

### Authentication failed
- Check radio ID and password match what's in the reflector database
- Run: `sudo journalctl -u p25-hotspot -f` to see detailed logs
- Verify reflector is reachable: `ping 199.201.221.8`

### No audio
- Check frequencies match between hotspot config and radio
- Verify NAC matches
- Check modem TX power setting in config
- Look for "Voice call started" in logs when you key up

### Service won't start
```bash
# Check logs
sudo journalctl -u p25-hotspot -n 50

# Check config syntax
sudo /usr/local/bin/p25-hotspot /etc/p25-hotspot.yaml

# Check file permissions
sudo chown root:root /etc/p25-hotspot.yaml
sudo chmod 600 /etc/p25-hotspot.yaml
```

## Updating

```bash
cd /opt/p25-hotspot
git pull  # or copy new files

cd build
make -j4
sudo make install

sudo systemctl restart p25-hotspot
```

## Step 11: Install Web Interface (Optional but Recommended)

The web interface provides an easy way to configure and monitor your hotspot from any device on your network.

```bash
# Install Python dependencies
sudo pip3 install -r /opt/p25-hotspot/web/requirements.txt

# Install web interface service
sudo cp /opt/p25-hotspot/web/p25-hotspot-web.service /etc/systemd/system/

# Enable and start
sudo systemctl daemon-reload
sudo systemctl enable p25-hotspot-web
sudo systemctl start p25-hotspot-web

# Check status
sudo systemctl status p25-hotspot-web
```

### Access Web Interface

Open a browser and go to:
```
http://raspberrypi.local:8080
```

Or use your Pi's IP address:
```
http://192.168.1.x:8080
```

### Web Interface Features

- **Dashboard** - Service status, config summary, live logs
- **Configuration** - Edit all settings via web form (no SSH needed!)
- **Logs** - View real-time systemd logs
- **Service Control** - Start/stop/restart buttons

### Allow Through Firewall (if needed)

```bash
sudo ufw allow 8080/tcp
```

**Security Note:** The web interface has no password. Only use on trusted local networks. To access remotely, use SSH port forwarding:

```bash
ssh -L 8080:localhost:8080 pi@raspberrypi.local
```

Then access at `http://localhost:8080` on your computer.

## Uninstalling

```bash
# Stop and disable services
sudo systemctl stop p25-hotspot p25-hotspot-web
sudo systemctl disable p25-hotspot p25-hotspot-web

# Remove files
sudo rm /usr/local/bin/p25-hotspot
sudo rm /etc/systemd/system/p25-hotspot.service
sudo rm /etc/systemd/system/p25-hotspot-web.service
sudo rm /etc/p25-hotspot.yaml
sudo rm -rf /opt/p25-hotspot

# Reload systemd
sudo systemctl daemon-reload
```

## Support

For issues or questions:
- Local hotspot web interface: `http://raspberrypi.local:8080`
- Reflector web interface: https://trunking.radxrf.com
