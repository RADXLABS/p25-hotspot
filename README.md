# P25 Hotspot Software

Custom P25 Phase 1 hotspot software designed to work with authenticated P25 trunking reflectors.

Built for **radxrf.com** P25 trunking network.

## Features

- **P25 Phase 1 Support** - Voice and trunking signaling
- **Authenticated Connection** - Connects to reflector with radio ID and password
- **MMDVM Modem Support** - Works with standard MMDVM hardware
- **Trunking Support** - Full P25 trunking with control and traffic channels
- **Web Interface** - Easy configuration via browser (no SSH needed!)
- **Lightweight** - Focused on P25 only
- **Raspberry Pi Optimized** - Built for ARM processors

## Hardware Requirements

- Raspberry Pi (3/4/5 or Zero 2 W)
- MMDVM modem (duplex or simplex)
- P25-capable radio

## One-Line Installation

**Easiest method** - automated installation script:

```bash
curl -sSL https://raw.githubusercontent.com/YOUR_USERNAME/p25-hotspot/main/install.sh | sudo bash
```

The installer will:
- ✅ Install all dependencies
- ✅ Build the C++ hotspot software
- ✅ Install the web interface
- ✅ Run configuration wizard
- ✅ Set up systemd services
- ✅ Enable GPIO UART (if needed)

**That's it!** After installation, access the web interface at:
```
http://raspberrypi.local:8080
```

## Manual Installation

If you prefer to install manually, see [INSTALL.md](INSTALL.md) for detailed step-by-step instructions.

## Quick Configuration

After installation, you can configure via:

### Option 1: Web Interface (Recommended)
```
http://raspberrypi.local:8080
```
- Dashboard with live status
- Visual configuration editor
- Real-time logs
- Service control buttons

### Option 2: Command Line
```bash
sudo nano /etc/p25-hotspot.yaml
sudo systemctl start p25-hotspot
```

## Configuration

Edit `/etc/p25-hotspot.yaml`:

```yaml
# Reflector connection
reflector:
  address: "199.201.221.8"
  port: 41000
  radio_id: 123456
  password: "your_password_here"
  callsign: "N0CALL"

# MMDVM modem settings
modem:
  port: "/dev/ttyAMA0"
  baud: 115200
  rx_frequency: 449000000
  tx_frequency: 444000000
  tx_power: 50

# P25 settings
p25:
  nac: 0x293
  enabled: true
  trunking: true
```

## Building from Source

```bash
mkdir build
cd build
cmake ..
make
sudo make install
```

## Architecture

```
┌──────────────┐         ┌──────────────┐         ┌──────────────┐
│   P25 Radio  │ ◄─────► │ MMDVM Modem  │ ◄─────► │   Hotspot    │
│              │  RF     │  (Hardware)  │ Serial  │   Software   │
└──────────────┘         └──────────────┘         └──────┬───────┘
                                                          │
                                                          │ UDP
                                                          │ (Authenticated)
                                                          ▼
                                                  ┌───────────────┐
                                                  │  P25 Reflector│
                                                  │    Server     │
                                                  └───────────────┘
```

## Components

- **main.cpp** - Main entry point, initialization
- **Config.cpp** - YAML configuration loading
- **ModemSerial.cpp** - MMDVM serial communication
- **P25Protocol.cpp** - P25 frame encoding/decoding
- **NetworkClient.cpp** - UDP client with authentication
- **TrunkingController.cpp** - Trunking signaling logic
- **Logger.cpp** - Logging system

## License

MIT

## Author

Built for radxrf.com P25 trunking network
