#!/usr/bin/env python3
# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Tests for scl.py functions
"""

import sys
import types
from contextlib import nullcontext
from importlib import reload
from unittest import mock

import pytest
import scl
from jsonschema.exceptions import ValidationError
from yaml.scanner import ScannerError


@pytest.mark.parametrize("has_cyaml", [True, False], ids=["C YAML", "non-C YAML"])
def test_yaml_imports(has_cyaml):
    """
    scl.py does:
        from yaml import CSafeLoader as SafeLoader
    falling back to:
        from yaml import SafeLoader
    So we simulate a yaml module with/without CSafeLoader.
    """
    fake_yaml = types.ModuleType("yaml")
    fake_yaml.load = mock.Mock()
    fake_yaml.SafeLoader = object()
    if has_cyaml:
        fake_yaml.CSafeLoader = object()

    with mock.patch.dict(sys.modules, {"yaml": fake_yaml}):
        reload(scl)
        assert scl.SafeLoader is (fake_yaml.CSafeLoader if has_cyaml else fake_yaml.SafeLoader)

    # cleanup
    reload(scl)


TESTDATA_3 = [
    (False),
    (True),
]

@pytest.mark.parametrize(
    'fail_parsing',
    TESTDATA_3,
    ids=['ok', 'parsing error']
)
def test_yaml_load(caplog, fail_parsing):
    result_mock = mock.Mock()

    def mock_load(*args, **kwargs):
        if fail_parsing:
            context_mark = mock.Mock()
            problem_mark = mock.Mock()
            type(context_mark).args = mock.PropertyMock(return_value=[])
            type(context_mark).name = 'dummy context mark'
            type(context_mark).line = 0
            type(context_mark).column = 0
            type(problem_mark).args = mock.PropertyMock(return_value=[])
            type(problem_mark).name = 'dummy problem mark'
            type(problem_mark).line = 0
            type(problem_mark).column = 0
            raise ScannerError(context='dummy context',
                               context_mark=context_mark, problem='dummy problem',
                               problem_mark=problem_mark, note='Dummy note')
        return result_mock

    filename = 'dummy/file.yaml'

    with mock.patch('yaml.load', side_effect=mock_load), \
         mock.patch('builtins.open', mock.mock_open()) as mock_file:
        with pytest.raises(ScannerError) if fail_parsing else nullcontext():
            result = scl.yaml_load(filename)

    mock_file.assert_called_with('dummy/file.yaml', 'r', encoding='utf-8')

    if not fail_parsing:
        assert result == result_mock
    else:
        assert 'dummy problem mark:0:0: error: dummy problem' \
               ' (note Dummy note context @dummy context mark:0:0' \
               ' dummy context)' in caplog.text



TESTDATA_4 = [
    (True, False, None),
    (False, False, ValidationError),
    (False, True, ScannerError),
]

@pytest.mark.parametrize(
    'validate, fail_load, expected_error',
    TESTDATA_4,
    ids=['successful validation', 'failed validation', 'failed load']
)
def test_yaml_load_verify(validate, fail_load, expected_error):
    filename = 'dummy/file.yaml'
    schema_mock = mock.Mock()
    data_mock = mock.Mock()

    def mock_load(file_name):
        assert file_name == filename
        if fail_load:
            raise ScannerError
        return data_mock

    def mock_validate(data, schema):
        assert data == data_mock
        assert schema == schema_mock
        if validate:
            return None
        raise ValidationError("Schema validation failed")

    with mock.patch('scl.yaml_load', side_effect=mock_load), \
         mock.patch('scl._yaml_validate', side_effect=mock_validate), \
         pytest.raises(expected_error) if expected_error else nullcontext():
        res = scl.yaml_load_verify(filename, schema_mock)

    if validate:
        assert res == data_mock

def test_yaml_validate():
    data = {"a": 1}
    schema = {
        "type": "object",
        "properties": {
            "a": {"type": "string"}
        },
        "required": ["a"],
        "additionalProperties": False,
    }

    with pytest.raises(ValidationError):
        scl._yaml_validate(data, schema)


def test_yaml_validate_no_schema():
    data = {"a": 1}
    assert scl._yaml_validate(data, None) is None


def test_yaml_load_empty_file(tmp_path):
    quarantine_file = tmp_path / 'empty_quarantine.yml'
    quarantine_file.write_text("# yaml file without data", encoding="utf-8")
    with pytest.raises(scl.EmptyYamlFileException):
        scl.yaml_load_verify(quarantine_file, None)
