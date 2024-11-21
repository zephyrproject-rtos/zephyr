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

import sys
import logging

import dictionary_parser
from dictionary_parser.log_database import LogDatabase


def parser(logdata, dbfile, logger):
    """function of serial parser"""
    # Read from database file
    database = LogDatabase.read_json_database(dbfile)

    if not isinstance(logger, logging.Logger):
        raise ValueError("Invalid logger instance. Please configure the logger!")

    if database is None:
        logger.error("ERROR: Cannot open database file:  exiting...")
        sys.exit(1)

    if logdata is None:
        logger.error("ERROR: cannot read log from file:  exiting...")
        sys.exit(1)

    log_parser = dictionary_parser.get_parser(database)
    if log_parser is not None:
        logger.debug("# Build ID: %s", database.get_build_id())
        logger.debug("# Target: %s, %d-bit", database.get_arch(), database.get_tgt_bits())
        if database.is_tgt_little_endian():
            logger.debug("# Endianness: Little")
        else:
            logger.debug("# Endianness: Big")

        ret = log_parser.parse_log_data(logdata)
        if not ret:
            logger.error("ERROR: there were error(s) parsing log data")
            sys.exit(1)
    else:
        logger.error("ERROR: Cannot find a suitable parser matching database version!")
