#!/usr/bin/env python3
# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

'''Common fixtures for use in testing the twister tool.'''

import logging
import mock
import os
import pytest
import sys

ZEPHYR_BASE = os.getenv('ZEPHYR_BASE')
TEST_DATA = os.path.join(ZEPHYR_BASE, 'scripts', 'tests',
                        'twister_blackbox', 'test_data')


sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/twister"))
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts"))


testsuite_filename_mock = mock.PropertyMock(return_value='test_data.yaml')

@pytest.fixture(name='zephyr_base')
def zephyr_base_directory():
    return ZEPHYR_BASE


@pytest.fixture(name='zephyr_test_data')
def zephyr_test_directory():
    return TEST_DATA

@pytest.fixture
def clear_log():
    # Required to fix the pytest logging error
    # See: https://github.com/pytest-dev/pytest/issues/5502
    loggers = [logging.getLogger()] \
            + list(logging.Logger.manager.loggerDict.values()) \
            + [logging.getLogger(name) for \
                                 name in logging.root.manager.loggerDict]
    for logger in loggers:
        handlers = getattr(logger, 'handlers', [])
        for handler in handlers:
            logger.removeHandler(handler)
