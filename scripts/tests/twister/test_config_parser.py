#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Tests for config_parser.py
"""

import os
import pytest
import mock
import scl

from twisterlib.config_parser import TwisterConfigParser, extract_fields_from_arg_list, ConfigurationError
from contextlib import nullcontext

def test_extract_single_field_from_string_argument():
    target_fields = {"FIELD1"}
    arg_list = "FIELD1=value1 FIELD2=value2 FIELD3=value3"
    extracted_fields, other_fields = extract_fields_from_arg_list(
        target_fields, arg_list)

    assert extracted_fields == {"FIELD1": ["value1"]}
    assert other_fields == "FIELD2=value2 FIELD3=value3"


def test_no_fields_to_extract():
    target_fields = set()
    arg_list = "arg1 arg2 arg3"

    extracted_fields, other_fields = extract_fields_from_arg_list(
        target_fields, arg_list)

    assert extracted_fields == {}
    assert other_fields == "arg1 arg2 arg3"


def test_missing_fields():
    target_fields = {"CONF_FILE", "OVERLAY_CONFIG", "DTC_OVERLAY_FILE"}
    arg_list = "arg1 arg2 arg3"
    extracted_fields, other_fields = extract_fields_from_arg_list(
        target_fields, arg_list)

    assert extracted_fields == {"CONF_FILE": [], "OVERLAY_CONFIG": [], "DTC_OVERLAY_FILE": []}
    assert other_fields == "arg1 arg2 arg3"

def test_load_yaml_with_extra_args_and_retrieve_scenario_data(zephyr_base):
    filename = "test_data.yaml"

    yaml_data = '''
    tests:
      scenario1:
        tags: ['tag1', 'tag2']
        extra_args: '--CONF_FILE=file1.conf --OVERLAY_CONFIG=config1.conf'
        filter: 'filter1'
    common:
      filter: 'filter2'
    '''

    loaded_schema = scl.yaml_load(
        os.path.join(zephyr_base, 'scripts', 'schemas','twister', 'testsuite-schema.yaml')
    )

    with mock.patch('builtins.open', mock.mock_open(read_data=yaml_data)):
        parser = TwisterConfigParser(filename, loaded_schema)
        parser.load()

    scenario_data = parser.get_scenario('scenario1')
    scenario_common = parser.common

    assert scenario_data['tags'] == {'tag1', 'tag2'}
    assert scenario_data['extra_args'] == ['--CONF_FILE=file1.conf', '--OVERLAY_CONFIG=config1.conf']
    assert scenario_common == {'filter': 'filter2'}


def test_default_values(zephyr_base):
    filename = "test_data.yaml"

    yaml_data = '''
    tests:
      scenario1:
        tags: 'tag1'
        extra_args: ''
    '''

    loaded_schema = scl.yaml_load(
        os.path.join(zephyr_base, 'scripts', 'schemas', 'twister','testsuite-schema.yaml')
    )

    with mock.patch('builtins.open', mock.mock_open(read_data=yaml_data)):
        parser = TwisterConfigParser(filename, loaded_schema)
        parser.load()

    expected_scenario_data = { 'type': 'integration',
        'extra_args': [],
        'extra_configs': [],
        'extra_conf_files': [],
        'extra_overlay_confs': [],
        'extra_dtc_overlay_files': [],
        'required_snippets': [],
        'build_only': False,
        'build_on_all': False,
        'skip': False, 'slow': False,
        'timeout': 60,
        'min_ram': 8,
        'modules': [],
        'depends_on': set(),
        'min_flash': 32,
        'arch_allow': set(),
        'arch_exclude': set(),
        'extra_sections': [],
        'integration_platforms': [],
        'ignore_faults': False,
        'ignore_qemu_crash': False,
        'testcases': [],
        'platform_type': [],
        'platform_exclude': set(),
        'platform_allow': set(),
        'platform_key': [],
        'toolchain_exclude': set(),
        'toolchain_allow': set(),
        'filter': '',
        'levels': [],
        'harness': 'test',
        'harness_config': {},
        'seed': 0, 'sysbuild': False
        }

    assert expected_scenario_data.items() <= expected_scenario_data.items()

@pytest.mark.parametrize(
    'value, typestr, expected',
    [
        (' hello ', 'str', 'hello'),
        ('3.14', 'float', 3.14),
        ('10', 'int', 10),
        ('True', 'bool', 'True'),          # do-nothing cast
        ('key: val', 'map', 'key: val'),   # do-nothing cast
        ('test', 'int', ValueError),
        ('test', 'unknown', ConfigurationError),
        ('1 2 2 3', 'list', ['1', '2', '2','3']),
        ('1 2 2 3', 'set', {'1', '2', '3'})
    ],
    ids=['str to str', 'str to float', 'str to int', 'str to bool', 'str to map',
         'invalid', 'to unknown', "to list", "to set"]
)

def test_cast_value(zephyr_base, value, typestr, expected):
    loaded_schema = scl.yaml_load(
        os.path.join(zephyr_base, 'scripts', 'schemas', 'twister','testsuite-schema.yaml')
    )

    parser = TwisterConfigParser("config.yaml", loaded_schema)
    with pytest.raises(expected) if \
         isinstance(expected, type) and issubclass(expected, Exception) else nullcontext():
        result = parser._cast_value(value, typestr)
        assert result == expected

def test_load_invalid_test_config_yaml(zephyr_base):
    filename = "test_data.yaml"

    yaml_data = '''
    gibberish data
    '''

    loaded_schema = scl.yaml_load(
        os.path.join(zephyr_base, 'scripts', 'schemas','twister', 'test-config-schema.yaml')
    )

    with mock.patch('builtins.open', mock.mock_open(read_data=yaml_data)):
        parser = TwisterConfigParser(filename, loaded_schema)
        with pytest.raises(Exception):
            parser.load()


def test_load_yaml_with_no_scenario_data(zephyr_base):
    filename = "test_data.yaml"

    yaml_data = '''
    tests:
      common:
        extra_args: '--CONF_FILE=file2.conf --OVERLAY_CONFIG=config2.conf'
    '''

    loaded_schema = scl.yaml_load(
        os.path.join(zephyr_base, 'scripts', 'schemas','twister', 'testsuite-schema.yaml')
    )

    with mock.patch('builtins.open', mock.mock_open(read_data=yaml_data)):
        parser = TwisterConfigParser(filename, loaded_schema)
        parser.load()

    with pytest.raises(KeyError):
        scenario_data = parser.get_scenario('scenario1')
        assert scenario_data is None
