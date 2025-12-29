#!/usr/bin/env python3
"""
P25 Hotspot License Validator Service
Runs continuously to validate license every 60 seconds
Kills hotspot service if license is revoked
"""

import sys
import time
import subprocess
import logging
from datetime import datetime

# Add web directory to path to import license module
sys.path.insert(0, '/opt/p25-hotspot/web')
from license import validate_license, is_licensed

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='[%(asctime)s] [%(levelname)s] %(message)s',
    handlers=[
        logging.FileHandler('/var/log/p25-license-validator.log'),
        logging.StreamHandler()
    ]
)

logger = logging.getLogger(__name__)

HOTSPOT_SERVICE = 'p25-hotspot'
VALIDATION_INTERVAL = 60  # seconds


def stop_hotspot_service():
    """Stop the hotspot service."""
    try:
        logger.critical("STOPPING HOTSPOT SERVICE DUE TO LICENSE FAILURE")
        subprocess.run(['systemctl', 'stop', HOTSPOT_SERVICE], check=True)
        return True
    except Exception as e:
        logger.error(f"Failed to stop hotspot service: {e}")
        return False


def main():
    """Main validation loop."""
    logger.info("=" * 60)
    logger.info("P25 Hotspot License Validator Starting")
    logger.info("=" * 60)
    logger.info(f"Validation interval: {VALIDATION_INTERVAL} seconds")
    logger.info(f"Monitoring service: {HOTSPOT_SERVICE}")
    logger.info("")

    consecutive_failures = 0
    max_consecutive_failures = 3  # Allow 3 failures before shutdown (grace period)

    while True:
        try:
            # Check if license exists
            if not is_licensed():
                logger.error("No license activated - hotspot should not be running")
                consecutive_failures += 1
            else:
                # Validate license with reflector
                valid, message = validate_license()

                if valid:
                    logger.info(f"✓ License valid: {message}")
                    consecutive_failures = 0  # Reset on success
                else:
                    logger.warning(f"✗ License validation failed: {message}")
                    consecutive_failures += 1

                    # Check if this is a revocation (contains "REVOKED")
                    if "REVOKED" in message.upper():
                        logger.critical("=" * 60)
                        logger.critical("LICENSE HAS BEEN REVOKED!")
                        logger.critical(f"Reason: {message}")
                        logger.critical("=" * 60)
                        # Immediate shutdown on revocation
                        stop_hotspot_service()
                        logger.critical("Hotspot service stopped. Exiting validator.")
                        sys.exit(1)

            # Check if too many consecutive failures
            if consecutive_failures >= max_consecutive_failures:
                logger.critical("=" * 60)
                logger.critical(f"LICENSE VALIDATION FAILED {consecutive_failures} TIMES")
                logger.critical("Maximum consecutive failures reached")
                logger.critical("=" * 60)
                stop_hotspot_service()
                logger.critical("Hotspot service stopped. Exiting validator.")
                sys.exit(1)

            # Wait for next validation
            time.sleep(VALIDATION_INTERVAL)

        except KeyboardInterrupt:
            logger.info("Received shutdown signal - exiting")
            break

        except Exception as e:
            logger.error(f"Validation error: {e}")
            consecutive_failures += 1

            if consecutive_failures >= max_consecutive_failures:
                logger.critical("Too many errors - stopping hotspot service")
                stop_hotspot_service()
                sys.exit(1)

            time.sleep(VALIDATION_INTERVAL)

    logger.info("License validator stopped")


if __name__ == '__main__':
    main()
