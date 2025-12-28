# P25 Hotspot - Web Interface Complete!

## Overview

A local web interface has been added to make hotspot configuration and monitoring super easy!

## Features

### 📊 Dashboard (`/`)
- **Service Status** - Real-time running/stopped indicator
- **Quick Stats** - Radio ID, callsign, reflector, frequencies
- **Recent Logs** - Last 20 log lines (auto-refreshing)
- **Control Buttons** - Start, stop, restart service with one click

### ⚙️ Configuration (`/config`)
- **Full Settings Editor** - All config options in one form
- **Organized Sections**:
  - Reflector Settings (address, port, radio ID, password, callsign)
  - Modem Settings (serial port, frequencies, power levels)
  - P25 Settings (NAC, trunking enable/disable)
  - Logging Settings (log level, file path)
- **Visual Controls** - Sliders for power levels, dropdowns for selections
- **Frequency Helper** - Enter frequencies in MHz (auto-converts to Hz)
- **NAC Helper** - Hex input with validation (0x293 format)
- **Save & Restart** - One button applies changes and restarts service

### 📜 Logs (`/logs`)
- **Live Systemd Logs** - Real-time journal output
- **Configurable Lines** - View 50/100/200/500 lines
- **Auto-Refresh** - Updates every 5 seconds
- **Auto-Scroll** - Always shows latest entries

## Technology

- **Flask** - Lightweight Python web framework
- **Terminal UI** - Same retro green/amber theme as reflector
- **No Database** - Reads/writes YAML config directly
- **REST API** - JSON endpoints for programmatic access

## Files Created

```
p25-hotspot/web/
├── app.py                           # Flask application (200 lines)
├── requirements.txt                  # Python dependencies
├── p25-hotspot-web.service          # systemd service file
├── README.md                         # Web interface documentation
├── templates/
│   ├── base.html                    # Base template with navbar
│   ├── index.html                   # Dashboard page
│   ├── config.html                  # Configuration editor
│   └── logs.html                    # Logs viewer
└── static/
    └── css/
        └── style.css                # Retro terminal CSS (same as reflector)
```

## Installation

Already included in INSTALL.md Step 11:

```bash
# Install dependencies
sudo pip3 install -r /opt/p25-hotspot/web/requirements.txt

# Install service
sudo cp /opt/p25-hotspot/web/p25-hotspot-web.service /etc/systemd/system/
sudo systemctl enable p25-hotspot-web
sudo systemctl start p25-hotspot-web
```

## Access

```
http://raspberrypi.local:8080
```

Or use IP address:
```
http://192.168.1.x:8080
```

## API Endpoints

For automation or custom scripts:

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/status` | Service status + config summary |
| GET | `/api/logs?lines=100` | System logs (JSON) |
| GET | `/api/config` | Full configuration (JSON) |
| POST | `/api/config` | Save configuration (JSON body) |
| POST | `/api/service/start` | Start hotspot service |
| POST | `/api/service/stop` | Stop hotspot service |
| POST | `/api/service/restart` | Restart hotspot service |

### Example API Usage

```bash
# Get status
curl http://localhost:8080/api/status | jq

# Get config
curl http://localhost:8080/api/config | jq

# Restart service
curl -X POST http://localhost:8080/api/service/restart

# Get logs
curl http://localhost:8080/api/logs?lines=50 | jq -r .logs
```

## Security

**IMPORTANT:** No authentication! The web interface is intended for:
- **Local network use only**
- **Trusted users**
- **Single-user hotspots**

**DO NOT** expose port 8080 to the internet!

### Remote Access (Secure)

Use SSH port forwarding:

```bash
# On your computer
ssh -L 8080:localhost:8080 pi@raspberrypi.local

# Then access http://localhost:8080 in your browser
```

This tunnels the connection through SSH securely.

## Screenshots

The interface has a **retro terminal theme** matching the reflector:

- Dark background (#0a0e14)
- Green accent (#00ff88) for headings and active elements
- Amber accent (#ffb454) for labels and warnings
- Monospace font (Monaco/Consolas)
- Subtle CRT scanline effect
- Green glowing borders on hover

It looks like old ham radio equipment displays - fitting for a P25 hotspot!

## Benefits

### Before Web Interface:
```bash
ssh pi@raspberrypi
sudo nano /etc/p25-hotspot.yaml
# Edit YAML manually (easy to make syntax errors)
sudo systemctl restart p25-hotspot
# Switch to another terminal
sudo journalctl -u p25-hotspot -f
```

### With Web Interface:
1. Open browser → `http://raspberrypi.local:8080`
2. Click Configuration
3. Edit settings in form
4. Click "Save & Restart"
5. View logs on same page

**Much easier!**

## Use Cases

### For Beginners
- No SSH or command-line needed
- Visual form prevents YAML syntax errors
- Can't accidentally break configuration
- Instant feedback with live logs

### For Advanced Users
- Quick config changes from phone/tablet
- Monitor multiple hotspots from one browser
- REST API for scripting/automation
- Still have full SSH access when needed

### For Troubleshooting
- View logs without SSH
- See exact running configuration
- Restart service without command line
- Monitor status in real-time

## Future Enhancements (Optional)

Possible additions if users want them:

- [ ] **Authentication** - Simple password login
- [ ] **HTTPS** - Self-signed cert for encryption
- [ ] **Multi-user** - Different access levels
- [ ] **Backup/Restore** - Save/load config files
- [ ] **Network Scan** - Auto-detect modem serial port
- [ ] **Frequency Presets** - Common ham bands
- [ ] **Status Graphs** - Uptime, packet counts
- [ ] **Mobile App** - Native iOS/Android wrapper

But honestly, it's perfect as-is for the target use case!

## Summary

**The web interface is production-ready and complete.**

Users can now:
1. Install hotspot software (C++)
2. Install web interface (Python Flask)
3. Never touch SSH/command line again if they don't want to

All configuration and monitoring can be done through the clean, retro-styled web UI at `http://raspberrypi.local:8080`.

---

**Built with Flask 🍶 for radxrf.com**
