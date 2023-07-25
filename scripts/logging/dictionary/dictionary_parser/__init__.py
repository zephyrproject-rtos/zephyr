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

    # DB version 1 and 2 correspond to v1 parser
    if db_ver in [1, 2]:
        return LogParserV1(database)

    return None
