#!/usr/bin/env python3
#
# Copyright (c) 2021 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""
Log Parser for Dictionary-based Logging

This uses the JSON database file to decode the input binary
log data and print the log messages.
"""

import argparse
import binascii
import logging
import sys

import dictionary_parser
from dictionary_parser.log_database import LogDatabase


LOGGER_FORMAT = "%(message)s"
logger = logging.getLogger("parser")

LOG_HEX_SEP = "##ZLOGV1##"


def parse_args():
    """Parse command line arguments"""
    argparser = argparse.ArgumentParser(allow_abbrev=False)

    argparser.add_argument("dbfile", help="Dictionary Logging Database file")
    argparser.add_argument("logfile", help="Log Data file")
    argparser.add_argument("--hex", action="store_true",
                           help="Log Data file is in hexadecimal strings")
    argparser.add_argument("--rawhex", action="store_true",
                           help="Log file only contains hexadecimal log data")
    argparser.add_argument("--debug", action="store_true",
                           help="Print extra debugging information")

    return argparser.parse_args()


def read_log_file(args):
    """
    Read the log from file
    """
    logdata = None

    # Open log data file for reading
    if args.hex:
        if args.rawhex:
            # Simply log file with only hexadecimal data
            logdata = dictionary_parser.utils.convert_hex_file_to_bin(args.logfile)
        else:
            hexdata = ''

            with open(args.logfile, "r", encoding="iso-8859-1") as hexfile:
                for line in hexfile.readlines():
                    hexdata += line.strip()

            if LOG_HEX_SEP not in hexdata:
                logger.error("ERROR: Cannot find start of log data, exiting...")
                sys.exit(1)

            idx = hexdata.index(LOG_HEX_SEP) + len(LOG_HEX_SEP)
            hexdata = hexdata[idx:]

            if len(hexdata) % 2 != 0:
                # Make sure there are even number of characters
                idx = int(len(hexdata) / 2) * 2
                hexdata = hexdata[:idx]

            idx = 0
            while idx < len(hexdata):
                # When running QEMU via west or ninja, there may be additional
                # strings printed by QEMU, west or ninja (for example, QEMU
                # is terminated, or user interrupted, etc). So we need to
                # figure out where the end of log data stream by
                # trying to convert from hex to bin.
                idx += 2

                try:
                    binascii.unhexlify(hexdata[:idx])
                except binascii.Error:
                    idx -= 2
                    break

            logdata = binascii.unhexlify(hexdata[:idx])
    else:
        logfile = open(args.logfile, "rb")
        if not logfile:
            logger.error("ERROR: Cannot open binary log data file: %s, exiting...", args.logfile)
            sys.exit(1)

        logdata = logfile.read()

        logfile.close()

    return logdata


def main():
    """Main function of log parser"""
    args = parse_args()

    # Setup logging for parser
    logging.basicConfig(format=LOGGER_FORMAT)
    if args.debug:
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

    # Read from database file
    database = LogDatabase.read_json_database(args.dbfile)
    if database is None:
        logger.error("ERROR: Cannot open database file: %s, exiting...", args.dbfile)
        sys.exit(1)

    logdata = read_log_file(args)
    if logdata is None:
        logger.error("ERROR: cannot read log from file: %s, exiting...", args.logfile)
        sys.exit(1)

    log_parser = dictionary_parser.get_parser(database)
    if log_parser is not None:
        logger.debug("# Build ID: %s", database.get_build_id())
        logger.debug("# Target: %s, %d-bit", database.get_arch(), database.get_tgt_bits())
        if database.is_tgt_little_endian():
            logger.debug("# Endianness: Little")
        else:
            logger.debug("# Endianness: Big")

        logger.debug("# Database version: %d", database.get_version())

        ret = log_parser.parse_log_data(logdata, debug=args.debug)
        if not ret:
            logger.error("ERROR: there were error(s) parsing log data")
            sys.exit(1)
    else:
        logger.error("ERROR: Cannot find a suitable parser matching database version!")
        sys.exit(1)


if __name__ == "__main__":
    main()
