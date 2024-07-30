#!/usr/bin/env python3
#
# Copyright (c) 2021 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""
Abstract Class for Dictionary-based Logging Parsers
"""

import abc
from colorama import Fore

from .data_types import DataTypes

LOG_LEVELS = [
    ('none', Fore.WHITE),
    ('err', Fore.RED),
    ('wrn', Fore.YELLOW),
    ('inf', Fore.GREEN),
    ('dbg', Fore.BLUE)
]

def get_log_level_str_color(lvl):
    """Convert numeric log level to string"""
    if lvl < 0 or lvl >= len(LOG_LEVELS):
        return ("unk", Fore.WHITE)

    return LOG_LEVELS[lvl]


def formalize_fmt_string(fmt_str):
    """Replace unsupported formatter"""
    new_str = fmt_str

    for spec in ['d', 'i', 'o', 'u', 'x', 'X']:
        # Python doesn't support %ll for integer specifiers, so remove extra 'l'
        new_str = new_str.replace("%ll" + spec, "%l" + spec)

        if spec in ['x', 'X']:
            new_str = new_str.replace("%#ll" + spec, "%#l" + spec)

        # Python doesn't support %hh for integer specifiers, so remove extra 'h'
        new_str = new_str.replace("%hh" + spec, "%h" + spec)

    # No %p for pointer either, so use %x
    new_str = new_str.replace("%p", "0x%x")

    return new_str


class LogParser(abc.ABC):
    """Abstract class of log parser"""
    def __init__(self, database):
        self.database = database

        self.data_types = DataTypes(self.database)


    @abc.abstractmethod
    def parse_log_data(self, logdata, debug=False):
        """Parse log data"""
        return None
