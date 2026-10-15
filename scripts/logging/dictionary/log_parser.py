#!/usr/bin/env python3
#
# Copyright (c) 2021 Intel Corporation
# Copyright (c) 2024 Nordic Semiconductor ASA
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
import parserlib

LOGGER_FORMAT = "%(message)s"
logger = logging.getLogger("parser")

LOG_HEX_SEP = "##ZLOGV1##"


def parse_args():
    """Parse command line arguments"""
    argparser = argparse.ArgumentParser(
        allow_abbrev=False,
        description=(
            "Decode dictionary logs from a file. Pass one or more database JSON files, "
            "then the log data file."
        ),
    )
    argparser.add_argument(
        "--hex", action="store_true", help="Log Data file is in hexadecimal strings"
    )
    argparser.add_argument(
        "--rawhex", action="store_true", help="Log file only contains hexadecimal log data"
    )
    argparser.add_argument("--debug", action="store_true", help="Print extra debugging information")
    parserlib.add_sorting_arguments(argparser)

    args, remainder = argparser.parse_known_args()
    parserlib.check_sorting_arguments(argparser, args)
    if len(remainder) < 2:
        argparser.error(
            "at least one dbfile and logfile are required "
            "(e.g. app/log_dictionary.json remote/log_dictionary.json capture.bin)"
        )

    args.logfile = remainder[-1]
    args.dbfile = parserlib.normalize_dbfiles(remainder[:-1])
    return args


def read_log_file(args):
    """
    Read the log from file
    """
    logdata = None
    hexdata = ''

    # Open log data file for reading
    if args.hex:
        if args.rawhex:
            # Simply log file with only hexadecimal data
            logdata = dictionary_parser.utils.convert_hex_file_to_bin(args.logfile)
        else:
            hexdata = ''

            with open(args.logfile, encoding="iso-8859-1") as hexfile:
                for line in hexfile.readlines():
                    hexdata += line.strip()

            found, hexdata = parserlib.skip_to_sync(hexdata, LOG_HEX_SEP)
            if not found:
                logger.error("ERROR: Cannot find start of log data, exiting...")
                sys.exit(1)

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
        with open(args.logfile, "rb") as logfile:
            if not logfile:
                logger.error(f"ERROR: Cannot open binary log data file: {args.logfile}, exiting...")
                sys.exit(1)
            logdata = logfile.read()

        # RTT logs add header information to the logdata, the actual log comes
        # after newline following "Process:" line in logdata
        if b"Process:" in logdata:
            process_idx = logdata.find(b"Process:")
            newline_idx = logdata.find(b"\n", process_idx)
            if newline_idx != -1:
                # Keep only the data after this newline
                logdata = logdata[newline_idx + 1 :]
                logger.debug("Found 'Process:' in the RTT header, trimmed data")

        # Binary log data must contain the synchronization sequence (the equivalent of
        # the hexadecimal separator in the hex mode). Data before it is discarded.
        found, logdata = parserlib.skip_to_sync(logdata)
        if not found:
            logger.error("ERROR: Cannot find synchronization sequence, exiting...")
            sys.exit(1)

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

    log_parser = parserlib.get_log_parser(
        args.dbfile, logger, latency_ms=args.latency, ts_freq=args.ts_freq
    )

    logdata = read_log_file(args)
    if logdata is None:
        logger.error("ERROR: cannot read log from file: %s, exiting...", args.logfile)
        sys.exit(1)

    parsed_data_offset, error = parserlib.parser(logdata, log_parser, logger)

    # All data was read, print messages which are still cached for sorting.
    log_parser.flush()

    if error is not None:
        logger.error("ERROR: Failed to process log data: %s", error)
        sys.exit(1)

    if parsed_data_offset != len(logdata):
        logger.error(
            'ERROR: Not all data was parsed, %d bytes left unparsed',
            len(logdata) - parsed_data_offset,
        )
        sys.exit(1)


if __name__ == "__main__":
    main()
