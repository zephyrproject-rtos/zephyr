#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Status classes to be used instead of str statuses.
"""
from __future__ import annotations

from enum import Enum, EnumMeta

from colorama import Fore
from twisterlib.error import StatusAttributeError, StatusInitError, StatusKeyError


class TwisterStatusEnumMeta(EnumMeta):
    def __getattr__(cls, name):
        try:
            obj = super().__getattribute__(name)
        except AttributeError as ae:
            # Looks like an Enum key - let's give the user more information.
            if (
                name.isupper()
                and not (name.startswith('_') and name.endswith('_'))
            ):
                raise StatusAttributeError(name) from ae
            else:
                raise
        return obj

    def __getitem__(cls, name):
        try:
            return super().__getitem__(name)
        except KeyError as ke:
            raise StatusKeyError(name) from ke


class TwisterStatus(str, Enum, metaclass=TwisterStatusEnumMeta):
    def __str__(self):
        return str(self.value)

    @classmethod
    def _missing_(cls, value):
        super()._missing_(value)
        if value is None:
            return TwisterStatus.NONE
        elif value not in cls._value2member_map_:
            raise StatusInitError(value)

    @staticmethod
    def get_color(status: TwisterStatus) -> str:
        status2color = {
            TwisterStatus.PASS: Fore.GREEN,
            TwisterStatus.NOTRUN: Fore.CYAN,
            TwisterStatus.SKIP: Fore.YELLOW,
            TwisterStatus.FILTER: Fore.YELLOW,
            TwisterStatus.BLOCK: Fore.YELLOW,
            TwisterStatus.FAIL: Fore.RED,
            TwisterStatus.ERROR: Fore.RED,
            TwisterStatus.STARTED: Fore.MAGENTA,
            TwisterStatus.NONE: Fore.MAGENTA
        }
        return status2color.get(status, Fore.RESET)

    # All statuses below this comment can be used for TestCase
    BLOCK = 'blocked'
    STARTED = 'started'

    # All statuses below this comment can be used for TestSuite
    # All statuses below this comment can be used for TestInstance
    FILTER = 'filtered'
    NOTRUN = 'not run'

    # All statuses below this comment can be used for Harness
    NONE = None
    ERROR = 'error'
    FAIL = 'failed'
    PASS = 'passed'
    SKIP = 'skipped'
