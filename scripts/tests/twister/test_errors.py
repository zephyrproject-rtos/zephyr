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
from twisterlib.error import ConfigurationError

def test_configurationerror():
    cfile = Path('some') / 'path'
    message = 'dummy message'

    expected_err = f'{os.path.join("some", "path")}: dummy message'

    with pytest.raises(ConfigurationError, match=expected_err):
        raise ConfigurationError(cfile, message)
