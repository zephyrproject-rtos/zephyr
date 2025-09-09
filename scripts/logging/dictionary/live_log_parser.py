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

import parserlib
import serial

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


def parse_args():
    """Parse command line arguments"""
    parser = argparse.ArgumentParser(allow_abbrev=False)

    parser.add_argument("dbfile", help="Dictionary Logging Database file")
    parser.add_argument("--debug", action="store_true", help="Print extra debugging information")

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
    else:
        raise ValueError("Invalid mode selected. Use 'serial' or 'file'.")

    with reader.open():
        while True:
            ready, _, _ = select.select([reader], [], [])
            if ready:
                data += reader.read_non_blocking()
                parsed_data_offset = parserlib.parser(data, log_parser, logger)
                data = data[parsed_data_offset:]


if __name__ == "__main__":
    main()
