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
import sys

import live_log_parser


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

    print("This script is deprecated. Use 'live_log_parser.py' instead.",
          file=sys.stderr)

    # Convert the arguments to the format expected by live_log_parser, and invoke it directly.
    args = parse_args()

    sys.argv = [
        'live_log_parser.py',
        args.dbfile,
    ]
    if args.debug:
        sys.argv.append('--debug')

    sys.argv += [
        'serial',
        args.serialPort,
        args.baudrate
    ]

    live_log_parser.main()


if __name__ == "__main__":
    main()
