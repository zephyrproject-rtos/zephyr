#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Status classes to be used instead of str statuses.
"""
from __future__ import annotations

from enum import Enum

from colorama import Fore
from twisterlib.error import StatusAttributeError


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


class StatusMixin:
    """Mixin providing a validated ``status`` property backed by ``_status``.

    Assigning a value that is not a valid TwisterStatus (by name or value)
    raises StatusAttributeError, so illegal statuses fail loudly at the
    assignment site. Shared by TestCase, TestSuite and TestInstance.
    """
    _status: TwisterStatus = TwisterStatus.NONE

    @property
    def status(self) -> TwisterStatus:
        return self._status

    @status.setter
    def status(self, value: TwisterStatus) -> None:
        # Check for illegal assignments by value
        try:
            key = value.name if isinstance(value, Enum) else value
            self._status = TwisterStatus[key]
        except KeyError as err:
            raise StatusAttributeError(self.__class__, value) from err
