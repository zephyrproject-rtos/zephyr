#!/usr/bin/env python3
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=redefined-outer-name
# pylint: disable=line-too-long
'''Common fixtures for use in testing the sanitycheck tool.'''

import os
import sys
import pytest

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/sanity_chk"))
from sanitylib import TestSuite

@pytest.fixture(name='test_data')
def _test_data():
    """ Pytest fixture to load the test data directory"""
    data = ZEPHYR_BASE + "/scripts/tests/sanitycheck/test_data/"
    return data

@pytest.fixture
def testcases_dir():
    """ Pytest fixture to load the test data directory"""
    return ZEPHYR_BASE + "/scripts/tests/sanitycheck/test_data/testcases"

@pytest.fixture
def class_testsuite(test_data, testcases_dir):
    """ Pytest fixture to initialize and return the class TestSuite object"""
    board_root = test_data +"board_config/1_level/2_level/"
    testcase_root = [testcases_dir + '/tests', testcases_dir + '/samples']
    outdir = test_data +'sanity_out_demo'
    suite = TestSuite(board_root, testcase_root, outdir)
    return suite
