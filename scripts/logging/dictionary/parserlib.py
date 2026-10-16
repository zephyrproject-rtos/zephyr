#!/usr/bin/env python3
#
# Copyright (c) 2021 Intel Corporation
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

"""
Parser library for Dictionary-based Logging

This library along with dictionary_parser converts the
input binary data to the log using log database.
"""

import logging

import dictionary_parser
from dictionary_parser.log_database import LogDatabase


DB_FILES_SEPARATOR = ";"

# Synchronization sequence sent at the beginning of the stream by the dictionary UART
SYNC_SEQUENCE = bytes([0x44, 0x9C, 0x64, 0xCF, 0x84, 0x1C, 0xFB, 0x40, 0x73, 0x2F])


def skip_to_sync(data, sync=SYNC_SEQUENCE):
    """Look for the synchronization sequence in the data.

    ``data`` and ``sync`` must be of the same type (``bytes`` or ``str``).

    Returns a tuple ``(found, remaining)``. If the sequence is found, ``found`` is True and
    ``remaining`` is the data following the sequence (everything before it, including the
    sequence itself, is discarded). If it is not found, ``found`` is False and ``remaining``
    contains only the trailing bytes which may be the beginning of a sequence split across
    reads; all other (garbage) bytes are discarded.
    """
    idx = data.find(sync)
    if idx >= 0:
        return True, data[idx + len(sync) :]

    # Keep the longest suffix of data which is a prefix of the sync sequence.
    for keep in range(min(len(sync) - 1, len(data)), 0, -1):
        if data.endswith(sync[:keep]):
            return False, data[-keep:]

    return False, data[:0]


def normalize_dbfiles(dbfile):
    """Return a list of database file paths from a string or sequence."""
    if isinstance(dbfile, str):
        items = [dbfile]
    else:
        items = list(dbfile)
    dbfiles = []
    for item in items:
        for part in str(item).split(DB_FILES_SEPARATOR):
            part = part.strip()
            if part:
                dbfiles.append(part)
    return dbfiles


def add_sorting_arguments(argparser):
    """Add arguments controlling sorting of the log messages to an argument parser."""
    argparser.add_argument(
        "--latency",
        type=float,
        default=0,
        metavar="MS",
        help=(
            "Time in milliseconds for which parsed messages are cached and sorted by timestamp. "
            "Messages from a remote domain may be delayed (e.g. by IPC) so they can appear in "
            "the stream out of order. A message is printed when a message newer by more than "
            "this time was received. If no new data arrives for this time, all cached messages "
            "are printed. Requires --ts_freq. Disabled (messages are printed immediately) "
            "by default."
        ),
    )
    argparser.add_argument(
        "--ts_freq",
        type=float,
        default=None,
        metavar="HZ",
        help="Frequency of the log timestamp in Hz. Required when --latency is used.",
    )


def check_sorting_arguments(argparser, args):
    """Validate sorting arguments, report error using the argument parser."""
    if args.latency < 0:
        argparser.error("--latency must not be negative")
    if args.ts_freq is not None and args.ts_freq <= 0:
        argparser.error("--ts_freq must be greater than 0")
    if args.latency > 0 and args.ts_freq is None:
        argparser.error("--ts_freq is required when --latency is used")


def get_log_parser(dbfile, logger, latency_ms=0, ts_freq=None):
    """Get the log parser for the given database(s).

    ``dbfile`` is one path, a sequence of paths, or paths separated with ``;`` in one string.
    The first database is the primary one (domain 0). If more than one database is given, then
    for messages from other domains a database is selected by looking for the message string
    in each of them and source names are prefixed with ``cpu<domain_id>.``.

    If ``latency_ms`` is greater than 0, parsed messages are cached for that time and printed
    sorted by timestamp (``ts_freq``, in Hz, is the timestamp frequency). Call ``flush()`` on
    the returned parser when there is no more data.

    In addition to creating the parser, the function prints general information about the parser.
    """
    dbfiles = normalize_dbfiles(dbfile)
    if not dbfiles:
        logger.error("ERROR: No database file provided: exiting...")
        raise ValueError("No database file provided.")

    databases = []
    for one_file in dbfiles:
        database = LogDatabase.read_json_database(one_file)

        if database is None:
            logger.error("ERROR: Cannot open database file: %s, exiting...", one_file)
            raise ValueError(f"Cannot open database file: {one_file}")
        databases.append(database)

    log_parser = dictionary_parser.get_parser(databases[0], databases[1:])

    if log_parser is not None:
        for idx, database in enumerate(databases):
            logger.debug("# Database %d: %s", idx, dbfiles[idx])
            logger.debug("# Build ID: %s", database.get_build_id())
            logger.debug("# Target: %s, %d-bit", database.get_arch(), database.get_tgt_bits())
            if database.is_tgt_little_endian():
                logger.debug("# Endianness: Little")
            else:
                logger.debug("# Endianness: Big")
    else:
        logger.error("ERROR: Cannot find a suitable parser matching database version!")
        raise ValueError("Cannot create parser.")

    if latency_ms and latency_ms > 0:
        if not ts_freq:
            raise ValueError("Timestamp frequency is required to sort messages.")
        log_parser.enable_sorting(latency_ms, ts_freq)

    return log_parser


def parser(logdata, log_parser, logger):
    """function of serial parser"""

    if not isinstance(logger, logging.Logger):
        raise ValueError("Invalid logger instance. Please configure the logger!")

    if logdata is None:
        logger.error("ERROR: cannot read log from file:  exiting...")
        raise ValueError("Cannot read log data.")

    logger.debug("parser: logdata: %d", len(logdata))
    ret = log_parser.parse_log_data(logdata)
    return ret
