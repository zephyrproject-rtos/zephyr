#!/usr/bin/env python3
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
This test file contains foundational testcases for Sanitycheck tool
"""

import os
import imp
import sys
import pytest
ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/"))
from sanity_chk import scl
sanitycheck = imp.load_source('sanitycheck', ZEPHYR_BASE + '/scripts/sanitycheck')

@pytest.fixture(name='test_data')
def _test_data():
    """ Pytest fixture to set the path of test_data"""
    data = ZEPHYR_BASE + "/scripts/tests/sanitycheck/test_data/"
    return data

def test_yamlload():
    """ Test to check if loading the non-existent files raises the errors """
    filename = 'testcase_nc.yaml'
    with pytest.raises(FileNotFoundError):
        scl.yaml_load(filename)

def test_correct_testcase_schema(test_data):
    """ Test to validate the testcase schema"""
    filename = test_data + 'testcase_correct_schema.yaml'
    schema = scl.yaml_load(ZEPHYR_BASE +'/scripts/sanity_chk/testcase-schema.yaml')
    data = sanitycheck.SanityConfigParser(filename, schema)
    data.load()
    assert data
