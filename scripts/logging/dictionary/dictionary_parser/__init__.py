#!/usr/bin/env python3
#
# Copyright (c) 2021 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""
Dictionary-based Logging Parser Module
"""

from .log_parser_v1 import LogParserV1


def get_parser(database):
    """Get the parser object based on database"""
    db_ver = int(database.get_version())

    if db_ver == 1:
        return LogParserV1(database)

    return None
