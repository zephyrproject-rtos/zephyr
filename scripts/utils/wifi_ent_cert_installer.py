#!/usr/bin/env python3
# Copyright (c) 2025, Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

import argparse
import logging
import os
import signal
import subprocess
import sys


def signal_handler(sig, frame):
    logger.info('Script terminated by user')
    sys.exit(0)


def main():
    signal.signal(signal.SIGINT, signal_handler)
    parser = argparse.ArgumentParser(description='Install Wi-Fi certificates', allow_abbrev=False)
    parser.add_argument('--path', required=True, help='Path to certificate files')
    parser.add_argument(
        '--serial-device', default='/dev/ttyACM1', help='Serial port device (default: /dev/ttyACM1)'
    )
    parser.add_argument(
        '--operation-mode',
        choices=['AP', 'STA'],
        default='STA',
        help='Operation mode: AP or STA (default: STA)',
    )
    parser.add_argument('-v', '--verbose', action='store_true', help='Enable verbose output')
    args = parser.parse_args()

    # Configure logging
    log_level = logging.DEBUG if args.verbose else logging.INFO
    logging.basicConfig(level=log_level, format='%(asctime)s - %(levelname)s - %(message)s')
    global logger
    logger = logging.getLogger(__name__)

    cert_path = args.path
    port = args.serial_device
    mode = args.operation_mode
    if not os.path.isdir(cert_path):
        logger.error(f"Directory {cert_path} does not exist.")
        sys.exit(1)

    logger.warning(
        "Please make sure that the Serial port is not being used by another application."
    )
    input("Press Enter to continue or Ctrl+C to exit...")

    # TLS credential types
    TLS_CREDENTIAL_CA_CERTIFICATE = 0
    TLS_CREDENTIAL_PUBLIC_CERTIFICATE = 1
    TLS_CREDENTIAL_PRIVATE_KEY = 2

    WIFI_CERT_SEC_TAG_BASE = 0x1020001
    WIFI_CERT_SEC_TAG_MAP = {
        "ca.pem": (TLS_CREDENTIAL_CA_CERTIFICATE, WIFI_CERT_SEC_TAG_BASE),
        "client-key.pem": (TLS_CREDENTIAL_PRIVATE_KEY, WIFI_CERT_SEC_TAG_BASE + 1),
        "server-key.pem": (TLS_CREDENTIAL_PRIVATE_KEY, WIFI_CERT_SEC_TAG_BASE + 2),
        "client.pem": (TLS_CREDENTIAL_PUBLIC_CERTIFICATE, WIFI_CERT_SEC_TAG_BASE + 3),
        "server.pem": (TLS_CREDENTIAL_PUBLIC_CERTIFICATE, WIFI_CERT_SEC_TAG_BASE + 4),
        "ca2.pem": (TLS_CREDENTIAL_CA_CERTIFICATE, WIFI_CERT_SEC_TAG_BASE + 5),
        "client-key2.pem": (TLS_CREDENTIAL_PRIVATE_KEY, WIFI_CERT_SEC_TAG_BASE + 6),
        "client2.pem": (TLS_CREDENTIAL_PUBLIC_CERTIFICATE, WIFI_CERT_SEC_TAG_BASE + 7),
    }

    cert_files = (
        ["ca.pem", "server-key.pem", "server.pem"]
        if mode == "AP"
        else ["ca.pem", "client-key.pem", "client.pem", "ca2.pem", "client-key2.pem", "client2.pem"]
    )

    total_certs = len(cert_files)
    for idx, cert in enumerate(cert_files, 1):
        logger.info(f"Processing certificate {idx} of {total_certs}: {cert}")

        cert_file_path = os.path.join(cert_path, cert)
        if not os.path.isfile(cert_file_path):
            logger.warning(f"Certificate file {cert_file_path} does not exist. Skipping...")
            continue

        cert_type, sec_tag = WIFI_CERT_SEC_TAG_MAP[cert]
        try:
            command = [
                "./scripts/utils/tls_creds_installer.py",
                "-p",
                port,
                "-l",
                cert_file_path,
                "-d",
                "-t",
                str(cert_type),
                "-S",
                str(sec_tag),
            ]
            if args.verbose:
                command.append("-v")

            subprocess.run(command, check=True)
            logger.info(f"Successfully installed {cert}.")
        except subprocess.CalledProcessError:
            logger.error(f"Failed to install {cert}.")

    logger.info("Certificate installation process completed.")


if __name__ == "__main__":
    main()
