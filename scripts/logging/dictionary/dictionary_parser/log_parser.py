#!/usr/bin/env python3
#
# Copyright (c) 2021 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""
Abstract Class for Dictionary-based Logging Parsers
"""

import abc
import re
import struct

from colorama import Fore

from .data_types import DataTypes
from .log_sorter import LogSorter

LOG_LEVELS = [
    ('none', Fore.WHITE),
    ('err', Fore.RED),
    ('wrn', Fore.YELLOW),
    ('inf', Fore.GREEN),
    ('dbg', Fore.BLUE),
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
        new_str = re.sub(r'%(\#?\d*)ll' + spec, r'%\1l' + spec, new_str)

        if spec in ['x', 'X']:
            new_str = re.sub(r'%\#(\d*)ll' + spec, r'%#\1l' + spec, new_str)

        # Python doesn't support %hh for integer specifiers, so remove extra 'h'
        new_str = re.sub(r'%(\#?\d*)hh' + spec, r'%\1h' + spec, new_str)

    # No %p for pointer either, so use %x
    new_str = new_str.replace("%p", "0x%x")

    # No %z support, use %d instead
    new_str = new_str.replace("%z", "%d")

    return new_str


class LogParser(abc.ABC):
    """Abstract class of log parser"""

    def __init__(self, database, additional_databases=None):
        # Primary database. It is used for decoding the message headers and for all messages
        # coming from domain 0.
        self.database = database
        self.data_types = DataTypes(self.database)

        # All databases (primary first) together with their data types descriptors.
        self.databases = [database, *(additional_databases or [])]
        self.databases_data_types = [self.data_types] + [DataTypes(db) for db in self.databases[1:]]

        # Optional sorter, which caches the parsed messages and prints them ordered by timestamp.
        self.sorter = None

    def enable_sorting(self, latency_ms, ts_freq):
        """Cache parsed messages and print them sorted by timestamp.

        Messages from a remote domain may be delayed (e.g. by IPC), so they can appear in the
        stream out of order. When sorting is enabled the parser holds the messages for
        ``latency_ms`` and prints them in the timestamp order. Remaining messages must be
        printed by calling ``flush()`` (end of data) or ``flush_if_idle()`` (periodically when
        reading live data).

        :param latency_ms: Time for which messages are cached, in milliseconds.
        :param ts_freq: Frequency of the log timestamp, in Hz.
        """
        ts_bits = struct.calcsize(self.fmt_msg_timestamp) * 8
        self.sorter = LogSorter(latency_ms, ts_freq, ts_bits)

    def emit(self, timestamp, text):
        """Output the text of one parsed message.

        The text is printed immediately, or cached and sorted by ``timestamp`` if sorting is
        enabled. ``timestamp`` may be None if the message has no timestamp.
        """
        if self.sorter is not None:
            self.sorter.add(timestamp, text)
        else:
            print(text, end='')

    def flush(self):
        """Print all messages cached by the sorter (if sorting is enabled)."""
        if self.sorter is not None:
            self.sorter.flush()

    def flush_if_idle(self):
        """Print cached messages if no new message arrived for the latency time."""
        if self.sorter is not None:
            return self.sorter.flush_if_idle()
        return False

    def idle_timeout(self):
        """Time in seconds after which ``flush_if_idle()`` should be called, or None."""
        if self.sorter is not None:
            return self.sorter.timeout()
        return None

    @abc.abstractmethod
    def parse_log_data(self, logdata, debug=False):
        """Parse log data"""
        return None
