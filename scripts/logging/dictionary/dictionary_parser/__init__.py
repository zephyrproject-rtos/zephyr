#!/usr/bin/env python3
#
# Copyright (c) 2021 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""
Dictionary-based Logging Parser Module
"""

from .log_parser_v1 import LogParserV1
from .log_parser_v3 import LogParserV3


def get_parser(database, additional_databases=None):
    """Get the parser object based on database

    If additional databases are provided, they are used together with the primary
    database for parsing messages from multiple domains. This is supported only
    for version 3 databases.
    """
    db_ver = int(database.get_version())

    if additional_databases:
        if db_ver != 3 or any(int(db.get_version()) != 3 for db in additional_databases):
            raise ValueError("Multiple databases are supported only for version 3 databases.")

        return LogParserV3(database, additional_databases)

    # DB version 1 and 2 correspond to v1 parser
    if db_ver in [1, 2]:
        return LogParserV1(database)

    # DB version 3 correspond to v3 parser
    if db_ver == 3:
        return LogParserV3(database)

    return None
