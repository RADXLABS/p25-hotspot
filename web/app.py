#!/usr/bin/env python3
"""
P25 Hotspot Web Interface
Simple local configuration and monitoring interface
"""

from flask import Flask, render_template, request, jsonify, redirect, url_for
import yaml
import subprocess
import os
import re
from datetime import datetime
from license import is_licensed, activate_license, get_mac_address, load_license

app = Flask(__name__)

CONFIG_FILE = '/etc/p25-hotspot.yaml'
SERVICE_NAME = 'p25-hotspot'

def load_config():
    """Load current configuration."""
    try:
        with open(CONFIG_FILE, 'r') as f:
            return yaml.safe_load(f)
    except Exception as e:
        return None

def save_config(config):
    """Save configuration to file."""
    try:
        with open(CONFIG_FILE, 'w') as f:
            yaml.dump(config, f, default_flow_style=False, sort_keys=False)
        return True
    except Exception as e:
        print(f"Error saving config: {e}")
        return False

def get_service_status():
    """Get systemd service status."""
    try:
        result = subprocess.run(
            ['systemctl', 'is-active', SERVICE_NAME],
            capture_output=True,
            text=True
        )
        active = result.stdout.strip() == 'active'

        # Get detailed status
        result = subprocess.run(
            ['systemctl', 'status', SERVICE_NAME],
            capture_output=True,
            text=True
        )
        status_text = result.stdout

        return {
            'active': active,
            'status': 'Running' if active else 'Stopped',
            'details': status_text
        }
    except Exception as e:
        return {
            'active': False,
            'status': 'Unknown',
            'details': str(e)
        }

def get_service_logs(lines=50):
    """Get recent service logs."""
    try:
        result = subprocess.run(
            ['journalctl', '-u', SERVICE_NAME, '-n', str(lines), '--no-pager'],
            capture_output=True,
            text=True
        )
        return result.stdout
    except Exception as e:
        return f"Error getting logs: {e}"

def restart_service():
    """Restart the hotspot service."""
    try:
        subprocess.run(['systemctl', 'restart', SERVICE_NAME], check=True)
        return True
    except Exception as e:
        return False

def stop_service():
    """Stop the hotspot service."""
    try:
        subprocess.run(['systemctl', 'stop', SERVICE_NAME], check=True)
        return True
    except Exception as e:
        return False

def start_service():
    """Start the hotspot service."""
    try:
        subprocess.run(['systemctl', 'start', SERVICE_NAME], check=True)
        return True
    except Exception as e:
        return False

@app.route('/')
def index():
    """Main dashboard - or license activation if not licensed."""
    # Check if licensed
    if not is_licensed():
        # Redirect to license activation
        return redirect(url_for('license_page'))

    config = load_config()
    status = get_service_status()

    return render_template('index.html',
                         config=config,
                         status=status)

@app.route('/license')
def license_page():
    """License activation page."""
    try:
        mac_address = get_mac_address()
    except Exception as e:
        mac_address = f"Error: {str(e)}"

    license_data = load_license()

    return render_template('license.html',
                         mac_address=mac_address,
                         license=license_data)

@app.route('/config')
def config_page():
    """Configuration page."""
    # Require license
    if not is_licensed():
        return redirect(url_for('license_page'))

    config = load_config()
    return render_template('config.html', config=config)

@app.route('/logs')
def logs_page():
    """Logs page."""
    # Require license
    if not is_licensed():
        return redirect(url_for('license_page'))

    logs = get_service_logs(100)
    return render_template('logs.html', logs=logs)

@app.route('/api/status')
def api_status():
    """API endpoint for service status."""
    status = get_service_status()
    config = load_config()

    return jsonify({
        'service': status,
        'config': {
            'radio_id': config.get('reflector', {}).get('radio_id'),
            'callsign': config.get('reflector', {}).get('callsign'),
            'reflector': config.get('reflector', {}).get('address'),
        } if config else {}
    })

@app.route('/api/logs')
def api_logs():
    """API endpoint for logs."""
    lines = request.args.get('lines', 50, type=int)
    logs = get_service_logs(lines)
    return jsonify({'logs': logs})

@app.route('/api/config', methods=['GET', 'POST'])
def api_config():
    """API endpoint for configuration."""
    if request.method == 'GET':
        config = load_config()
        return jsonify(config)

    elif request.method == 'POST':
        new_config = request.json

        if save_config(new_config):
            # Restart service to apply changes
            restart_service()
            return jsonify({'success': True, 'message': 'Configuration saved and service restarted'})
        else:
            return jsonify({'success': False, 'message': 'Failed to save configuration'}), 500

@app.route('/api/service/<action>', methods=['POST'])
def api_service_action(action):
    """API endpoint for service control."""
    success = False

    if action == 'restart':
        success = restart_service()
        message = 'Service restarted' if success else 'Failed to restart service'
    elif action == 'stop':
        success = stop_service()
        message = 'Service stopped' if success else 'Failed to stop service'
    elif action == 'start':
        success = start_service()
        message = 'Service started' if success else 'Failed to start service'
    else:
        return jsonify({'success': False, 'message': 'Invalid action'}), 400

    return jsonify({'success': success, 'message': message})

@app.route('/api/license/activate', methods=['POST'])
def api_activate_license():
    """API endpoint for license activation."""
    data = request.json

    if not data or 'license_key' not in data:
        return jsonify({'success': False, 'error': 'Missing license key'}), 400

    license_key = data['license_key']

    # Activate license
    success, message = activate_license(license_key)

    if success:
        # Start hotspot and license validator services
        try:
            subprocess.run(['systemctl', 'start', SERVICE_NAME], check=True)
            subprocess.run(['systemctl', 'start', 'p25-license-validator'], check=True)
        except Exception as e:
            pass  # Services will start eventually

        return jsonify({'success': True, 'message': message})
    else:
        return jsonify({'success': False, 'error': message}), 400

@app.route('/api/license/status', methods=['GET'])
def api_license_status():
    """API endpoint for license status."""
    try:
        mac_address = get_mac_address()
    except Exception as e:
        mac_address = None

    license_data = load_license()

    return jsonify({
        'licensed': is_licensed(),
        'mac_address': mac_address,
        'license': license_data
    })

if __name__ == '__main__':
    # Run on all interfaces, port 8080
    app.run(host='0.0.0.0', port=8080, debug=False)
