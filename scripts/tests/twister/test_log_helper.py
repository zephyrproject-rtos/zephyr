#!/usr/bin/env python3
# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Tests for log_helper.py functions
"""

import logging
import mock
import pytest

from importlib import reload

import twisterlib.log_helper


TESTDATA = [
    ('Windows', 'dummy message: [\'dummy\', \'command\', \'-flag\']'),
    ('Linux', 'dummy message: dummy command -flag'),
]

@pytest.mark.parametrize(
    'system, expected_log',
    TESTDATA,
    ids=['Windows', 'Linux']
)
def test_log_command(caplog, system, expected_log):
    caplog.set_level(logging.DEBUG)

    logger = logging.getLogger('dummy')
    message = 'dummy message'
    args = ['dummy', 'command', '-flag']

    with mock.patch('platform.system', return_value=system):
        reload(twisterlib.log_helper)
        twisterlib.log_helper.log_command(logger, message, args)

    reload(twisterlib.log_helper)

    assert expected_log in caplog.text
