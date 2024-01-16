#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Blackbox tests for twister's command line functions - those requiring testplan.json
"""

import importlib
import mock
import os
import pytest
import sys
import json

from conftest import ZEPHYR_BASE, TEST_DATA, testsuite_filename_mock
from twisterlib.testplan import TestPlan
from twisterlib.error import TwisterRuntimeError


class TestTestPlan:
    TESTDATA_1 = [
        ('smoke', 5),
        ('acceptance', 6),
    ]
    TESTDATA_2 = [
        ('dummy.agnostic.group2.assert1', SystemExit, 3),
        (
            os.path.join('scripts', 'tests', 'twister_blackbox', 'test_data', 'tests',
                         'dummy', 'agnostic', 'group1', 'subgroup1',
                         'dummy.agnostic.group2.assert1'),
            TwisterRuntimeError,
            None
        ),
    ]
    TESTDATA_3 = [
        ('buildable', 6),
        ('runnable', 5),
    ]
    TESTDATA_4 = [
        (True, 1),
        (False, 6),
    ]

    @classmethod
    def setup_class(cls):
        apath = os.path.join(ZEPHYR_BASE, 'scripts', 'twister')
        cls.loader = importlib.machinery.SourceFileLoader('__main__', apath)
        cls.spec = importlib.util.spec_from_loader(cls.loader.name, cls.loader)
        cls.twister_module = importlib.util.module_from_spec(cls.spec)

    @classmethod
    def teardown_class(cls):
        pass

    @pytest.mark.parametrize(
        'level, expected_tests',
        TESTDATA_1,
        ids=['smoke', 'acceptance']
    )
    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', testsuite_filename_mock)
    def test_level(self, out_path, level, expected_tests):
        test_platforms = ['qemu_x86', 'frdm_k64f']
        path = os.path.join(TEST_DATA, 'tests')
        config_path = os.path.join(TEST_DATA, 'test_config.yaml')
        args = ['-i','--outdir', out_path, '-T', path, '--level', level, '-y',
                '--test-config', config_path] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        with open(os.path.join(out_path, 'testplan.json')) as f:
            j = json.load(f)
        filtered_j = [
            (ts['platform'], ts['name'], tc['identifier']) \
                for ts in j['testsuites'] \
                for tc in ts['testcases'] if 'reason' not in tc
        ]

        assert str(sys_exit.value) == '0'

        assert expected_tests == len(filtered_j)

    @pytest.mark.parametrize(
        'test, expected_exception, expected_subtest_count',
        TESTDATA_2,
        ids=['valid', 'invalid']
    )
    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', testsuite_filename_mock)
    def test_subtest(self, out_path, test, expected_exception, expected_subtest_count):
        test_platforms = ['qemu_x86', 'frdm_k64f']
        path = os.path.join(TEST_DATA, 'tests', 'dummy')
        args = ['-i', '--outdir', out_path, '-T', path, '--sub-test', test, '-y'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(expected_exception) as exc:
            self.loader.exec_module(self.twister_module)

        if expected_exception != SystemExit:
            assert True
            return

        with open(os.path.join(out_path, 'testplan.json')) as f:
            j = json.load(f)
        filtered_j = [
            (ts['platform'], ts['name'], tc['identifier']) \
                for ts in j['testsuites'] \
                for tc in ts['testcases'] if 'reason' not in tc
        ]

        assert str(exc.value) == '0'
        assert len(filtered_j) == expected_subtest_count

    @pytest.mark.parametrize(
        'filter, expected_count',
        TESTDATA_3,
        ids=['buildable', 'runnable']
    )
    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', testsuite_filename_mock)
    def test_filter(self, out_path, filter, expected_count):
        test_platforms = ['qemu_x86', 'frdm_k64f']
        path = os.path.join(TEST_DATA, 'tests', 'dummy')
        args = ['-i', '--outdir', out_path, '-T', path, '--filter', filter, '-y'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as exc:
            self.loader.exec_module(self.twister_module)

        assert str(exc.value) == '0'

        with open(os.path.join(out_path, 'testplan.json')) as f:
            j = json.load(f)
        filtered_j = [
            (ts['platform'], ts['name'], tc['identifier']) \
                for ts in j['testsuites'] \
                for tc in ts['testcases'] if 'reason' not in tc
        ]

        assert expected_count == len(filtered_j)

    @pytest.mark.parametrize(
        'integration, expected_count',
        TESTDATA_4,
        ids=['integration', 'no integration']
    )
    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', testsuite_filename_mock)
    @mock.patch.object(TestPlan, 'SAMPLE_FILENAME', '')
    def test_integration(self, out_path, integration, expected_count):
        test_platforms = ['qemu_x86', 'frdm_k64f']
        path = os.path.join(TEST_DATA, 'tests', 'dummy')
        args = ['-i', '--outdir', out_path, '-T', path, '-y'] + \
               (['--integration'] if integration else []) + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as exc:
            self.loader.exec_module(self.twister_module)

        assert str(exc.value) == '0'

        with open(os.path.join(out_path, 'testplan.json')) as f:
            j = json.load(f)
        filtered_j = [
            (ts['platform'], ts['name'], tc['identifier']) \
                for ts in j['testsuites'] \
                for tc in ts['testcases'] if 'reason' not in tc
        ]

        assert expected_count == len(filtered_j)
