#!/usr/bin/env python3
# Copyright (c) 2020 Intel Corporation
# Copyright (c) 2024 Arm Limited (or its affiliates). All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
"""
This test file contains foundational testcases for Twister tool
"""

import os
import sys
import mock
import pytest

from pathlib import Path

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/twister"))

import scl
from twisterlib.error import ConfigurationError
from twisterlib.testplan import TwisterConfigParser

def test_yamlload():
    """ Test to check if loading the non-existent files raises the errors """
    filename = 'testcase_nc.yaml'
    with pytest.raises(FileNotFoundError):
        scl.yaml_load(filename)


@pytest.mark.parametrize("filename, schema",
                         [("testsuite_correct_schema.yaml", "testsuite-schema.yaml"),
                          ("platform_correct_schema.yaml", "platform-schema.yaml")])
def test_correct_schema(filename, schema, test_data):
    """ Test to validate the testsuite schema"""
    filename = test_data + filename
    schema = scl.yaml_load(ZEPHYR_BASE +'/scripts/schemas/twister//' + schema)
    data = TwisterConfigParser(filename, schema)
    data.load()
    assert data


@pytest.mark.parametrize("filename, schema",
                         [("testsuite_incorrect_schema.yaml", "testsuite-schema.yaml"),
                          ("platform_incorrect_schema.yaml", "platform-schema.yaml")])
def test_incorrect_schema(filename, schema, test_data):
    """ Test to validate the exception is raised for incorrect testsuite schema"""
    filename = test_data + filename
    schema = scl.yaml_load(ZEPHYR_BASE +'/scripts/schemas/twister//' + schema)
    with pytest.raises(Exception) as exception:
        scl.yaml_load_verify(filename, schema)
        assert str(exception.value) == "Schema validation failed"

def test_testsuite_config_files():
    """ Test to validate conf and overlay files are extracted properly """
    filename = Path(ZEPHYR_BASE) / "scripts/tests/twister/test_data/test_data_with_deprecation_warnings.yaml"
    schema = scl.yaml_load(Path(ZEPHYR_BASE) / "scripts/schemas/twister/testsuite-schema.yaml")
    data = TwisterConfigParser(filename, schema)
    data.load()

    with mock.patch('warnings.warn') as warn_mock:
        # Load and validate the specific scenario from testcases.yaml
        scenario = data.get_scenario("test_config.main")
        assert scenario

        # CONF_FILE, DTC_OVERLAY_FILE, OVERLAY_CONFIG fields should be stripped out
        # of extra_args. Other fields should remain untouched.
        warn_mock.assert_called_once_with(
            "Do not specify CONF_FILE, OVERLAY_CONFIG, or DTC_OVERLAY_FILE in extra_args."
            " This feature is deprecated and will soon result in an error."
            " Use extra_conf_files, extra_overlay_confs"
            " or extra_dtc_overlay_files YAML fields instead",
            DeprecationWarning,
            stacklevel=2
        )

    assert scenario["extra_args"] == ["UNRELATED1=abc", "UNRELATED2=xyz"]

    # Check that all conf files have been assembled in the correct order
    assert ";".join(scenario["extra_conf_files"]) == \
        "conf1;conf2;conf3;conf4;conf5;conf6;conf7;conf8"

    # Check that all DTC overlay files have been assembled in the correct order
    assert ";".join(scenario["extra_dtc_overlay_files"]) == \
        "overlay1;overlay2;overlay3;overlay4;overlay5;overlay6;overlay7;overlay8"

    # Check that all overlay conf files have been assembled in the correct order
    assert scenario["extra_overlay_confs"] == \
        ["oc1.conf", "oc2.conf", "oc3.conf", "oc4.conf"]

    # Check extra kconfig statements, too
    assert scenario["extra_configs"] == ["CONFIG_FOO=y"]

def test_configuration_error():
    """Test to validate that the ConfigurationError is raisable without further errors.

    It ought to have an str with a path, colon and a message as its only args entry.
    """
    filename = Path(ZEPHYR_BASE) / "scripts/tests/twister/test_twister.py"

    with pytest.raises(ConfigurationError) as exception:
        raise ConfigurationError(filename, "Dummy message.")

    assert len(exception.value.args) == 1
    assert exception.value.args[0] == str(filename) + ": " + "Dummy message."
