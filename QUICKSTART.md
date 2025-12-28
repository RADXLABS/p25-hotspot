# P25 Hotspot - Quick Start Guide

## TL;DR - Get Running in 10 Minutes

### 1. Copy to Raspberry Pi
```bash
scp -r p25-hotspot pi@your-pi.local:/home/pi/
```

### 2. SSH and Build
```bash
ssh pi@your-pi.local
sudo apt update && sudo apt install -y build-essential cmake libyaml-cpp-dev
cd /home/pi/p25-hotspot
mkdir build && cd build
cmake .. && make -j4
sudo make install
```

### 3. Configure
```bash
sudo cp /etc/p25-hotspot.yaml.example /etc/p25-hotspot.yaml
sudo nano /etc/p25-hotspot.yaml
```

**Change these lines:**
```yaml
reflector:
  address: "199.201.221.8"
  radio_id: YOUR_RADIO_ID           # ← Your DMR ID
  password: "your_password"          # ← Get from reflector admin
  callsign: "YOUR_CALL"              # ← Your callsign

modem:
  port: "/dev/ttyAMA0"               # ← GPIO: ttyAMA0, USB: ttyUSB0
  rx_frequency: 449000000            # ← Your RX freq in Hz
  tx_frequency: 444000000            # ← Your TX freq in Hz
```

### 4. Enable GPIO UART (if using GPIO modem)
```bash
sudo raspi-config
# 3. Interface Options → P6. Serial Port
# Login shell: NO
# Hardware enabled: YES
sudo reboot
```

### 5. Test Run
```bash
sudo /usr/local/bin/p25-hotspot /etc/p25-hotspot.yaml
```

Should see:
```
✓ Authentication successful!
✓ P25 Hotspot Running
```

### 6. Install Service
```bash
sudo cp /home/pi/p25-hotspot/p25-hotspot.service /etc/systemd/system/
sudo systemctl enable p25-hotspot
sudo systemctl start p25-hotspot
```

### 7. Check Status
```bash
sudo systemctl status p25-hotspot
sudo journalctl -u p25-hotspot -f
```

---

## Serial Port Quick Reference

| Hardware | Typical Port |
|----------|-------------|
| RPi GPIO (MMDVM_HS) | `/dev/ttyAMA0` or `/dev/ttyS0` |
| USB Modem | `/dev/ttyUSB0` or `/dev/ttyACM0` |
| ZUMspot (RPi GPIO) | `/dev/ttyAMA0` |
| Nano Hotspot (USB) | `/dev/ttyUSB0` |

Find yours:
```bash
ls -l /dev/tty*
sudo dmesg | grep tty
```

---

## Frequency Conversion

**MHz to Hz:** Multiply by 1,000,000

Examples:
- 449.000 MHz = 449000000 Hz
- 444.500 MHz = 444500000 Hz
- 147.450 MHz = 147450000 Hz

---

## Common Issues

### "Failed to open serial port"
```bash
# Check port exists
ls -l /dev/ttyAMA0

# Add user to dialout group
sudo usermod -a -G dialout pi
# Logout and back in
```

### "Authentication failed"
- Check radio ID matches database
- Check password is correct
- Verify reflector is running: `ping 199.201.221.8`

### "No modem version"
- Wrong serial port
- UART not enabled (run raspi-config)
- Bad modem connection

---

## Useful Commands

```bash
# View live logs
sudo journalctl -u p25-hotspot -f

# Restart service
sudo systemctl restart p25-hotspot

# Stop service
sudo systemctl stop p25-hotspot

# Check if running
sudo systemctl status p25-hotspot

# Disable auto-start
sudo systemctl disable p25-hotspot

# Test without service
sudo /usr/local/bin/p25-hotspot /etc/p25-hotspot.yaml
```

---

## Optional: Install Web Interface

Makes configuration way easier!

```bash
sudo pip3 install -r /opt/p25-hotspot/web/requirements.txt
sudo cp /opt/p25-hotspot/web/p25-hotspot-web.service /etc/systemd/system/
sudo systemctl enable p25-hotspot-web
sudo systemctl start p25-hotspot-web
```

Access at: **http://raspberrypi.local:8080**

- Dashboard with live status
- Easy configuration editor (no more nano!)
- Live logs viewer
- Start/stop/restart buttons

---

## What Next?

1. **Add your radio ID** to reflector database (via web interface or CLI)
2. **Configure your P25 radio** to match hotspot frequencies
3. **Set NAC to 0x293** (or whatever you configured)
4. **Key up and test!**

Your voice should go through the hotspot → reflector → back out to all connected radios.

**Pro Tip:** Use the web interface at `http://raspberrypi.local:8080` to change settings!

---

## File Locations

| File | Location |
|------|----------|
| Binary | `/usr/local/bin/p25-hotspot` |
| Config | `/etc/p25-hotspot.yaml` |
| Service | `/etc/systemd/system/p25-hotspot.service` |
| Logs | `sudo journalctl -u p25-hotspot` |
| Source | `/opt/p25-hotspot` (or wherever you put it) |

---

## Getting Help

1. Check logs: `sudo journalctl -u p25-hotspot -n 100`
2. Review INSTALL.md for detailed troubleshooting
3. Check reflector web interface: https://trunking.radxrf.com
4. Verify radio is in database with correct password

---

**73 and good DX! 📻**
