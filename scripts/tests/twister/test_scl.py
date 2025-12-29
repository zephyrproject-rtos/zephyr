#!/usr/bin/env python3
# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Tests for scl.py functions
"""

import logging
import sys
import types
from contextlib import nullcontext
from importlib import reload
from unittest import mock

import pytest
import scl
from pykwalify.errors import SchemaError
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


TESTDATA_1 = [
    (False),
    (True),
]

@pytest.mark.parametrize(
    'fail_parsing',
    TESTDATA_1,
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


TESTDATA_2 = [
    (True, False, None),
    (False, False, SchemaError),
    (False, True, ScannerError),
]

@pytest.mark.parametrize(
    'validate, fail_load, expected_error',
    TESTDATA_2,
    ids=['successful validation', 'failed validation', 'failed load']
)
def test_yaml_load_verify(validate, fail_load, expected_error):
    filename = 'dummy/file.yaml'
    data_mock = mock.Mock()

    def mock_load(file_name):
        assert file_name == filename
        if fail_load:
            raise ScannerError
        return data_mock

    def mock_validate(data):
        assert data == data_mock
        if validate:
            return True
        raise SchemaError(u'Schema validation failed.')

    with mock.patch('scl.yaml_load', side_effect=mock_load), \
         pytest.raises(expected_error) if expected_error else nullcontext():
        res = scl.yaml_load_verify(filename, mock_validate)

    if validate:
        assert res == data_mock


def test_make_yaml_validator_no_schema_path():
    v = scl.make_yaml_validator("")
    # should be a no-op
    assert v({"any": "thing"}) is None


def test_make_yaml_validator_pykwalify_ok(monkeypatch):
    """
    make_yaml_validator(schema_path) should:
      - yaml_load(schema_path) to get schema
      - create pykwalify.core.Core(source_data={}, schema_data=schema)
      - return validator that sets core.source = data and calls core.validate(raise_exception=True)
    """
    schema = {"type": "map", "mapping": {}}
    core_instance = mock.Mock()


    def core_ctor(*, source_data, schema_data):
        assert source_data == {}
        assert schema_data == schema
        return core_instance

    # Create a fake pykwalify.core module with Core
    fake_pykwalify_core = types.ModuleType("pykwalify.core")
    fake_pykwalify_core.Core = mock.Mock(side_effect=core_ctor)

    fake_pykwalify = types.ModuleType("pykwalify")
    fake_pykwalify.core = fake_pykwalify_core

    monkeypatch.setitem(sys.modules, "pykwalify", fake_pykwalify)
    monkeypatch.setitem(sys.modules, "pykwalify.core", fake_pykwalify_core)

    monkeypatch.setattr(scl, "yaml_load", mock.Mock(return_value=schema))

    validator = scl.make_yaml_validator("schema.yaml")
    data = {"any": "thing"}
    validator(data)

    assert core_instance.source == data
    core_instance.validate.assert_called_once_with(raise_exception=True)
    assert logging.getLogger("pykwalify.core").level == 50


def test_make_yaml_validator_no_pykwalify(caplog, monkeypatch):
    """
    If importing pykwalify.core fails, make_yaml_validator should:
      - log warning
      - return no-op validator
    """
    monkeypatch.setattr(scl, "yaml_load", mock.Mock(return_value={"type": "map"}))

    real_import = __import__

    def import_hook(name, globals=None, locals=None, fromlist=(), level=0):
        if name == "pykwalify.core" or name.startswith("pykwalify"):
            raise ImportError("no pykwalify for test")
        return real_import(name, globals, locals, fromlist, level)

    with mock.patch("builtins.__import__", side_effect=import_hook):
        v = scl.make_yaml_validator("schema.yaml")

    assert "can't import pykwalify; won't validate YAML" in caplog.text

    # no-op
    assert v({"any": "thing"}) is None


def test_yaml_load_empty_file(tmp_path):
    quarantine_file = tmp_path / 'empty_quarantine.yml'
    quarantine_file.write_text("# yaml file without data", encoding="utf-8")
    with pytest.raises(scl.EmptyYamlFileException):
        # validate callable shouldn't be reached, but must be provided
        scl.yaml_load_verify(quarantine_file, lambda _data: None)
