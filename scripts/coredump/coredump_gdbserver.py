#!/usr/bin/env python3
#
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import logging
import os
import socket
import sys

from coredump_parser.log_parser import CoredumpLogFile
from coredump_parser.elf_parser import CoredumpElfFile

import gdbstubs

LOGGING_FORMAT = "[%(levelname)s][%(name)s] %(message)s"

# Only bind to local host
GDBSERVER_HOST = ""


def parse_args():
    parser = argparse.ArgumentParser(allow_abbrev=False)

    parser.add_argument("elffile", help="Zephyr ELF binary")
    parser.add_argument("logfile", help="Coredump binary log file")
    parser.add_argument("--debug", action="store_true",
                        help="Print extra debugging information")
    parser.add_argument("--port", type=int, default=1234,
                        help="GDB server port")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="Print more information")

    return parser.parse_args()


def main():
    args = parse_args()

    # Setup logging
    logging.basicConfig(format=LOGGING_FORMAT)

    # Setup logging for "parser"
    logger = logging.getLogger("parser")
    if args.debug:
        logger.setLevel(logging.DEBUG)
    elif args.verbose:
        logger.setLevel(logging.INFO)
    else:
        logger.setLevel(logging.WARNING)

    # Setup logging for follow code
    logger = logging.getLogger("gdbserver")
    if args.debug:
        logger.setLevel(logging.DEBUG)
    else:
        # Use INFO as default since we need to let user
        # know what is going on
        logger.setLevel(logging.INFO)

    # Setup logging for "gdbstub"
    logger = logging.getLogger("gdbstub")
    if args.debug:
        logger.setLevel(logging.DEBUG)
    elif args.verbose:
        logger.setLevel(logging.INFO)
    else:
        logger.setLevel(logging.WARNING)

    if not os.path.isfile(args.elffile):
        logger.error(f"Cannot find file {args.elffile}, exiting...")
        sys.exit(1)

    if not os.path.isfile(args.logfile):
        logger.error(f"Cannot find file {args.logfile}, exiting...")
        sys.exit(1)

    logger.info(f"Log file: {args.logfile}")
    logger.info(f"ELF file: {args.elffile}")

    # Parse the coredump binary log file
    logf = CoredumpLogFile(args.logfile)
    logf.open()
    if not logf.parse():
        logger.error("Cannot parse log file, exiting...")
        logf.close()
        sys.exit(1)

    # Parse ELF file for code and read-only data
    elff = CoredumpElfFile(args.elffile)
    elff.open()
    if not elff.parse():
        logger.error("Cannot parse ELF file, exiting...")
        elff.close()
        logf.close()
        sys.exit(1)

    gdbstub = gdbstubs.get_gdbstub(logf, elff)

    # Start a GDB server
    gdbserver = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # Reuse address so we don't have to wait for socket to be
    # close before we can bind to the port again
    gdbserver.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    gdbserver.bind((GDBSERVER_HOST, args.port))
    gdbserver.listen(1)

    logger.info(f"Waiting GDB connection on port {args.port}...")

    conn, remote = gdbserver.accept()

    if conn:
        logger.info(f"Accepted GDB connection from {remote}")

        gdbstub.run(conn)

        conn.close()

    gdbserver.close()

    logger.info("GDB session finished.")

    elff.close()
    logf.close()


if __name__ == "__main__":
    main()
