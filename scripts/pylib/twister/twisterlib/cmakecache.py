#!/usr/bin/env python3
# vim: set syntax=python ts=4 :
#
# Copyright (c) 2018 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import re
from collections import OrderedDict


class CMakeCacheEntry:
    '''Represents a CMake cache entry.

    This class understands the type system in a CMakeCache.txt, and
    converts the following cache types to Python types:

    Cache Type    Python type
    ----------    -------------------------------------------
    FILEPATH      str
    PATH          str
    STRING        str OR list of str (if ';' is in the value)
    BOOL          bool
    INTERNAL      str OR list of str (if ';' is in the value)
    ----------    -------------------------------------------
    '''

    # Regular expression for a cache entry.
    #
    # CMake variable names can include escape characters, allowing a
    # wider set of names than is easy to match with a regular
    # expression. To be permissive here, use a non-greedy match up to
    # the first colon (':'). This breaks if the variable name has a
    # colon inside, but it's good enough.
    CACHE_ENTRY = re.compile(
        r'''(?P<name>.*?)                               # name
         :(?P<type>FILEPATH|PATH|STRING|BOOL|INTERNAL)  # type
         =(?P<value>.*)                                 # value
        ''', re.X)

    @classmethod
    def _to_bool(cls, val):
        # Convert a CMake BOOL string into a Python bool.
        #
        #   "True if the constant is 1, ON, YES, TRUE, Y, or a
        #   non-zero number. False if the constant is 0, OFF, NO,
        #   FALSE, N, IGNORE, NOTFOUND, the empty string, or ends in
        #   the suffix -NOTFOUND. Named boolean constants are
        #   case-insensitive. If the argument is not one of these
        #   constants, it is treated as a variable."
        #
        # https://cmake.org/cmake/help/v3.0/command/if.html
        val = val.upper()
        if val in ('ON', 'YES', 'TRUE', 'Y'):
            return 1
        elif (
            val in ('OFF', 'NO', 'FALSE', 'N', 'IGNORE', 'NOTFOUND', '')
            or val.endswith('-NOTFOUND')
        ):
            return 0
        else:
            try:
                v = int(val)
                return v != 0
            except ValueError as exc:
                raise ValueError(f'invalid bool {val}') from exc

    @classmethod
    def from_line(cls, line, line_no):
        # Comments can only occur at the beginning of a line.
        # (The value of an entry could contain a comment character).
        if line.startswith('//') or line.startswith('#'):
            return None

        # Whitespace-only lines do not contain cache entries.
        if not line.strip():
            return None

        m = cls.CACHE_ENTRY.match(line)
        if not m:
            return None

        name, type_, value = (m.group(g) for g in ('name', 'type', 'value'))
        if type_ == 'BOOL':
            try:
                value = cls._to_bool(value)
            except ValueError as exc:
                args = exc.args + (f'on line {line_no}: {line}',)
                raise ValueError(args) from exc
        # If the value is a CMake list (i.e. is a string which contains a ';'),
        # convert to a Python list.
        elif type_ in ['STRING', 'INTERNAL'] and ';' in value:
            value = value.split(';')

        return CMakeCacheEntry(name, value)

    def __init__(self, name, value):
        self.name = name
        self.value = value

    def __str__(self):
        fmt = 'CMakeCacheEntry(name={}, value={})'
        return fmt.format(self.name, self.value)


class CMakeCache:
    '''Parses and represents a CMake cache file.'''

    @staticmethod
    def from_file(cache_file):
        return CMakeCache(cache_file)

    def __init__(self, cache_file):
        self.cache_file = cache_file
        self.load(cache_file)

    def load(self, cache_file):
        entries = []
        with open(cache_file) as cache:
            for line_no, line in enumerate(cache):
                entry = CMakeCacheEntry.from_line(line, line_no)
                if entry:
                    entries.append(entry)
        self._entries = OrderedDict((e.name, e) for e in entries)

    def get(self, name, default=None):
        entry = self._entries.get(name)
        if entry is not None:
            return entry.value
        else:
            return default

    def get_list(self, name, default=None):
        if default is None:
            default = []
        entry = self._entries.get(name)
        if entry is not None:
            value = entry.value
            if isinstance(value, list):
                return value
            elif isinstance(value, str):
                return [value] if value else []
            else:
                msg = 'invalid value {} type {}'
                raise RuntimeError(msg.format(value, type(value)))
        else:
            return default

    def __contains__(self, name):
        return name in self._entries

    def __getitem__(self, name):
        return self._entries[name].value

    def __setitem__(self, name, entry):
        if not isinstance(entry, CMakeCacheEntry):
            msg = 'improper type {} for value {}, expecting CMakeCacheEntry'
            raise TypeError(msg.format(type(entry), entry))
        self._entries[name] = entry

    def __delitem__(self, name):
        del self._entries[name]

    def __iter__(self):
        return iter(self._entries.values())
