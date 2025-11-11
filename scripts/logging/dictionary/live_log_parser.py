#!/usr/bin/env python3
#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

"""
Log Parser for Dictionary-based Logging

This uses the JSON database file to decode the binary
log data taken directly from input serialport and print
the log messages.
"""

import argparse
import contextlib
import logging
import os
import select
import sys
import time

import parserlib
import serial

try:
    # Pylink is an optional dependency for RTT reading, which requires it's own installation.
    # Don't fail, unless the user tries to use RTT reading.
    import pylink
except ImportError:
    pylink = None

LOGGER_FORMAT = "%(message)s"
logger = logging.getLogger("parser")


class SerialReader:
    """Class to read data from serial port and parse it"""

    def __init__(self, serial_port, baudrate):
        self.serial_port = serial_port
        self.baudrate = baudrate
        self.serial = None

    @contextlib.contextmanager
    def open(self):
        try:
            self.serial = serial.Serial(self.serial_port, self.baudrate)
            yield
        finally:
            self.serial.close()

    def fileno(self):
        return self.serial.fileno()

    def read_non_blocking(self):
        size = self.serial.in_waiting
        return self.serial.read(size)


class FileReader:
    """Class to read data from serial port and parse it"""

    def __init__(self, filepath):
        self.filepath = filepath
        self.file = None

    @contextlib.contextmanager
    def open(self):
        if self.filepath is not None:
            with open(self.filepath, 'rb') as f:
                self.file = f
                yield
        else:
            sys.stdin = os.fdopen(sys.stdin.fileno(), 'rb', 0)
            self.file = sys.stdin
            yield

    def fileno(self):
        return self.file.fileno()

    def read_non_blocking(self):
        # Read available data using a reasonable buffer size (without buffer size, this blocks
        # forever, but with buffer size it returns even when less data than the buffer read was
        # available).
        return self.file.read(1024)


class JLinkRTTReader:
    """Class to read data from JLink's RTT"""

    @staticmethod
    def _create_jlink_connection(lib_path):
        if pylink is None:
            raise ImportError(
                "pylink module is required for RTT reading. "
                "Please install it using 'pip install pylink-square'."
            )

        if lib_path is not None:
            lib = pylink.Library(lib_path, True)
            jlink = pylink.JLink(lib)
        else:
            jlink = pylink.JLink()

        return jlink

    @contextlib.contextmanager
    def open(self):
        try:
            self.jlink.open()
            self.jlink.set_tif(pylink.enums.JLinkInterfaces.SWD)
            if self.speed != 0:
                self.jlink.connect(self.target_device, self.speed)
            else:
                self.jlink.connect(self.target_device)

            self.jlink.rtt_start(self.block_address)

            # Wait for the JLINK RTT buffers to be initialized.
            up_down_initialized = False
            while not up_down_initialized:
                try:
                    _ = self.jlink.rtt_get_num_up_buffers()
                    _ = self.jlink.rtt_get_num_down_buffers()
                    up_down_initialized = True
                except pylink.errors.JLinkRTTException:
                    time.sleep(0.1)

            yield

        finally:
            self.close()

    def __init__(self, target_device, block_address, channel, speed, lib_path):
        self.target_device = target_device
        self.block_address = block_address
        self.speed = speed
        self.channel = channel

        self.jlink = self._create_jlink_connection(lib_path)

    def close(self):
        # JLink closes the connection through the __del__ method.
        del self.jlink

    def read_non_blocking(self):
        return bytes(self.jlink.rtt_read(self.channel, 1024))


def parse_args():
    """Parse command line arguments"""
    parser = argparse.ArgumentParser(allow_abbrev=False)

    parser.add_argument("dbfile", help="Dictionary Logging Database file")
    parser.add_argument("--debug", action="store_true", help="Print extra debugging information")
    parser.add_argument(
        "--polling-interval",
        type=float,
        default=0.1,
        help="Interval for polling input source, if it does not support 'select'",
    )

    # Create subparsers for different input modes
    subparsers = parser.add_subparsers(dest="mode", required=True, help="Input source mode")

    # Serial subparser
    serial_parser = subparsers.add_parser("serial", help="Read from serial port")
    serial_parser.add_argument("port", help="Serial port")
    serial_parser.add_argument("baudrate", type=int, help="Baudrate")

    # File subparser
    file_parser = subparsers.add_parser("file", help="Read from file")
    file_parser.add_argument(
        "filepath", nargs="?", default=None, help="Input file path, leave empty for stdin"
    )

    # RTT subparser
    jlink_rtt_parser = subparsers.add_parser("jlink-rtt", help="Read from RTT")
    jlink_rtt_parser.add_argument(
        "target_device", help="Device Name (see https://www.segger.com/supported-devices/jlink/)"
    )
    jlink_rtt_parser.add_argument(
        "--block-address", help="RTT block address in hex", type=lambda x: int(x, 16)
    )
    jlink_rtt_parser.add_argument("--channel", type=int, help="RTT channel number", default=0)
    jlink_rtt_parser.add_argument("--speed", type=int, help="Reading speed", default='0')
    jlink_rtt_parser.add_argument("--lib-path", help="Path to libjlinkarm.so library")

    return parser.parse_args()


def main():
    """function of serial parser"""
    args = parse_args()

    if args.dbfile is None or '.json' not in args.dbfile:
        logger.error("ERROR: invalid log database path: %s, exiting...", args.dbfile)
        sys.exit(1)

    logging.basicConfig(format=LOGGER_FORMAT)

    if args.debug:
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

    log_parser = parserlib.get_log_parser(args.dbfile, logger)

    data = b''

    if args.mode == "serial":
        reader = SerialReader(args.port, args.baudrate)
    elif args.mode == "file":
        reader = FileReader(args.filepath)
    elif args.mode == "jlink-rtt":
        reader = JLinkRTTReader(
            args.target_device, args.block_address, args.channel, args.speed, args.lib_path
        )
    else:
        raise ValueError("Invalid mode selected. Use 'serial' or 'file'.")

    with reader.open():
        while True:
            if hasattr(reader, 'fileno'):
                _, _, _ = select.select([reader], [], [])
            else:
                time.sleep(args.polling_interval)
            data += reader.read_non_blocking()
            parsed_data_offset = parserlib.parser(data, log_parser, logger)
            data = data[parsed_data_offset:]


if __name__ == "__main__":
    main()
