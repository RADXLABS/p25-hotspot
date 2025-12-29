#!/usr/bin/env python3
"""
P25 Hotspot License Manager
Handles license activation and validation for the hotspot
"""

import requests
import yaml
import os
import re
import subprocess
from datetime import datetime

LICENSE_FILE = '/etc/p25-hotspot-license.yaml'
CONFIG_FILE = '/etc/p25-hotspot.yaml'


def get_mac_address():
    """
    Get the MAC address of the primary network interface.

    Returns:
        str: MAC address in format AA:BB:CC:DD:EE:FF
    """
    try:
        # Try to get eth0 MAC first (wired)
        result = subprocess.run(
            ['cat', '/sys/class/net/eth0/address'],
            capture_output=True,
            text=True
        )

        if result.returncode == 0 and result.stdout.strip():
            return result.stdout.strip().upper()

        # Fall back to wlan0 (wireless)
        result = subprocess.run(
            ['cat', '/sys/class/net/wlan0/address'],
            capture_output=True,
            text=True
        )

        if result.returncode == 0 and result.stdout.strip():
            return result.stdout.strip().upper()

        # Last resort: use ip command
        result = subprocess.run(
            ['ip', 'link', 'show'],
            capture_output=True,
            text=True
        )

        # Find first non-loopback MAC
        for line in result.stdout.split('\n'):
            if 'link/ether' in line:
                mac = line.split()[1]
                return mac.upper()

        raise Exception("Could not find network interface MAC address")

    except Exception as e:
        raise Exception(f"Failed to get MAC address: {str(e)}")


def load_license():
    """
    Load license information from file.

    Returns:
        dict or None: License info if activated, None otherwise
    """
    if not os.path.exists(LICENSE_FILE):
        return None

    try:
        with open(LICENSE_FILE, 'r') as f:
            data = yaml.safe_load(f)
            if data and data.get('activated'):
                return data
    except Exception:
        pass

    return None


def save_license(license_key, mac_address):
    """
    Save activated license to file.

    Args:
        license_key: The license key
        mac_address: The MAC address
    """
    data = {
        'activated': True,
        'license_key': license_key,
        'mac_address': mac_address,
        'activated_at': datetime.utcnow().isoformat()
    }

    with open(LICENSE_FILE, 'w') as f:
        yaml.dump(data, f, default_flow_style=False)

    # Set restrictive permissions (root only)
    os.chmod(LICENSE_FILE, 0o600)


def get_reflector_address():
    """
    Get reflector address from config file.

    Returns:
        str: Reflector URL (https://hostname or http://IP:PORT)
    """
    try:
        with open(CONFIG_FILE, 'r') as f:
            config = yaml.safe_load(f)
            # Check if there's a web_url configured (for HTTPS domains)
            if 'reflector' in config and 'web_url' in config['reflector']:
                return config['reflector']['web_url']
            # Otherwise use IP:port format
            address = config['reflector']['address']
            port = config['reflector'].get('port', 41000)
            return f"http://{address}:5000"  # Web interface port
    except Exception:
        # Default fallback to HTTPS domain
        return "https://trunking.radxrf.com"


def activate_license(license_key):
    """
    Activate a license key with the reflector server.

    Args:
        license_key: The license key to activate

    Returns:
        tuple: (success: bool, message: str)
    """
    try:
        mac_address = get_mac_address()
        reflector_url = get_reflector_address()

        # Call reflector API
        response = requests.post(
            f"{reflector_url}/api/license/activate",
            json={
                'license_key': license_key.strip().upper(),
                'mac_address': mac_address
            },
            timeout=10
        )

        if response.status_code == 200:
            data = response.json()
            if data.get('success'):
                # Save license locally
                save_license(license_key.strip().upper(), mac_address)
                return True, "License activated successfully!"
            else:
                return False, data.get('error', 'Activation failed')

        elif response.status_code == 401:
            return False, "Invalid license key or MAC address mismatch"

        elif response.status_code == 403:
            return False, "License has been revoked"

        else:
            data = response.json()
            return False, data.get('error', f'Activation failed (HTTP {response.status_code})')

    except requests.exceptions.Timeout:
        return False, "Connection to reflector server timed out"

    except requests.exceptions.ConnectionError:
        return False, "Cannot connect to reflector server"

    except Exception as e:
        return False, f"Activation error: {str(e)}"


def validate_license():
    """
    Validate the current license with the reflector server.
    Called every minute by the hotspot service.

    Returns:
        tuple: (valid: bool, message: str)
    """
    try:
        license_data = load_license()

        if not license_data:
            return False, "No license activated"

        license_key = license_data['license_key']
        mac_address = license_data['mac_address']
        reflector_url = get_reflector_address()

        # Call reflector API
        response = requests.post(
            f"{reflector_url}/api/license/validate",
            json={
                'license_key': license_key,
                'mac_address': mac_address
            },
            timeout=10
        )

        if response.status_code == 200:
            data = response.json()
            if data.get('valid'):
                return True, "License is valid"
            else:
                return False, data.get('error', 'License invalid')

        elif response.status_code == 403:
            data = response.json()
            reason = data.get('revoked_reason', 'License has been revoked')
            return False, f"LICENSE REVOKED: {reason}"

        else:
            data = response.json()
            return False, data.get('error', 'Validation failed')

    except requests.exceptions.Timeout:
        # If we can't reach the server, allow a grace period
        # Don't kill the hotspot immediately for network issues
        return True, "Validation timeout (grace period)"

    except requests.exceptions.ConnectionError:
        # Same as timeout - grace period
        return True, "Cannot reach reflector (grace period)"

    except Exception as e:
        return False, f"Validation error: {str(e)}"


def is_licensed():
    """
    Check if the hotspot is currently licensed.

    Returns:
        bool: True if license exists and is activated
    """
    license_data = load_license()
    return license_data is not None and license_data.get('activated') == True


# CLI interface for testing
if __name__ == '__main__':
    import sys

    if len(sys.argv) < 2:
        print("P25 Hotspot License Manager")
        print()
        print("Usage:")
        print("  python3 license.py status     - Check license status")
        print("  python3 license.py activate <key> - Activate license")
        print("  python3 license.py validate   - Validate current license")
        print("  python3 license.py mac        - Show MAC address")
        sys.exit(1)

    command = sys.argv[1]

    if command == 'status':
        if is_licensed():
            lic = load_license()
            print(f"✓ Licensed")
            print(f"  MAC: {lic['mac_address']}")
            print(f"  Activated: {lic['activated_at']}")
        else:
            print("✗ Not licensed")

    elif command == 'activate':
        if len(sys.argv) < 3:
            print("Error: License key required")
            sys.exit(1)

        key = sys.argv[2]
        success, message = activate_license(key)
        if success:
            print(f"✓ {message}")
        else:
            print(f"✗ {message}")
            sys.exit(1)

    elif command == 'validate':
        valid, message = validate_license()
        if valid:
            print(f"✓ {message}")
        else:
            print(f"✗ {message}")
            sys.exit(1)

    elif command == 'mac':
        try:
            mac = get_mac_address()
            print(f"MAC Address: {mac}")
        except Exception as e:
            print(f"Error: {e}")
            sys.exit(1)

    else:
        print(f"Unknown command: {command}")
        sys.exit(1)
