# Publishing P25 Hotspot to GitHub

## Step 1: Create GitHub Repository

1. Go to https://github.com/new
2. Repository name: `p25-hotspot`
3. Description: `P25 Phase 1 hotspot software for authenticated trunking reflectors`
4. **Public** (so users can curl the install script)
5. **Don't** initialize with README (we already have one)
6. Click "Create repository"

## Step 2: Initialize Git Locally

```bash
cd /Users/perryraybuck/Desktop/MMDVM/p25-hotspot

# Initialize git
git init

# Add all files
git add .

# First commit
git commit -m "Initial commit - P25 Hotspot with web interface"

# Set main branch
git branch -M main

# Add remote (replace YOUR_USERNAME)
git remote add origin https://github.com/YOUR_USERNAME/p25-hotspot.git

# Push to GitHub
git push -u origin main
```

## Step 3: Update Install Script

Before pushing, update the install script with your actual GitHub username:

```bash
# Edit install.sh
nano install.sh

# Change this line:
# git clone https://github.com/YOUR_USERNAME/p25-hotspot.git "$INSTALL_DIR"
# To:
# git clone https://github.com/radxrf/p25-hotspot.git "$INSTALL_DIR"
# (or whatever your username is)

# Also update README.md curl command
nano README.md

# Change:
# curl -sSL https://raw.githubusercontent.com/YOUR_USERNAME/p25-hotspot/main/install.sh | sudo bash
# To your actual username
```

Then commit the changes:

```bash
git add install.sh README.md
git commit -m "Update GitHub URLs with actual username"
git push
```

## Step 4: Test Installation

On a Raspberry Pi, test the one-liner:

```bash
curl -sSL https://raw.githubusercontent.com/YOUR_USERNAME/p25-hotspot/main/install.sh | sudo bash
```

## Step 5: Create a Release (Optional)

1. Go to your repository on GitHub
2. Click "Releases" → "Create a new release"
3. Tag: `v1.0.0`
4. Title: `P25 Hotspot v1.0.0 - Initial Release`
5. Description:
   ```
   # P25 Hotspot v1.0.0

   Custom P25 Phase 1 hotspot software for radxrf.com trunking network.

   ## Features
   - Authenticated reflector connection
   - Full P25 trunking support
   - MMDVM modem integration
   - Local web interface for configuration
   - One-line installation

   ## Installation

   ```bash
   curl -sSL https://raw.githubusercontent.com/YOUR_USERNAME/p25-hotspot/main/install.sh | sudo bash
   ```

   ## Requirements
   - Raspberry Pi 3/4/5 or Zero 2 W
   - MMDVM modem
   - P25-capable radio

   ## Documentation
   - [Installation Guide](INSTALL.md)
   - [Quick Start](QUICKSTART.md)
   - [Web Interface](WEB_INTERFACE.md)
   ```
6. Click "Publish release"

## Step 6: Add Topics (GitHub Tags)

In your repository:
1. Click the gear icon next to "About"
2. Add topics:
   - `p25`
   - `amateur-radio`
   - `ham-radio`
   - `hotspot`
   - `mmdvm`
   - `trunking`
   - `raspberry-pi`
   - `digital-radio`

## Step 7: Update Repository Description

1. Click Settings
2. Add description: `P25 Phase 1 hotspot software for authenticated trunking reflectors - Built for Raspberry Pi with MMDVM modems`
3. Add website: `https://trunking.radxrf.com` (your reflector web interface)

## File Structure on GitHub

Your repository will look like:

```
p25-hotspot/
├── .gitignore
├── README.md                    ← Main page (one-line install)
├── INSTALL.md                   ← Detailed installation
├── QUICKSTART.md                ← 10-minute guide
├── PROGRESS.md                  ← Development status
├── WEB_INTERFACE.md             ← Web UI documentation
├── GITHUB_SETUP.md              ← This file
├── install.sh                   ← Automated installer (IMPORTANT!)
├── CMakeLists.txt
├── config.example.yaml
├── p25-hotspot.service
├── src/                         ← C++ source
│   └── ...
└── web/                         ← Web interface
    └── ...
```

## User Installation Flow

Once on GitHub, users can install with:

```bash
curl -sSL https://raw.githubusercontent.com/YOUR_USERNAME/p25-hotspot/main/install.sh | sudo bash
```

The script will:
1. Update system
2. Install dependencies
3. Clone repo to `/opt/p25-hotspot`
4. Build C++ hotspot
5. Install web interface
6. Run configuration wizard
7. Enable services
8. Start everything

**Total time: ~5-10 minutes**

## Promoting Your Software

### On radxrf.com
Add to your reflector web interface:
```
Hotspot Software Available!
Install on Raspberry Pi with one command:
curl -sSL https://raw.githubusercontent.com/YOUR_USERNAME/p25-hotspot/main/install.sh | sudo bash
Documentation: https://github.com/YOUR_USERNAME/p25-hotspot
```

### Create a Quick Reference Card

```
┌─────────────────────────────────────────────────┐
│  radxrf.com P25 Hotspot Software                │
├─────────────────────────────────────────────────┤
│  Install (one command):                         │
│  curl -sSL https://raw.githubusercontent.com/   │
│  YOUR_USERNAME/p25-hotspot/main/install.sh |    │
│  sudo bash                                       │
│                                                  │
│  Configure:                                      │
│  http://raspberrypi.local:8080                  │
│                                                  │
│  Requirements:                                   │
│  • Raspberry Pi (3/4/5/Zero 2W)                 │
│  • MMDVM modem (GPIO or USB)                    │
│  • P25 radio                                     │
│  • Radio ID from radxrf.com admin               │
└─────────────────────────────────────────────────┘
```

## Maintaining the Repository

### For Updates

```bash
cd /Users/perryraybuck/Desktop/MMDVM/p25-hotspot

# Make changes
# ...

# Commit
git add .
git commit -m "Description of changes"
git push

# Create new release if major update
```

### For Bug Fixes

```bash
# Fix the bug
git add fixed_file.cpp
git commit -m "Fix: Description of bug fix"
git push
```

Users can update with:
```bash
cd /opt/p25-hotspot
git pull
cd build
make
sudo systemctl restart p25-hotspot
```

## Security Notes

### The install script:
- ✅ Safe to curl and pipe to bash (common practice)
- ✅ Users can review first: `curl -sSL URL > install.sh && less install.sh`
- ✅ Exits on errors (`set -e`)
- ✅ Checks for root
- ✅ Asks before destructive operations
- ✅ Preserves existing configs

### The .gitignore:
- ✅ Excludes config.yaml (contains passwords)
- ✅ Excludes .env (web interface secrets)
- ✅ Excludes build artifacts
- ✅ No sensitive data in repository

## Example GitHub README Badge

Add to top of README.md:

```markdown
![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-Raspberry%20Pi-red.svg)
![Language](https://img.shields.io/badge/language-C%2B%2B17-orange.svg)
```

## License

Add a LICENSE file:

```bash
# Create MIT license
cat > LICENSE << 'EOF'
MIT License

Copyright (c) 2025 radxrf.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
EOF

git add LICENSE
git commit -m "Add MIT license"
git push
```

## That's It!

Your P25 hotspot software is now:
- ✅ On GitHub (public)
- ✅ One-line installation
- ✅ Fully documented
- ✅ Easy to maintain
- ✅ Easy to share

Users just need:
```bash
curl -sSL https://raw.githubusercontent.com/YOUR_USERNAME/p25-hotspot/main/install.sh | sudo bash
```

**73!** 📻
