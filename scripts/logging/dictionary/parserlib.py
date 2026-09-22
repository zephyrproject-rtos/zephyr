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


def get_log_parser(dbfile, logger):
    """Get the log parser for the given database.

    In addition to creating the parser, the function prints general information about the parser.
    """
    database = LogDatabase.read_json_database(dbfile)

    if database is None:
        logger.error("ERROR: Cannot open database file:  exiting...")
        raise ValueError(f"Cannot open database file: {dbfile}")
    log_parser = dictionary_parser.get_parser(database)

    if log_parser is not None:
        logger.debug("# Build ID: %s", database.get_build_id())
        logger.debug("# Target: %s, %d-bit", database.get_arch(), database.get_tgt_bits())
        if database.is_tgt_little_endian():
            logger.debug("# Endianness: Little")
        else:
            logger.debug("# Endianness: Big")
    else:
        logger.error("ERROR: Cannot find a suitable parser matching database version!")
        raise ValueError("Cannot create parser.")

    return log_parser


def parser(logdata, log_parser, logger):
    """function of serial parser"""

    if not isinstance(logger, logging.Logger):
        raise ValueError("Invalid logger instance. Please configure the logger!")

    if logdata is None:
        logger.error("ERROR: cannot read log from file:  exiting...")
        raise ValueError("Cannot read log data.")

    ret = log_parser.parse_log_data(logdata)
    return ret
