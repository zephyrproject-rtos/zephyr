#!/usr/bin/env python3
#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

"""
This script is used to install TLS credentials on a device via a serial connection.
It supports both deleting and writing credentials, as well as checking for their existence.
It also verifies the hash of the installed credentials against the expected hash.

This script is based on https://github.com/nRFCloud/utils/, specifically
"command_interface.py" and "device_credentials_installer.py".
"""

import argparse
import base64
import hashlib
import logging
import math
import os
import sys
import time

import serial

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

CMD_TERM_DICT = {'NULL': '\0', 'CR': '\r', 'LF': '\n', 'CRLF': '\r\n'}
# 'CR' is the default termination value for the at_host library in the nRF Connect SDK
cmd_term_key = 'CR'

TLS_CRED_TYPES = ["CA", "SERV", "PK"]
TLS_CRED_CHUNK_SIZE = 48
serial_timeout = 1
ser = None


class TLSCredShellInterface:
    def __init__(self, serial_write_line, serial_wait_for_response, verbose):
        self.serial_write_line = serial_write_line
        self.serial_wait_for_response = serial_wait_for_response
        self.verbose = verbose

    def write_raw(self, command):
        if self.verbose:
            logger.debug(f'-> {command}')
        self.serial_write_line(command)

    def write_credential(self, sectag, cred_type, cred_text):
        # Because the Zephyr shell does not support multi-line commands,
        # we must base-64 encode our PEM strings and install them as if they were binary.
        # Yes, this does mean we are base-64 encoding a string which is already mostly base-64.
        # We could alternatively strip the ===== BEGIN/END XXXX ===== header/footer, and then pass
        # everything else directly as a binary payload (using BIN mode instead of BINT, since
        # MBedTLS uses the NULL terminator to determine if the credential is raw DER, or is a
        # PEM string). But this will fail for multi-CA installs, such as CoAP.

        # text -> bytes -> base64 bytes -> base64 text
        encoded = base64.b64encode(cred_text.encode()).decode()
        self.write_raw("cred buf clear")
        chunks = math.ceil(len(encoded) / TLS_CRED_CHUNK_SIZE)
        for c in range(chunks):
            chunk = encoded[c * TLS_CRED_CHUNK_SIZE : (c + 1) * TLS_CRED_CHUNK_SIZE]
            self.write_raw(f"cred buf {chunk}")
            result, output = self.serial_wait_for_response("Stored", "RX ring buffer full")
            if not result:
                logging.error("Failed to store chunk in the device: unknown error")
            if output and b"RX ring buffer full" in output:
                logging.error(f"Failed to store chunk in the device: {output}")
                return False
        if not 0 <= cred_type < len(TLS_CRED_TYPES):
            logger.error(
                f"Invalid credential type: {cred_type}. Range [0, {len(TLS_CRED_TYPES) - 1}]."
            )
            return False
        self.write_raw(f"cred add {sectag} {TLS_CRED_TYPES[cred_type]} DEFAULT bint")
        result, _ = self.serial_wait_for_response("Added TLS credential", "already exists")
        time.sleep(1)
        return result

    def delete_credential(self, sectag, cred_type):
        if not 0 <= cred_type < len(TLS_CRED_TYPES):
            logger.error(
                f"Invalid credential type: {cred_type}. Range [0, {len(TLS_CRED_TYPES) - 1}]."
            )
            return False
        self.write_raw(f'cred del {sectag} {TLS_CRED_TYPES[cred_type]}')
        result, _ = self.serial_wait_for_response(
            "Deleted TLS credential", "There is no TLS credential"
        )
        time.sleep(2)
        return result

    def check_credential_exists(self, sectag, cred_type, get_hash=True):
        self.write_raw(f'cred list {sectag} {TLS_CRED_TYPES[cred_type]}')
        _, output = self.serial_wait_for_response(
            "1 credentials found.",
            "0 credentials found.",
            store=f"{sectag},{TLS_CRED_TYPES[cred_type]}",
        )

        if not output:
            return False, None

        if not get_hash:
            return True, None

        data = output.decode().split(",")
        logger.debug(f"Cred list output: {data}")
        if len(data) < 4:
            logger.error("Invalid output format from device, skipping hash check.")
            return False, None
        cred_hash = data[2].strip()
        status_code = data[3].strip()

        if status_code != "0":
            logger.warning(f"Error retrieving credential hash: {output.decode().strip()}.")
            logger.warning("Device might not support credential digests.")
            return True, None

        return True, cred_hash

    def calculate_expected_hash(self, cred_text):
        cred_hash = hashlib.sha256(cred_text.encode('utf-8') + b'\x00')
        return base64.b64encode(cred_hash.digest()).decode()

    def check_cred_command(self):
        logger.info("Checking for 'cred' command existence...")
        self.serial_write_line("cred")
        result, output = self.serial_wait_for_response(
            "TLS Credentials Commands", "command not found", store="cred"
        )
        logger.debug(f"Result: {result}, Output: {output}")
        if not result:
            logger.error("Device did not respond to 'cred' command.")
            return False
        if output and b"command not found" in output:
            logger.error("Device does not support 'cred' command.")
            logger.error("Hint: Add 'CONFIG_TLS_CREDENTIALS_SHELL=y' to your prj.conf file.")
            return False
        logger.info("'cred' command found.")
        return True


def write_line(line, hidden=False):
    if not hidden:
        logger.debug(f'-> {line}')
    ser.write(bytes((line + CMD_TERM_DICT[cmd_term_key]).encode('utf-8')))


def wait_for_prompt(val1='uart:~$ ', val2=None, timeout=15, store=None):
    found = False
    retval = False
    output = None

    if not ser:
        logger.error('Serial interface not initialized')
        return False, None

    if isinstance(val1, str):
        val1 = val1.encode()

    if isinstance(val2, str):
        val2 = val2.encode()

    if isinstance(store, str):
        store = store.encode()

    ser.flush()

    while not found and timeout != 0:
        try:
            line = ser.readline()
        except serial.SerialException as e:
            logger.error(f"Error reading from serial interface: {e}")
            return False, None
        except Exception as e:
            logger.error(f"Unexpected error: {e}")
            return False, None

        if line == b'\r\n':
            continue

        if line is None or len(line) == 0:
            if timeout > 0:
                timeout -= serial_timeout
            continue

        logger.debug(f'<- {line.decode("utf-8", errors="replace")}')

        if val1 in line:
            found = True
            retval = True
        elif val2 is not None and val2 in line:
            found = True
            retval = False
        elif store is not None and (store in line or str(store) in str(line)):
            output = line

    if b'\n' not in line:
        logger.debug('')

    ser.flush()
    if store is not None and output is None:
        logger.error(f'String {store} not detected in line {line}')

    if timeout == 0:
        logger.error('Serial timeout waiting for prompt')

    return retval, output


def parse_args(in_args):
    parser = argparse.ArgumentParser(
        description="Device Credentials Installer",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        allow_abbrev=False,
    )
    parser.add_argument(
        "-p", "--port", type=str, help="Specify which serial port to open", default="/dev/ttyACM1"
    )
    parser.add_argument(
        "-x",
        "--xonxoff",
        help="Enable software flow control for serial connection",
        action='store_true',
        default=False,
    )
    parser.add_argument(
        "-r",
        "--rtscts-off",
        help="Disable hardware (RTS/CTS) flow control for serial connection",
        action='store_true',
        default=False,
    )
    parser.add_argument(
        "-f",
        "--dsrdtr",
        help="Enable hardware (DSR/DTR) flow control for serial connection",
        action='store_true',
        default=False,
    )
    parser.add_argument(
        "-d", "--delete", help="Delete sectag from device first", action='store_true', default=False
    )
    parser.add_argument(
        "-l",
        "--local-cert-file",
        type=str,
        help="Filepath to a local certificate (PEM) to use for the device",
        required=True,
    )
    parser.add_argument(
        "-t", "--cert-type", type=int, help="Certificate type to use for the device", default=1
    )
    parser.add_argument(
        "-S", "--sectag", type=int, help="integer: Security tag to use", default=16842753
    )
    parser.add_argument(
        "-H",
        "--check-hash",
        help="Check hash of the credential after writing",
        action='store_true',
        default=False,
    )

    parser.add_argument("-v", "--verbose", action="store_true", help="Enable verbose output")
    args = parser.parse_args(in_args)
    return args


def main(in_args):
    global ser

    args = parse_args(in_args)

    if args.verbose:
        logger.setLevel(logging.DEBUG)

    if not os.path.isfile(args.local_cert_file):
        logger.error(f'Local certificate file {args.local_cert_file} does not exist')
        sys.exit(3)

    logger.info(f'Opening port {args.port}')
    try:
        try:
            ser = serial.Serial(
                args.port,
                115200,
                xonxoff=args.xonxoff,
                rtscts=(not args.rtscts_off),
                dsrdtr=args.dsrdtr,
                timeout=serial_timeout,
            )
            ser.reset_input_buffer()
            ser.reset_output_buffer()
        except FileNotFoundError:
            logger.error(f'Specified port {args.port} does not exist or cannot be accessed')
            sys.exit(2)
        except serial.SerialException as e:
            logger.error(f'Failed to open serial port {args.port}: {e}')
            sys.exit(2)
    except serial.serialutil.SerialException:
        logger.error('Port could not be opened; not a device, or open already')
        sys.exit(2)

    cred_if = TLSCredShellInterface(write_line, wait_for_prompt, args.verbose)
    cmd_exits = cred_if.check_cred_command()
    if not cmd_exits:
        sys.exit(1)

    with open(args.local_cert_file) as f:
        dev_bytes = f.read()

    if args.delete:
        logger.info(f'Deleting sectag {args.sectag}...')
        cred_if.delete_credential(args.sectag, args.cert_type)

    result = cred_if.write_credential(args.sectag, args.cert_type, dev_bytes)
    if not result:
        logger.error(f'Failed to write credential for sectag {args.sectag}, it may already exist')
        sys.exit(5)
    logger.info(f'Writing sectag {args.sectag}...')
    result, cred_hash = cred_if.check_credential_exists(
        args.sectag, args.cert_type, args.check_hash
    )
    if args.check_hash:
        logger.debug(f'Checking hash for sectag {args.sectag}...')
    if not result:
        logger.error(f'Failed to check credential existence for sectag {args.sectag}')
        sys.exit(4)
    if cred_hash:
        logger.debug(f'Credential hash: {cred_hash}')
        expected_hash = cred_if.calculate_expected_hash(dev_bytes)
        if cred_hash != expected_hash:
            logger.error(
                f'Hash mismatch for sectag {args.sectag}. Exp: {expected_hash}, got: {cred_hash}'
            )
            sys.exit(6)
    logger.info(f'Credential for sectag {args.sectag} written successfully')
    sys.exit(0)


def run():
    try:
        main(sys.argv[1:])
    except KeyboardInterrupt:
        logger.info("Execution interrupted by user (Ctrl-C). Exiting...")
        sys.exit(1)


if __name__ == '__main__':
    run()
