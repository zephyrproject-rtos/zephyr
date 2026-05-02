#!/usr/bin/env python3
# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

'''Common fixtures for use in testing the twister tool.'''

import logging
import shutil
from unittest import mock
import os
import pytest
import sys

logging.getLogger("twister").setLevel("DEBUG")  # requires for testing twister

ZEPHYR_BASE = os.getenv('ZEPHYR_BASE')
TEST_DATA = os.path.join(ZEPHYR_BASE, 'scripts', 'tests',
                        'twister_blackbox', 'test_data')


sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/twister"))
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts"))


sample_filename_mock = mock.PropertyMock(return_value='test_sample.yaml')
suite_filename_mock = mock.PropertyMock(return_value='test_data.yaml')
sample_filename_mock = mock.PropertyMock(return_value='test_sample.yaml')

def pytest_configure(config):
    config.addinivalue_line("markers", "noclearlog: disable the clear_log autouse fixture")
    config.addinivalue_line("markers", "noclearout: disable the provide_out autouse fixture")

@pytest.fixture(name='zephyr_base')
def zephyr_base_directory():
    return ZEPHYR_BASE


@pytest.fixture(name='zephyr_test_data')
def zephyr_test_directory():
    return TEST_DATA

@pytest.fixture(autouse=True)
def clear_log(request):
    # As this fixture is autouse, one can use the pytest.mark.noclearlog decorator
    # in order to be sure that this fixture's code will not fire.
    if 'noclearlog' in request.keywords:
        return

    # clear_log is used by pytest fixture
    # However, clear_log_in_test is prepared to be used directly in the code, wherever required
    clear_log_in_test()

def clear_log_in_test():
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

# This fixture provides blackbox tests with an `out_path` parameter
# It should be used as the `-O` (`--out_dir`) parameter in blackbox tests
# APPRECIATED: method of using this out_path wholly outside of test code
@pytest.fixture(name='out_path', autouse=True)
def provide_out(tmp_path, request):
    # As this fixture is autouse, one can use the pytest.mark.noclearout decorator
    # in order to be sure that this fixture's code will not fire.
    # Most of the time, just omitting the `out_path` parameter is sufficient.
    if 'noclearout' in request.keywords:
        yield
        return

    # Before
    out_container_path = tmp_path / 'blackbox-out-container'
    out_container_path.mkdir()
    out_path = os.path.join(out_container_path, "blackbox-out")

    # Test
    yield out_path

    # After
    # We're operating in temp, so it is not strictly necessary
    # but the files can get large quickly as we do not need them after the test.
    loggers = [logging.getLogger(name) for name in logging.root.manager.loggerDict]
    for logg in loggers:
        handls = logg.handlers[:]
        for handl in handls:
            logg.removeHandler(handl)
            handl.close()
    shutil.rmtree(out_container_path)
