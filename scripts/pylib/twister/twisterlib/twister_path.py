#!/usr/bin/env python3
# vim: set syntax=python ts=4 :
#
# Copyright (c) 2024 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import os

from pathlib import Path, PosixPath

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
canonical_zephyr_base = os.path.realpath(ZEPHYR_BASE)


class TPath(os.PathLike):
    def __init__(self, path, *args):
        self.path = Path(path)

        for p in args:
            self.path = self._joinpath(p)

    def get_longpath(self):
        normalised = os.fspath(self.path.resolve())

        if isinstance(self.path, PosixPath):
            return Path(normalised)

        # On Windows, without this prefix, there is 260-character path length limit.
        if not normalised.startswith('\\\\?\\'):
            normalised = '\\\\?\\' + normalised
        return Path(normalised)

    def _joinpath(self, other):
        return self.path.joinpath(str(other))

    def joinpath(self, other):
        res = TPath(self._joinpath(other))
        return res

    def get_rel_str(self):
        str_path = os.path.relpath(str(self.path), start=canonical_zephyr_base)
        return str_path

    def get_rel_after_dots(self):
        str_path = str(self.path)
        str_path = str_path.rsplit(os.pardir+os.path.sep, 1)[-1]
        return TPath(str_path)

    def get_rel_after_dots_str(self):
        str_path = str(self.path)
        if os.path.isabs(str_path):
            return str_path
        str_path = os.path.relpath(str(self.path), start=canonical_zephyr_base)
        str_path = str_path.rsplit(os.pardir+os.path.sep, 1)[-1]
        return str_path

    def is_dir(self):
        return self.path.is_dir()

    def __hash__(self):
        return hash(os.path.abspath(os.fspath(self)))

    def __eq__(self, other):
        try:
            return os.path.abspath(os.fspath(self)) == os.path.abspath(os.fspath(other))
        except TypeError:
            return False

    def __lt__(self, other):
        return str(self) < str(other)

    def __truediv__(self, other):
        return self.joinpath(other)

    def __rtruediv__(self, other):
        return self.joinpath(other)

    def __add__(self, other):
        return self.joinpath(other)

    def __radd__(self, other):
        return self.joinpath(other)

    def __iadd__(self, other):
        self.path = self._joinpath(other)

    def __str__(self):
        return str(self.get_longpath())

    def __repr__(self):
        return '<TPath: ' + str(self.get_longpath()) + '>'

    def __fspath__(self):
        return os.fspath(self.path)

    def __format__(self, format_spec):
        return self.__str__().format(format_spec)
