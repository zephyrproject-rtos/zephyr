#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2025 Netfeasa Ltd.
"""
HL78xx Modem Firmware Update Script
====================================
This script performs firmware updates for Sierra Wireless HL78xx cellular modems
over serial UART using the XMODEM-1K protocol.
The script connects directly to the modem UART port and executes the firmware
update sequence using AT commands and XMODEM file transfer.
Requirements:
    pip install pyserial xmodem
Supported Modems:
    - Sierra Wireless HL7800
    - Sierra Wireless HL7802
    - Sierra Wireless HL7810
    - Sierra Wireless HL7812
    - Other HL78xx variants
Architecture:
    Host PC (This Script) <-> Modem UART <-> HL78xx Modem
Update Sequence:
    1. Connect to modem UART (e.g., /dev/ttyUSB0, COM5)
    2. Enable WDSI indications (AT+WDSI=?)
    3. Send AT+WDSD=<file_size> to initiate download
    4. Modem enters XMODEM mode and sends ready signal (NAK or 'C')
    5. Perform XMODEM-1K transfer of firmware file
    6. Modem sends "OK" then "+WDSI:3" (download complete)
    7. Send AT+WDSR=4 to start installation
    8. Wait for installation result: "+WDSI:16" (success) or "+WDSI:15" (failure)
Usage:
    python hl78xx_firmware_update_direct.py -p COM5 -f firmware.ua -v
Author: Netfeasa Ltd.
"""

import argparse
import logging
import re
import sys
import time
from pathlib import Path

try:
    import serial
except ImportError:
    print("Error: pyserial not installed...")
    sys.exit(1)

try:
    from xmodem import XMODEM1k
except ImportError:
    print("Error: xmodem not installed...")
    sys.exit(1)

try:
    from xmodem import XMODEM1k
except ImportError:
    print("Error: xmodem not installed. Install with: pip install xmodem")
    sys.exit(1)
# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)


class HL78xxFirmwareUpdater:
    """Handles HL78xx modem firmware update via XMODEM-1K protocol"""

    # XMODEM control characters
    SOH = b'\x01'  # Start of 128-byte packet
    STX = b'\x02'  # Start of 1024-byte packet
    EOT = b'\x04'  # End of transmission
    ACK = b'\x06'  # Acknowledge
    NAK = b'\x15'  # Negative acknowledge
    CAN = b'\x18'  # Cancel transmission
    C = b'C'  # XMODEM-CRC request

    # AT command response patterns
    ERROR_RESPONSE = b'ERROR'
    CME_ERROR_RESPONSE = b'+CME ERROR'

    def __init__(self, port, baudrate=115200, timeout=600):
        """
        Initialize firmware updater
        Args:
            port: Serial port connected to modem (e.g., '/dev/ttyUSB0', 'COM5')
            baudrate: Baud rate (default: 115200)
            timeout: Update timeout in seconds (default: 600)
        """
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.serial = None

    def open_port(self):
        """Open serial port connection to modem"""
        try:
            self.serial = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=5,
                write_timeout=5,
            )
            logger.info(f"Opened serial port: {self.port} @ {self.baudrate} baud")
            time.sleep(0.5)  # Allow port to stabilize
            return True
        except serial.SerialException as e:
            logger.error(f"Failed to open serial port {self.port}: {e}")
            return False

    def close_port(self):
        """Close serial port"""
        if self.serial and self.serial.is_open:
            self.serial.close()
            logger.info("Serial port closed")

    def _check_response_for_errors(self, buffer):
        """
        Check if buffer contains error responses
        Returns:
            True if error found, False otherwise
        """
        if self.ERROR_RESPONSE in buffer or self.CME_ERROR_RESPONSE in buffer:
            logger.error(f"Error response: {buffer.decode('ascii', errors='ignore')}")
            return True
        return False

    def send_at_command(self, command, expected_response='OK', timeout=5):
        """
        Send AT command directly to modem and wait for response
        Args:
            command: AT command to send (e.g., "AT+WDSD=530832")
            expected_response: Expected response string (default: 'OK')
            timeout: Response timeout in seconds
        Returns:
            Tuple (success, response_data)
        """
        logger.info(f"Sending AT command: {command}")
        # Clear input buffer and send command
        self.serial.reset_input_buffer()
        cmd_bytes = (command + '\r\n').encode('ascii')
        self.serial.write(cmd_bytes)
        self.serial.flush()
        # Wait for response
        start_time = time.time()
        buffer = b''
        while (time.time() - start_time) < timeout:
            if self.serial.in_waiting > 0:
                data = self.serial.read(self.serial.in_waiting)
                buffer += data
                # Try to decode and log
                try:
                    decoded = buffer.decode('ascii', errors='ignore')
                    if decoded.strip():
                        logger.debug(f"Received: {decoded}")
                except Exception:
                    pass
                # Check for expected response
                if expected_response.encode() in buffer:
                    logger.info(f"Received expected response: {expected_response}")
                    return True, buffer
                # Check for errors using helper method
                if self._check_response_for_errors(buffer):
                    return False, buffer
            time.sleep(0.1)
        logger.error(f"Timeout waiting for response: {expected_response}")
        return False, buffer

    def _log_xmodem_data(self, data):
        """Log received XMODEM data for debugging"""
        logger.info(f"Received {len(data)} bytes: HEX={data.hex()} ASCII={data}")
        for i, byte in enumerate(data):
            byte_char = chr(byte) if 32 <= byte < 127 else '?'
            logger.debug(f"  Byte[{i}]: 0x{byte:02x} ({byte}) = '{byte_char}'")

    def _check_xmodem_crc_ready(self, data, c_count):
        """Check if data contains XMODEM-CRC ready signal 'C' (0x43)"""
        if b'C' in data or 0x43 in data:
            c_count += data.count(b'C') + data.count(0x43)
            logger.info(f"*** XMODEM-CRC ready signal 'C' (0x43) detected! (count: {c_count}) ***")
            return True, c_count
        return False, c_count

    def _check_xmodem_nak_ready(self, data, nak_count):
        """Check if data contains XMODEM-checksum ready signal NAK (0x15)"""
        if b'\x15' in data or 0x15 in data:
            nak_count += data.count(b'\x15') + data.count(0x15)
            logger.info(
                f"*** XMODEM-checksum ready signal NAK (0x15) detected! (count: {nak_count}) ***"
            )
            logger.warning("Modem is using XMODEM-checksum mode, not XMODEM-1K CRC mode")
            logger.warning(
                "This may cause compatibility issues. Consider using XMODEM1k if supported."
            )
            return True, nak_count
        return False, nak_count

    def _log_xmodem_timeout(self, all_data):
        """Log timeout information for XMODEM ready signal"""
        logger.error("Timeout waiting for XMODEM ready signal")
        logger.error(f"Total data received during wait: {len(all_data)} bytes")
        if all_data:
            logger.error(f"All data (hex): {all_data.hex()}")
            logger.error(f"All data (ascii): {all_data}")
        else:
            logger.error("NO DATA RECEIVED AT ALL - Check serial connection!")

    def _process_xmodem_data(self, data, c_count, nak_count):
        """
        Process received data to check for XMODEM ready signals
        Returns:
            Tuple (ready, mode, c_count, nak_count) where:
                ready: True if signal found, False otherwise
                mode: 'crc', 'checksum', or None
                c_count: Updated CRC signal count
                nak_count: Updated NAK signal count
        """
        self._log_xmodem_data(data)
        # Check for XMODEM-CRC ready signal
        crc_ready, c_count = self._check_xmodem_crc_ready(data, c_count)
        if crc_ready:
            return True, 'crc', c_count, nak_count
        # Check for XMODEM-checksum ready signal
        nak_ready, nak_count = self._check_xmodem_nak_ready(data, nak_count)
        if nak_ready:
            return True, 'checksum', c_count, nak_count
        # Check for error responses
        if self._check_response_for_errors(data):
            return True, None, c_count, nak_count
        return False, None, c_count, nak_count

    def wait_for_xmodem_ready(self, timeout=30):
        """
        Wait for XMODEM ready signal from modem
        After AT+WDSD command, the modem enters XMODEM receive mode and sends:
        - 'C' (0x43) for XMODEM-CRC mode (1K packets with CRC)
        - NAK (0x15) for XMODEM-checksum mode (128-byte packets with checksum)
        Args:
            timeout: Timeout in seconds
        Returns:
            Tuple (ready, mode) where mode is 'crc' or 'checksum'
        """
        logger.info("Waiting for XMODEM ready signal from modem...")
        logger.info("Looking for 'C' (0x43) for CRC mode or NAK (0x15) for checksum mode")
        start_time = time.time()
        c_count = 0
        nak_count = 0
        all_data = b''
        while (time.time() - start_time) < timeout:
            if self.serial.in_waiting > 0:
                data = self.serial.read(self.serial.in_waiting)
                all_data += data
                # Process data using helper method
                ready, mode, c_count, nak_count = self._process_xmodem_data(
                    data, c_count, nak_count
                )
                if ready:
                    return mode is not None, mode
            else:
                # No data available, show we're still waiting
                elapsed = time.time() - start_time
                if int(elapsed) % 5 == 0 and elapsed > 0:
                    logger.debug(f"Still waiting... ({int(elapsed)}s elapsed)")
            time.sleep(0.1)
        self._log_xmodem_timeout(all_data)
        return False, None

    def xmodem_transfer(self, firmware_file):
        """
        Perform XMODEM-1K transfer of firmware file
        Args:
            firmware_file: Path to firmware file (.ua file)
        Returns:
            True on success, False on failure
        """
        logger.info(f"Starting XMODEM-1K transfer of {firmware_file}")
        try:
            with open(firmware_file, 'rb') as f:
                file_size = Path(firmware_file).stat().st_size
                logger.info(f"Firmware file size: {file_size:,} bytes")
                # Calculate expected transfer time
                # XMODEM-1K: 1024 bytes per packet, ~3-5 packets/sec typical
                estimated_time = (file_size / 1024) / 3  # Conservative estimate
                logger.info(f"Estimated transfer time: {estimated_time:.1f} seconds")

                # Create XMODEM instance with callback
                def getc(size, timeout=5):
                    """Read callback for XMODEM"""
                    return self.serial.read(size) or None

                def putc(data, timeout=5):
                    """Write callback for XMODEM"""
                    return self.serial.write(data)

                # Track transfer progress
                bytes_sent = [0]  # Use list to allow modification in callback
                last_percent = [-1]
                start_time = time.time()

                def status_callback(total_packets, success_count, error_count):
                    """Progress callback - updates every 1%"""
                    bytes_sent[0] = success_count * 1024
                    percent = int((bytes_sent[0] / file_size) * 100)
                    if percent != last_percent[0]:
                        # Calculate transfer rate and ETA
                        elapsed = time.time() - start_time
                        if elapsed > 0:
                            rate = bytes_sent[0] / elapsed  # bytes per second
                            remaining_bytes = file_size - bytes_sent[0]
                            eta = remaining_bytes / rate if rate > 0 else 0
                            # Create progress bar
                            bar_width = 30
                            filled = int(bar_width * percent / 100)
                            bar = '█' * filled + '░' * (bar_width - filled)
                            # Print on same line using \r and flush
                            msg = (
                                f"Progress: [{bar}] {percent:3d}% "
                                f"({bytes_sent[0]:,}/{file_size:,} bytes) "
                                f"| {rate / 1024:.1f} KB/s | ETA: {eta:.0f}s"
                            )
                            print(f"\r{msg}", end='', flush=True)
                        else:
                            bar_width = 30
                            filled = int(bar_width * percent / 100)
                            bar = '█' * filled + '░' * (bar_width - filled)
                            msg = (
                                f"Progress: [{bar}] {percent:3d}% "
                                f"({bytes_sent[0]:,}/{file_size:,} bytes)"
                            )
                            print(f"\r{msg}", end='', flush=True)
                        last_percent[0] = percent

                modem = XMODEM1k(getc, putc, mode='xmodem1k')
                # Perform transfer
                success = modem.send(f, callback=status_callback)
                print()  # New line after progress bar
                if success:
                    logger.info(f"XMODEM transfer completed successfully: {bytes_sent[0]:,} bytes")
                    return True
                else:
                    print()  # Ensure new line on failure too
                    logger.error("XMODEM transfer failed")
                    return False
        except FileNotFoundError:
            logger.error(f"Firmware file not found: {firmware_file}")
            return False
        except Exception as e:
            logger.error(f"XMODEM transfer error: {e}")
            return False

    def _check_for_expected_response(self, buffer, expected_strings):
        """
        Check if buffer contains any expected response
        Returns:
            (found, matching_string) tuple
        """
        for expected in expected_strings:
            if expected.encode() in buffer:
                logger.info(f"Received expected response: {expected}")
                return True, expected
        return False, None

    def wait_for_response(self, expected_strings, timeout=30):
        """
        Wait for specific response from modem
        Args:
            expected_strings: List of strings to wait for (e.g., ['OK', '+WDSI: 3'])
            timeout: Timeout in seconds
        Returns:
            Tuple (success, received_data)
        """
        logger.info(f"Waiting for response: {expected_strings}")
        start_time = time.time()
        buffer = b''
        while (time.time() - start_time) < timeout:
            if self.serial.in_waiting > 0:
                data = self.serial.read(self.serial.in_waiting)
                buffer += data
                # Try to decode and log
                try:
                    decoded = buffer.decode('ascii', errors='ignore')
                    if decoded.strip():
                        logger.debug(f"Received: {decoded}")
                except Exception:
                    pass
                # Check for expected strings using helper
                found, _ = self._check_for_expected_response(buffer, expected_strings)
                if found:
                    return True, buffer
                # Check for errors using helper
                if self._check_response_for_errors(buffer):
                    return False, buffer
            time.sleep(0.1)
        logger.error(f"Timeout waiting for response: {expected_strings}")
        return False, buffer

    def check_and_enable_wdsi(self):
        """
        Check WDSI indication range and enable maximum indications
        Returns:
            True on success, False on failure
        """
        logger.info("Checking WDSI indication support...")
        # Step 1: Query WDSI range with AT+WDSI=?
        success, response = self.send_at_command("AT+WDSI=?", expected_response='OK', timeout=5)
        if not success:
            logger.error("Failed to query WDSI range")
            return False
        # Parse response to find the range
        # Expected format: +WDSI: (range)
        try:
            decoded = response.decode('ascii', errors='ignore')
            logger.debug(f"WDSI query response: {decoded}")
            # Look for +WDSI: (min-max) pattern
            match = re.search(r'\+WDSI:\s*\((\d+)-(\d+)\)', decoded)
            if match:
                min_val = int(match.group(1))
                max_val = int(match.group(2))
                logger.info(f"WDSI range: {min_val}-{max_val}")
                # Step 2: Set WDSI to maximum value to enable all indications
                logger.info(f"Enabling WDSI indications with AT+WDSI={max_val}")
                success, response = self.send_at_command(
                    f"AT+WDSI={max_val}", expected_response='OK', timeout=5
                )
                if not success:
                    logger.error("Failed to enable WDSI indications")
                    return False
                logger.info(f"WDSI indications enabled (value: {max_val})")
                return True
            else:
                logger.warning(
                    "Could not parse WDSI range, attempting to set to default value 4095"
                )
                # Try common maximum value
                success, response = self.send_at_command(
                    "AT+WDSI=4095", expected_response='OK', timeout=5
                )
                if success:
                    logger.info("WDSI indications enabled (value: 4095)")
                    return True
                else:
                    logger.warning("Could not enable WDSI indications, continuing anyway...")
                    return True  # Continue even if this fails
        except Exception as e:
            logger.warning(f"Error parsing WDSI response: {e}, continuing anyway...")
            return True  # Continue even if this fails

    def _send_wdsd_command(self, file_size):
        """Send AT+WDSD command to initiate firmware download"""
        logger.info("\nStep 1: Sending AT+WDSD command to modem...")
        logger.info(f"Command: AT+WDSD={file_size}")
        # Clear input buffer
        self.serial.reset_input_buffer()
        # Send AT+WDSD command (don't wait for OK - modem enters XMODEM mode immediately)
        cmd_bytes = (f"AT+WDSD={file_size}" + '\r\n').encode('ascii')
        logger.info(f"Sending command bytes: {cmd_bytes}")
        bytes_written = self.serial.write(cmd_bytes)
        self.serial.flush()
        logger.info(
            f"AT+WDSD command sent ({bytes_written} bytes), modem should enter XMODEM mode..."
        )
        # Give modem a moment to process and enter XMODEM mode
        time.sleep(0.5)
        # Check if modem responded immediately
        if self.serial.in_waiting > 0:
            immediate_response = self.serial.read(self.serial.in_waiting)
            logger.info(
                f"Immediate response after AT+WDSD: "
                f"HEX={immediate_response.hex()} ASCII={immediate_response}"
            )
            time.sleep(0.1)

    def _wait_for_installation_result(self):
        """Wait for firmware installation result and return status"""
        logger.info("\nStep 6: Waiting for installation result...")
        success, response = self.wait_for_response(['+WDSI: 15', '+WDSI: 16'], timeout=self.timeout)
        if not success:
            logger.warning("Timeout or error while waiting for installation result")
        if b'+WDSI: 16' in response:
            logger.info("=" * 70)
            logger.info("FIRMWARE UPDATE SUCCESSFUL (+WDSI: 16)")
            logger.info("=" * 70)
            return True
        elif b'+WDSI: 15' in response:
            logger.error("=" * 70)
            logger.error("FIRMWARE UPDATE FAILED (+WDSI: 15)")
            logger.error("=" * 70)
            return False
        else:
            logger.warning("Installation result not received within timeout")
            return False

    def perform_firmware_update(self, firmware_file):
        """
        Perform complete firmware update sequence
        Args:
            firmware_file: Path to firmware file (.ua file)
        Returns:
            True on success, False on failure
        """
        logger.info("=" * 70)
        logger.info("HL78xx Direct Firmware Update")
        logger.info("=" * 70)
        logger.info(f"Firmware file: {firmware_file}")
        logger.info(f"Serial port: {self.port} @ {self.baudrate} baud")
        logger.info(f"Timeout: {self.timeout} seconds")
        logger.info("=" * 70)
        if not Path(firmware_file).exists():
            logger.error(f"Firmware file not found: {firmware_file}")
            return False
        file_size = Path(firmware_file).stat().st_size
        logger.info(f"Firmware file size: {file_size:,} bytes")
        # Open serial port
        if not self.open_port():
            return False
        try:
            # Step 0: Check and enable WDSI indications
            logger.info("\nStep 0: Checking and enabling WDSI indications...")
            if not self.check_and_enable_wdsi():
                logger.warning("WDSI check failed, but continuing...")
            # Step 1: Send AT+WDSD command using helper
            self._send_wdsd_command(file_size)
            # Step 2: Wait for XMODEM ready signal from modem
            logger.info("\nStep 2: Waiting for XMODEM ready signal...")
            ready, xmodem_mode = self.wait_for_xmodem_ready(timeout=30)
            if not ready:
                logger.error("Failed to receive XMODEM ready signal")
                logger.error("The modem may not have entered XMODEM mode")
                return False
            logger.info(f"Modem ready for XMODEM transfer (mode: {xmodem_mode})")
            # Step 3: Perform XMODEM-1K transfer
            logger.info("\nStep 3: Performing XMODEM-1K transfer...")
            if not self.xmodem_transfer(firmware_file):
                logger.error("XMODEM transfer failed")
                return False
            # Step 4: Wait for OK and +WDSI: 3
            logger.info("\nStep 4: Waiting for download confirmation...")
            success, response = self.wait_for_response(['OK', '+WDSI: 3'], timeout=30)
            if not success:
                logger.error("Failed to receive download confirmation")
                return False
            if b'+WDSI: 3' in response:
                logger.info("Firmware download completed successfully (+WDSI: 3)")
            else:
                logger.warning("Did not receive +WDSI: 3, but got OK")
            # Step 5: Send AT+WDSR=4 to start installation
            logger.info("\nStep 5: Starting firmware installation...")
            logger.info("Command: AT+WDSR=4")
            success, response = self.send_at_command(
                "AT+WDSR=4", expected_response='OK', timeout=10
            )
            if not success:
                logger.error("Failed to send AT+WDSR=4 command")
                return False
            logger.info("Firmware installation command sent")
            logger.info("Modem will reboot and install firmware...")
            logger.info("This may take several minutes, please wait...")
            # Wait for +WDSI: 14 (installation starting)
            success, response = self.wait_for_response(['+WDSI: 14'], timeout=60)
            if success:
                logger.info("Firmware installation started (+WDSI: 14)")
            # Step 6: Wait for installation result using helper
            return self._wait_for_installation_result()
        except KeyboardInterrupt:
            logger.warning("\nUpdate interrupted by user")
            return False
        except Exception as e:
            logger.error(f"Unexpected error: {e}")
            return False
        finally:
            self.close_port()


def main():
    """Main entry point for firmware update script"""
    parser = argparse.ArgumentParser(
        allow_abbrev=False,
        description='Sierra Wireless HL78xx Modem Firmware Update via XMODEM-1K',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Update firmware on Linux (direct modem connection)
  python hl78xx_firmware_update_direct.py -p /dev/ttyUSB0 -f HL7800.4.6.7.0.ua
  # Update firmware on Windows (direct modem connection)
  python hl78xx_firmware_update_direct.py -p COM5 -f HL7800.4.6.7.0.ua
  # Update with verbose logging
  python hl78xx_firmware_update_direct.py -p COM5 -f firmware.ua -v
  # Update with custom baud rate and timeout
  python hl78xx_firmware_update_direct.py -p COM5 -f firmware.ua -b 115200 -t 900
Requirements:
  - Python 3.6 or later
  - pyserial: pip install pyserial
  - xmodem: pip install xmodem
Notes:
  - This script connects DIRECTLY to the modem UART port
  - Ensure the modem is powered on and accessible
  - Firmware files typically have .ua extension (delta updates)
  - Installation can take 5-15 minutes depending on modem model
  - Make sure no other application is using the modem serial port
  - For updates via Zephyr AT client bridge, see samples/drivers/modem/at_client/
Firmware Files:
  Sierra Wireless provides firmware update files with .ua extension.
  These are delta update packages that upgrade from one version to another.
  Contact Sierra Wireless or your distributor for firmware files.
        """,
    )
    parser.add_argument(
        '-p',
        '--port',
        required=True,
        help='Serial port connected to modem (e.g., /dev/ttyUSB0, COM5)',
    )
    parser.add_argument('-f', '--firmware', required=True, help='Firmware file path (.ua file)')
    parser.add_argument(
        '-b', '--baudrate', type=int, default=115200, help='Baud rate (default: 115200)'
    )
    parser.add_argument(
        '-t', '--timeout', type=int, default=600, help='Update timeout in seconds (default: 600)'
    )
    parser.add_argument('-v', '--verbose', action='store_true', help='Enable verbose logging')
    args = parser.parse_args()
    # Set logging level
    if args.verbose:
        logger.setLevel(logging.DEBUG)
    # Create updater and perform update
    updater = HL78xxFirmwareUpdater(port=args.port, baudrate=args.baudrate, timeout=args.timeout)
    success = updater.perform_firmware_update(args.firmware)
    if success:
        logger.info("\nFirmware update completed successfully!")
        sys.exit(0)
    else:
        logger.error("\nFirmware update failed!")
        sys.exit(1)


if __name__ == '__main__':
    main()
