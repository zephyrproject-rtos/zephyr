#!/usr/bin/env python3
# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Blackbox tests for twister's command line functions
"""

import importlib
import mock
import os
import pytest
import sys

from conftest import TEST_DATA, ZEPHYR_BASE, testsuite_filename_mock
from twisterlib.testplan import TestPlan


@mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', testsuite_filename_mock)
class TestPrintOuts:
    TESTDATA_1 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['agnostic', 'subgrouped']
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'device'),
            ['device']
        ),
    ]

    TESTDATA_2 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            [
                'dummy.agnostic.group1.subgroup1.assert',
                'dummy.agnostic.group1.subgroup2.assert',
                'dummy.agnostic.group2.assert1',
                'dummy.agnostic.group2.assert2',
                'dummy.agnostic.group2.assert3'
            ]
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'device'),
            [
                'dummy.device.group.assert'
            ]
        ),
    ]

    TESTDATA_3 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            'Testsuite\n' \
            '├── Samples\n' \
            '└── Tests\n' \
            '    └── dummy\n' \
            '        └── agnostic\n' \
            '            ├── dummy.agnostic.group1.subgroup1.assert\n' \
            '            ├── dummy.agnostic.group1.subgroup2.assert\n' \
            '            ├── dummy.agnostic.group2.assert1\n' \
            '            ├── dummy.agnostic.group2.assert2\n' \
            '            └── dummy.agnostic.group2.assert3\n'
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'device'),
            'Testsuite\n'
            '├── Samples\n'
            '└── Tests\n'
            '    └── dummy\n'
            '        └── device\n'
            '            └── dummy.device.group.assert\n'
        ),
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


    @pytest.mark.usefixtures("clear_log")
    @pytest.mark.parametrize(
        'test_path, expected',
        TESTDATA_1,
        ids=[
            'tests/dummy/agnostic',
            'tests/dummy/device',
        ]
    )
    def test_list_tags(self, capfd, test_path, expected):
        args = ['-T', test_path, '--list-tags']

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
            pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        printed_tags = [tag.strip() for tag in out.split('- ')[1:]]

        assert all([tag in printed_tags for tag in expected])
        assert all([tag in expected for tag in printed_tags])

        assert str(sys_exit.value) == '0'


    @pytest.mark.usefixtures("clear_log")
    @pytest.mark.parametrize(
        'test_path, expected',
        TESTDATA_2,
        ids=[
            'tests/dummy/agnostic',
            'tests/dummy/device',
        ]
    )
    def test_list_tests(self, capfd, test_path, expected):
        args = ['-T', test_path, '--list-tests']

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
            pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        printed_tests = [test.strip() for test in out.split('- ')[1:]]
        printed_tests[-1] = printed_tests[-1].split('\n')[0]

        assert all([test in printed_tests for test in expected])
        assert all([test in expected for test in printed_tests])

        assert str(sys_exit.value) == '0'


    @pytest.mark.usefixtures("clear_log")
    @pytest.mark.parametrize(
        'test_path, expected',
        TESTDATA_3,
        ids=[
            'tests/dummy/agnostic',
            'tests/dummy/device',
        ]
    )
    def test_tree(self, capfd, test_path, expected):
        args = ['-T', test_path, '--test-tree']

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
            pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        assert expected in out
        assert str(sys_exit.value) == '0'
