#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Status classes to be used instead of str statuses.
"""

from enum import Enum


class TwisterStatus(str, Enum):
    def __str__(self):
        return str(self.value)

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
