#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Status classes to be used instead of str statuses.
"""

from enum import Enum


class TestInstanceStatus(str, Enum):
    def __str__(self):
        return str(self.value)

    NONE = None  # to preserve old functionality
    ERROR = 'error'
    FAIL = 'failed'
    FILTER = 'filtered'
    PASS = 'passed'
    SKIP = 'skipped'


# Possible direct assignments:
#   * TestSuiteStatus <- TestInstanceStatus
class TestSuiteStatus(str, Enum):
    def __str__(self):
        return str(self.value)

    NONE = None  # to preserve old functionality
    FILTER = 'filtered'
    PASS = 'passed'
    SKIP = 'skipped'


# Possible direct assignments:
#    * TestCaseStatus <- TestInstanceStatus
class TestCaseStatus(str, Enum):
    def __str__(self):
        return str(self.value)

    NONE = None  # to preserve old functionality
    BLOCK = 'blocked'
    ERROR = 'error'
    FAIL = 'failed'
    FILTER = 'filtered'
    PASS = 'passed'
    SKIP = 'skipped'
    STARTED = 'started'


# Possible direct assignments:
#   * OutputStatus <- HarnessStatus
class OutputStatus(str, Enum):
    def __str__(self):
        return str(self.value)

    NONE = None  # to preserve old functionality
    BYTE = 'unexpected byte'
    EOF = 'unexpected eof'
    FAIL = 'failed'
    TIMEOUT = 'timeout'


# Possible direct assignments:
#   * TestInstanceStatus <- HarnessStatus
class HarnessStatus(str, Enum):
    def __str__(self):
        return str(self.value)

    NONE = None  # to preserve old functionality
    ERROR = 'error'
    FAIL = 'failed'
    PASS = 'passed'
    SKIP = 'skipped'


class ReportStatus(str, Enum):
    def __str__(self):
        return str(self.value)

    ERROR = 'error'
    FAIL = 'failure'  # Note the difference!
    SKIP = 'skipped'
