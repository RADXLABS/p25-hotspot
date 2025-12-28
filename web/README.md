# P25 Hotspot - Local Web Interface

Simple web interface for configuring and monitoring your P25 hotspot locally.

## Features

- **Dashboard** - View service status, current configuration, and recent logs
- **Configuration Editor** - Edit all settings via web form (no SSH/nano needed)
- **Live Logs** - View real-time systemd logs
- **Service Control** - Start/stop/restart hotspot service
- **Auto-refresh** - Status and logs update automatically

## Installation

### Install Dependencies

```bash
sudo apt install -y python3 python3-pip
sudo pip3 install -r /opt/p25-hotspot/web/requirements.txt
```

### Install as Service

```bash
# Copy service file
sudo cp /opt/p25-hotspot/web/p25-hotspot-web.service /etc/systemd/system/

# Reload systemd
sudo systemctl daemon-reload

# Enable and start
sudo systemctl enable p25-hotspot-web
sudo systemctl start p25-hotspot-web

# Check status
sudo systemctl status p25-hotspot-web
```

## Access

Open a web browser and go to:

```
http://raspberrypi.local:8080
```

Or use the Pi's IP address:

```
http://192.168.1.x:8080
```

## Pages

### Dashboard (`/`)
- Service status (running/stopped)
- Current configuration summary
- Radio ID, callsign, frequencies
- Recent log entries (auto-refreshing)
- Start/stop/restart buttons

### Configuration (`/config`)
- Edit all hotspot settings
- Reflector connection (address, radio ID, password)
- Modem settings (port, frequencies, power)
- P25 settings (NAC, trunking)
- Logging settings
- **Save & Restart** button applies changes immediately

### Logs (`/logs`)
- View systemd journal logs
- Select number of lines (50/100/200/500)
- Auto-refreshing every 5 seconds
- Auto-scrolls to bottom

## Firewall

If accessing from another computer on the network:

```bash
sudo ufw allow 8080/tcp
```

## Security Note

This interface runs on port 8080 with **no authentication**. It's intended for:
- Local network access only
- Single-user hotspots
- Trusted networks

**Do not expose port 8080 to the internet!**

If you need to access it remotely, use SSH port forwarding:

```bash
# On your computer
ssh -L 8080:localhost:8080 pi@raspberrypi.local

# Then access: http://localhost:8080
```

## Troubleshooting

### Web interface won't start

```bash
# Check logs
sudo journalctl -u p25-hotspot-web -n 50

# Check if port 8080 is in use
sudo netstat -tulpn | grep 8080

# Test manually
cd /opt/p25-hotspot/web
python3 app.py
```

### Can't access from browser

```bash
# Check service is running
sudo systemctl status p25-hotspot-web

# Check firewall
sudo ufw status

# Find Pi's IP address
hostname -I
```

### Configuration won't save

```bash
# Check file permissions
ls -la /etc/p25-hotspot.yaml

# Make sure it's writable by root
sudo chown root:root /etc/p25-hotspot.yaml
sudo chmod 644 /etc/p25-hotspot.yaml
```

## Development

To run in development mode:

```bash
cd /opt/p25-hotspot/web
export FLASK_ENV=development
python3 app.py
```

## API Endpoints

The web interface provides a simple REST API:

- `GET /api/status` - Service and config status
- `GET /api/logs?lines=100` - System logs
- `GET /api/config` - Current configuration (JSON)
- `POST /api/config` - Save configuration (JSON body)
- `POST /api/service/start` - Start service
- `POST /api/service/stop` - Stop service
- `POST /api/service/restart` - Restart service

Example:

```bash
# Get current config
curl http://localhost:8080/api/config

# Restart service
curl -X POST http://localhost:8080/api/service/restart
```
