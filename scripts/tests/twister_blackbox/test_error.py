#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Blackbox tests for twister's command line functions - simple does-error-out or not tests
"""

import importlib
import mock
import os
import pytest
import sys
import re

from conftest import ZEPHYR_BASE, TEST_DATA, testsuite_filename_mock
from twisterlib.testplan import TestPlan
from twisterlib.error import TwisterRuntimeError


class TestError:
    TESTDATA_1 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy'),
            os.path.join('scripts', 'tests', 'twister_blackbox', 'test_data', 'tests',
                         'dummy', 'agnostic', 'group1', 'subgroup1',
                         'dummy.agnostic.group1.subgroup1'),
            SystemExit
        ),
        (
            None,
            'dummy.agnostic.group1.subgroup1',
            TwisterRuntimeError
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy'),
            'dummy.agnostic.group1.subgroup1',
            SystemExit
        )
    ]
    TESTDATA_2 = [
        (
            '',
            r'always_overflow.dummy SKIPPED \(RAM overflow\)'
        ),
        (
            '--overflow-as-errors',
            r'always_overflow.dummy  ERROR Build failure \(build\)'
        )
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
        'testroot, test, expected_exception',
        TESTDATA_1,
        ids=['valid', 'invalid', 'valid']
    )
    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', testsuite_filename_mock)
    def test_test(self, out_path, testroot, test, expected_exception):
        test_platforms = ['qemu_x86', 'intel_adl_crb']
        args = []
        if testroot:
            args = ['-T', testroot]
        args += ['-i', '--outdir', out_path, '--test', test, '-y'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(expected_exception) as exc:
            self.loader.exec_module(self.twister_module)

        if expected_exception == SystemExit:
            assert str(exc.value) == '0'
        assert True

    @pytest.mark.parametrize(
        'switch, expected',
        TESTDATA_2,
        ids=[
            'overflow skip',
            'overflow error',
        ],
    )

    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', testsuite_filename_mock)
    def test_overflow_as_errors(self, capfd, out_path, switch, expected):
        path = os.path.join(TEST_DATA, 'tests', 'qemu_overflow')
        test_platforms = ['qemu_x86']
        args = ['--outdir', out_path, '-T', path, '-vv'] + \
               ['--build-only'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]
        if switch:
            args += [switch]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
            pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        print(args)
        if switch:
            assert str(sys_exit.value) == '1'
        else:
            assert str(sys_exit.value) == '0'

        assert re.search(expected, err)
