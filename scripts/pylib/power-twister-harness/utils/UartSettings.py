# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import os
import re
import sys
from dataclasses import dataclass
from enum import Enum


class Parity(Enum):
    """
    Enum for UART parity settings.
    - NONE: No parity bit
    - EVEN: Even parity
    - ODD: Odd parity bit
    - MARK: Mark parity bit (always set to 1)
    - SPACE: Space parity bit (always set to 0)
    """

    NONE = "N"
    EVEN = "E"
    ODD = "O"
    MARK = "M"
    SPACE = "S"


class FlowControl(Enum):
    """
    Enum for UART flow control settings.
    - NONE: No flow control
    - RTS_CTS: RTS/CTS hardware flow control
    - XON_XOFF: Software flow control using XON/XOFF
    - DSR_DTR: DSR/DTR hardware flow control
    """

    NONE = "None"
    RTS_CTS = "RTS/CTS"
    XON_XOFF = "XON/XOFF"
    DSR_DTR = "DSR/DTR"


@dataclass
class UARTSettings:
    """Dataclass for UART settings.
    Attributes:
        port (str): Serial port name (e.g., COM3, /dev/ttyUSB0)
        baudrate (int): Baud rate for communication
        databits (int): Number of data bits (5-8)
        parity (Parity): Parity setting (NONE, EVEN, ODD, MARK, SPACE)
        stopbits (int | float): Number of stop bits (1, 1.5, or 2)
        flowcontrol (FlowControl): Flow control setting (NONE, RTS/CTS, XON/XOFF, DSR/DTR)
    """

    port: str
    baudrate: int
    databits: int
    parity: Parity
    stopbits: int | float
    flowcontrol: FlowControl

    def __str__(self):
        return f"UART({self.port}, {self.baudrate}bps, \
            {self.databits}{self.parity.value}{self.stopbits}, {self.flowcontrol.value})"


class UARTParser:
    """
    Parser for UART configuration strings.
    This class provides methods to parse a UART configuration string
    and validate the port name.
    """

    # Multiple patterns to handle different formats
    UART_PATTERN_COLON = re.compile(
        r'^([^:,]+)(?::(\d+))?(?::([5-8]))?(?::([NEOMS]))?(?::([12](?:\.5)?))?(?::(.+))?$',
        re.IGNORECASE,
    )

    # Pattern for comma-separated format like "COM3:115200,8-N-1,None"
    UART_PATTERN_COMMA = re.compile(
        r'^([^:,]+)(?::(\d+))?(?:,([5-8])-([NEOMS])-([12](?:\.5)?))?(?:,(.+))?$', re.IGNORECASE
    )

    # Pattern for mixed comma format like "/dev/ttyACM0,38400,8-N-1.5,None"
    UART_PATTERN_MIXED = re.compile(
        r'^([^:,]+)(?:[,:](\d+))?(?:[,:]([5-8])-([NEOMS])-([12](?:\.5)?))?(?:[,:](.+))?$',
        re.IGNORECASE,
    )

    VALID_BAUDRATES = {
        110,
        300,
        600,
        1200,
        2400,
        4800,
        9600,
        14400,
        19200,
        38400,
        57600,
        115200,
        128000,
        256000,
        460800,
        921600,
    }

    @classmethod
    def parse(cls, uart_string: str) -> UARTSettings | None:
        """
        Parse a UART configuration string into UARTSettings object.
        Tries to parse as much as possible, using defaults for invalid values.

        Supports formats:
        - "PORT"
        - "PORT:BAUDRATE"
        - "PORT:BAUDRATE:DATABITS:PARITY:STOPBITS:FLOWCONTROL"
        - "PORT:BAUDRATE,DATABITS-PARITY-STOPBITS,FLOWCONTROL"
        - "PORT,BAUDRATE,DATABITS-PARITY-STOPBITS,FLOWCONTROL"

        Args:
            uart_string: UART configuration string

        Returns:
            UARTSettings object with parsed or default values

        Raises:
            ValueError: Only if the input is completely invalid (empty/None)
        """
        if not uart_string or not isinstance(uart_string, str):
            raise ValueError("Input must be a non-empty string")

        # Default values
        defaults = {
            'port': 'COM1',
            'baudrate': 115200,
            'databits': 8,
            'parity': Parity.NONE,
            'stopbits': 1,
            'flowcontrol': FlowControl.NONE,
        }

        uart_string = uart_string.strip()
        port = None
        baudrate_str = None
        databits_str = None
        parity_str = None
        stopbits_str = None
        flowcontrol_str = None

        # Try different patterns in order of specificity
        patterns = [cls.UART_PATTERN_MIXED, cls.UART_PATTERN_COMMA, cls.UART_PATTERN_COLON]

        for pattern in patterns:
            match = pattern.match(uart_string)
            if match:
                port, baudrate_str, databits_str, parity_str, stopbits_str, flowcontrol_str = (
                    match.groups()
                )
                break

        if not match:
            # Fallback: manual parsing with flexible separators
            # Replace commas with colons for uniform parsing
            normalized = uart_string.replace(',', ':')
            parts = normalized.split(':')

            if len(parts) < 1 or not parts[0].strip():
                raise ValueError("At least port must be specified")

            port = parts[0] if len(parts) > 0 else None
            baudrate_str = parts[1] if len(parts) > 1 else None

            # Handle third part which might be "8-N-1" format or just databits
            if len(parts) > 2 and parts[2]:
                third_part = parts[2]
                if '-' in third_part:
                    # Parse "8-N-1" format
                    dash_parts = third_part.split('-')
                    if len(dash_parts) >= 3:
                        databits_str = dash_parts[0]
                        parity_str = dash_parts[1]
                        stopbits_str = dash_parts[2]
                else:
                    databits_str = third_part
                    parity_str = parts[3] if len(parts) > 3 else None
                    stopbits_str = parts[4] if len(parts) > 4 else None

            flowcontrol_str = (
                parts[-1]
                if len(parts) > 3 and databits_str
                else (parts[3] if len(parts) > 3 else None)
            )

        # Parse port (required)
        if not port or not port.strip():
            raise ValueError("Port cannot be empty")
        port = port.strip()

        # Parse baudrate with fallback
        try:
            if baudrate_str:
                baudrate = int(baudrate_str)
                if baudrate not in cls.VALID_BAUDRATES:
                    # Find closest valid baudrate
                    baudrate = min(cls.VALID_BAUDRATES, key=lambda x: abs(x - baudrate))
            else:
                baudrate = defaults['baudrate']
        except (ValueError, TypeError):
            baudrate = defaults['baudrate']

        # Parse databits with validation
        try:
            if databits_str:
                databits = int(databits_str)
                if databits not in range(5, 9):  # 5-8 are valid
                    databits = defaults['databits']
            else:
                databits = defaults['databits']
        except (ValueError, TypeError):
            databits = defaults['databits']

        # Parse parity with fallback
        try:
            if parity_str:
                parity = Parity(parity_str.upper())
            else:
                parity = defaults['parity']
        except (ValueError, AttributeError):
            parity = defaults['parity']

        # Parse stopbits with validation
        try:
            if stopbits_str:
                if stopbits_str == "1.5":
                    stopbits = 1.5
                else:
                    stopbits = int(stopbits_str)
                    if stopbits not in [1, 2]:
                        stopbits = defaults['stopbits']
            else:
                stopbits = defaults['stopbits']
        except (ValueError, TypeError):
            stopbits = defaults['stopbits']

        # Parse flow control with fallback and flexible matching
        try:
            if flowcontrol_str:
                fc_upper = flowcontrol_str.upper().strip()
                flowcontrol = None

                # Direct match first
                for fc in FlowControl:
                    if fc.value.upper() == fc_upper:
                        flowcontrol = fc
                        break

                # Partial match for common abbreviations
                if not flowcontrol:
                    if fc_upper in ['RTS/CTS', 'RTSCTS', 'RTS', 'CTS', 'HARDWARE']:
                        flowcontrol = FlowControl.RTS_CTS
                    elif fc_upper in ['XON/XOFF', 'XONXOFF', 'XON', 'XOFF', 'SOFTWARE']:
                        flowcontrol = FlowControl.XON_XOFF
                    elif fc_upper in ['DSR/DTR', 'DSRDTR', 'DSR', 'DTR']:
                        flowcontrol = FlowControl.DSR_DTR
                    elif fc_upper in ['NONE', 'N', 'NO', 'OFF']:
                        flowcontrol = FlowControl.NONE
                    else:
                        flowcontrol = defaults['flowcontrol']
            else:
                flowcontrol = defaults['flowcontrol']
        except (ValueError, AttributeError):
            flowcontrol = defaults['flowcontrol']

        return UARTSettings(
            port=port,
            baudrate=baudrate,
            databits=databits,
            parity=parity,
            stopbits=stopbits,
            flowcontrol=flowcontrol,
        )

    @classmethod
    def validate_port_name(cls, port: str) -> bool:
        """Validate if port name follows common patterns"""
        # Windows COM ports
        if re.match(r'^COM\d+$', port, re.IGNORECASE):
            return True
        # Linux/Unix serial ports
        if re.match(r'^/dev/tty(USB|ACM|S)\d+$', port):
            return True
        # Generic validation - allow any non-empty string
        return bool(port.strip())


def main():
    """Example usage and testing"""

    # Set UTF-8 encoding for stdout/stderr
    os.environ['PYTHONIOENCODING'] = 'utf-8'

    # For Python 3.7+, you can also reconfigure stdout/stderr
    if hasattr(sys.stdout, 'reconfigure'):
        sys.stdout.reconfigure(encoding='utf-8')
        sys.stderr.reconfigure(encoding='utf-8')

    test_strings = [
        "COM3",
        "COM3:115200",
        "COM3:115200,8-N-1,None",
        "/dev/ttyUSB0",
        "/dev/ttyUSB0:9600",
        "/dev/ttyUSB0:9600,8-E-1,RTS/CTS",
        "COM1:57600,7-O-2,XON/XOFF",
        "/dev/ttyACM0,38400,8-N-1.5,None",
        "COM10:921600,8-N-1,DSR/DTR",
    ]

    parser = UARTParser()

    print("UART Configuration Parser: (Enter to quit)")
    print("=" * 50)

    for uart_str in test_strings:
        try:
            settings = parser.parse(uart_str)
            print(f"✓ Input:  {uart_str}")
            print(f"  Output: {settings}")
            print(f"  Valid:  {parser.validate_port_name(settings.port)}")
            print()
        except ValueError as e:
            print(f"✗ Input:  {uart_str}")
            print(f"  Error:  {e}")
            print()

    # Interactive mode
    print("\nInteractive Mode (press Enter with empty input to exit):")
    while True:
        try:
            user_input = input("Enter UART string: ").strip()
            if not user_input:
                break

            settings = parser.parse(user_input)
            print(f"Parsed: {settings}")

        except ValueError as e:
            print(f"Error: {e}")
        except KeyboardInterrupt:
            break

    print("Goodbye!")


if __name__ == "__main__":
    main()
