#!/usr/bin/env python3

# Copyright (c) 2019 - 2020 Nordic Semiconductor ASA
# Copyright (c) 2019 Linaro Limited
# Copyright (c) 2024 SILA Embedded Solutions GmbH

import logging
import sys


class LogFormatter(logging.Formatter):
    '''A log formatter that prints the level name in lower case,
    for compatibility with earlier versions of edtlib.'''

    def __init__(self):
        super().__init__(fmt='%(levelnamelower)s: %(message)s')

    def format(self, record):
        record.levelnamelower = record.levelname.lower()
        return super().format(record)


def setup_edtlib_logging() -> None:
    # The edtlib module emits logs using the standard 'logging' module.
    # Configure it so that warnings and above are printed to stderr,
    # using the LogFormatter class defined above to format each message.

    handler = logging.StreamHandler(sys.stderr)
    handler.setFormatter(LogFormatter())

    logger = logging.getLogger('edtlib')
    logger.setLevel(logging.WARNING)
    logger.addHandler(handler)
