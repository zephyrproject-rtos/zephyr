#!/usr/bin/env python3
# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Tests for the error classes
"""

import os
import re
import pytest

from pathlib import Path
from twisterlib.error import (
    StatusAttributeError,
    StatusAssignmentError,
    StatusInitError,
    StatusKeyError,
    ConfigurationError
)
from twisterlib.harness import Test
from twisterlib.statuses import TwisterStatus


def test_configurationerror():
    cfile = Path('some') / 'path'
    message = 'dummy message'

    expected_err = f'{os.path.join("some", "path")}: dummy message'

    with pytest.raises(ConfigurationError, match=expected_err):
        raise ConfigurationError(cfile, message)


def test_status_assignment_error():
    harness = Test()

    expected_err = 'Test assigned status "OK", which could not be cast to a TwisterStatus.'

    with pytest.raises(StatusAssignmentError, match=expected_err):
        harness.status = "OK"


def test_status_attribute_error():
    harness = Test()

    expected_err = (
        'Attempted access to nonexistent TwisterStatus.OK. Please verify the status list.'
    )

    with pytest.raises(StatusAttributeError, match=expected_err):
        harness.status = TwisterStatus.OK


def test_status_init_error():
    harness = Test()

    expected_err = (
        'Attempted initialisation of status "OK", which could not be cast to a TwisterStatus.'
    )

    with pytest.raises(StatusInitError, match=expected_err):
        harness.status = TwisterStatus("OK")


def test_status_key_error():
    harness = Test()

    expected_err = re.escape(
        'Attempted access to nonexistent TwisterStatus[OK]. Please verify the status list.'
    )

    with pytest.raises(StatusKeyError, match=expected_err):
        harness.status = TwisterStatus["OK"]
