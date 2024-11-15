#!/usr/bin/env python3
# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Tests for the error classes
"""

import os
import pytest

from pathlib import Path
from twisterlib.error import StatusAttributeError
from twisterlib.error import ConfigurationError
from twisterlib.harness import Test


def test_configurationerror():
    cfile = Path('some') / 'path'
    message = 'dummy message'

    expected_err = f'{os.path.join("some", "path")}: dummy message'

    with pytest.raises(ConfigurationError, match=expected_err):
        raise ConfigurationError(cfile, message)


def test_status_value_error():
    harness = Test()

    expected_err = 'Test assigned status OK,' \
                   ' which could not be cast to a TwisterStatus.'

    with pytest.raises(StatusAttributeError, match=expected_err):
        harness.status = "OK"
