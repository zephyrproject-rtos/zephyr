#!/usr/bin/env python3
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
This test file contains foundational testcases for Sanitycheck tool
"""

import os
import sys
import pytest

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/sanity_chk"))

import scl
from sanitylib import SanityConfigParser

def test_yamlload():
    """ Test to check if loading the non-existent files raises the errors """
    filename = 'testcase_nc.yaml'
    with pytest.raises(FileNotFoundError):
        scl.yaml_load(filename)


@pytest.mark.parametrize("filename, schema",
                         [("testcase_correct_schema.yaml", "testcase-schema.yaml"),
                          ("platform_correct_schema.yaml", "platform-schema.yaml")])
def test_correct_schema(filename, schema, test_data):
    """ Test to validate the testcase schema"""
    filename = test_data + filename
    schema = scl.yaml_load(ZEPHYR_BASE +'/scripts/sanity_chk/' + schema)
    data = SanityConfigParser(filename, schema)
    data.load()
    assert data


@pytest.mark.parametrize("filename, schema",
                         [("testcase_incorrect_schema.yaml", "testcase-schema.yaml"),
                          ("platform_incorrect_schema.yaml", "platform-schema.yaml")])
def test_incorrect_schema(filename, schema, test_data):
    """ Test to validate the exception is raised for incorrect testcase schema"""
    filename = test_data + filename
    schema = scl.yaml_load(ZEPHYR_BASE +'/scripts/sanity_chk/' + schema)
    with pytest.raises(Exception) as exception:
        scl.yaml_load_verify(filename, schema)
        assert str(exception.value) == "Schema validation failed"
