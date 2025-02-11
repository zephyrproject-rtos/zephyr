#!/usr/bin/env python3
# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Tests for scl.py functions
"""

import logging
import mock
import os
import pytest
import sys

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/twister"))

import scl

from contextlib import nullcontext
from importlib import reload
from pykwalify.errors import SchemaError
from yaml.scanner import ScannerError


TESTDATA_1 = [
    (False),
    (True),
]

@pytest.mark.parametrize(
    'fail_c',
    TESTDATA_1,
    ids=['C YAML', 'non-C YAML']
)
def test_yaml_imports(fail_c):
    class ImportRaiser:
        def find_spec(self, fullname, path, target=None):
            if fullname == 'yaml.CLoader' and fail_c:
                raise ImportError()
            if fullname == 'yaml.CSafeLoader' and fail_c:
                raise ImportError()
            if fullname == 'yaml.CDumper' and fail_c:
                raise ImportError()

    modules_mock = sys.modules.copy()

    if hasattr(modules_mock['yaml'], 'CLoader'):
        del modules_mock['yaml'].CLoader
        del modules_mock['yaml'].CSafeLoader
        del modules_mock['yaml'].CDumper

    cloader_mock = mock.Mock()
    loader_mock = mock.Mock()
    csafeloader_mock = mock.Mock()
    safeloader_mock = mock.Mock()
    cdumper_mock = mock.Mock()
    dumper_mock = mock.Mock()

    if not fail_c:
        modules_mock['yaml'].CLoader = cloader_mock
        modules_mock['yaml'].CSafeLoader = csafeloader_mock
        modules_mock['yaml'].CDumper = cdumper_mock

    modules_mock['yaml'].Loader = loader_mock
    modules_mock['yaml'].SafeLoader = safeloader_mock
    modules_mock['yaml'].Dumper = dumper_mock

    meta_path_mock = sys.meta_path[:]
    meta_path_mock.insert(0, ImportRaiser())

    with mock.patch.dict('sys.modules', modules_mock, clear=True), \
         mock.patch('sys.meta_path', meta_path_mock):
        reload(scl)

    assert sys.modules['scl'].Loader == loader_mock if fail_c else \
                                        cloader_mock

    assert sys.modules['scl'].SafeLoader == safeloader_mock if fail_c else \
                                        csafeloader_mock

    assert sys.modules['scl'].Dumper == dumper_mock if fail_c else \
                                        cdumper_mock

    import yaml
    reload(yaml)


TESTDATA_2 = [
    (False, logging.CRITICAL, []),
    (True, None, ['can\'t import pykwalify; won\'t validate YAML']),
]

@pytest.mark.parametrize(
    'fail_pykwalify, log_level, expected_logs',
    TESTDATA_2,
    ids=['pykwalify OK', 'no pykwalify']
)
def test_pykwalify_import(caplog, fail_pykwalify, log_level, expected_logs):
    class ImportRaiser:
        def find_spec(self, fullname, path, target=None):
            if fullname == 'pykwalify.core' and fail_pykwalify:
                raise ImportError()

    modules_mock = sys.modules.copy()
    modules_mock['pykwalify'] = None if fail_pykwalify else \
                                modules_mock['pykwalify']

    meta_path_mock = sys.meta_path[:]
    meta_path_mock.insert(0, ImportRaiser())

    with mock.patch.dict('sys.modules', modules_mock, clear=True), \
         mock.patch('sys.meta_path', meta_path_mock):
        reload(scl)

    if log_level:
        assert logging.getLogger('pykwalify.core').level == log_level

    assert all([log in caplog.text for log in expected_logs])

    if fail_pykwalify:
        assert scl._yaml_validate(None, None) is None
        assert scl._yaml_validate(mock.Mock(), mock.Mock()) is None

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
    (False, False, SchemaError),
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

    def mock_load(file_name, *args, **kwargs):
        assert file_name == filename
        if fail_load:
            raise ScannerError
        return data_mock

    def mock_validate(data, schema, *args, **kwargs):
        assert data == data_mock
        assert schema == schema_mock
        if validate:
            return True
        raise SchemaError(u'Schema validation failed.')

    with mock.patch('scl.yaml_load', side_effect=mock_load), \
         mock.patch('scl._yaml_validate', side_effect=mock_validate), \
         pytest.raises(expected_error) if expected_error else nullcontext():
        res = scl.yaml_load_verify(filename, schema_mock)

    if validate:
        assert res == data_mock


TESTDATA_5 = [
    (True, True, None),
    (True, False, SchemaError),
    (False, None, None),
]

@pytest.mark.parametrize(
    'schema_exists, validate, expected_error',
    TESTDATA_5,
    ids=['successful validation', 'failed validation', 'no schema']
)
def test_yaml_validate(schema_exists, validate, expected_error):
    data_mock = mock.Mock()
    schema_mock = mock.Mock() if schema_exists else None

    def mock_validate(raise_exception, *args, **kwargs):
        assert raise_exception
        if validate:
            return True
        raise SchemaError(u'Schema validation failed.')

    def mock_core(source_data, schema_data, *args, **kwargs):
        assert source_data == data_mock
        assert schema_data == schema_mock
        return mock.Mock(validate=mock_validate)

    core_mock = mock.Mock(side_effect=mock_core)

    with mock.patch('pykwalify.core.Core', core_mock), \
         pytest.raises(expected_error) if expected_error else nullcontext():
        scl._yaml_validate(data_mock, schema_mock)

    if schema_exists:
        core_mock.assert_called_once()
    else:
        core_mock.assert_not_called()


def test_yaml_load_empty_file(tmp_path):
    quarantine_file = tmp_path / 'empty_quarantine.yml'
    quarantine_file.write_text("# yaml file without data")
    with pytest.raises(scl.EmptyYamlFileException):
        scl.yaml_load_verify(quarantine_file, None)
