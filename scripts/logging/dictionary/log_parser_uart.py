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
import logging
import sys
import time

import parserlib
import serial

LOGGER_FORMAT = "%(message)s"
logger = logging.getLogger("parser")

def parse_args():
    """Parse command line arguments"""
    argparser = argparse.ArgumentParser(allow_abbrev=False)

    argparser.add_argument("dbfile", help="Dictionary Logging Database file")
    argparser.add_argument("serialPort", help="Port where the logs are generated")
    argparser.add_argument("baudrate", help="Serial Port baud rate")
    argparser.add_argument("--debug", action="store_true",
                           help="Print extra debugging information")

    return argparser.parse_args()

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

    # Parse the log every second from serial port
    with serial.Serial(args.serialPort, args.baudrate) as ser:
        ser.timeout = 2
        while True:
            size = ser.inWaiting()
            if size:
                data = ser.read(size)
                parserlib.parser(data, args.dbfile, logger)
            time.sleep(1)

if __name__ == "__main__":
    main()
