# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0


class TwisterHarnessException(Exception):
    """General Twister harness exception."""


class TwisterHarnessTimeoutException(TwisterHarnessException):
    """Twister harness timeout exception"""
