#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Status classes to be used instead of str statuses.
"""
from __future__ import annotations

from colorama import Fore
from enum import Enum


class TwisterStatus(str, Enum):
    def __str__(self):
        return str(self.value)

    @classmethod
    def _missing_(cls, value):
        super()._missing_(value)
        if value is None:
            return TwisterStatus.NONE

    @staticmethod
    def get_color(status: TwisterStatus) -> str:
        match(status):
            case TwisterStatus.PASS:
                color = Fore.GREEN
            case TwisterStatus.SKIP | TwisterStatus.FILTER | TwisterStatus.BLOCK:
                color = Fore.YELLOW
            case TwisterStatus.FAIL | TwisterStatus.ERROR:
                color = Fore.RED
            case TwisterStatus.STARTED | TwisterStatus.NONE:
                color = Fore.MAGENTA
            case _:
                color = Fore.RESET
        return color

    # All statuses below this comment can be used for TestCase
    BLOCK = 'blocked'
    STARTED = 'started'

    # All statuses below this comment can be used for TestSuite
    # All statuses below this comment can be used for TestInstance
    FILTER = 'filtered'

    # All statuses below this comment can be used for Harness
    NONE = None
    ERROR = 'error'
    FAIL = 'failed'
    PASS = 'passed'
    SKIP = 'skipped'
