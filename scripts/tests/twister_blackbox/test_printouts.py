#!/usr/bin/env python3
# Copyright (c) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Blackbox tests for twister's command line functions
"""

import importlib
from unittest import mock
import os
import pytest
import sys
import re

# pylint: disable=no-name-in-module
from conftest import (
    TEST_DATA,
    ZEPHYR_BASE,
    clear_log_in_test,
    sample_filename_mock,
    suite_filename_mock
)
from twisterlib.testplan import TestPlan


@mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', suite_filename_mock)
class TestPrintOuts:
    TESTDATA_1 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['agnostic', 'subgrouped', 'even', 'odd']
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
                'dummy.agnostic.group1.subgroup1.a1_1_tests.assert',
                'dummy.agnostic.group1.subgroup2.a1_2_tests.assert',
                'dummy.agnostic.group2.a2_tests.assert1',
                'dummy.agnostic.group2.a2_tests.assert2',
                'dummy.agnostic.group2.a3_tests.assert1',
                'dummy.agnostic.group2.a2_tests.assert3'
            ],
            '--no-detailed-test-id',
            ''
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            [
                'dummy.agnostic.group1.subgroup2.a1_2_tests.assert',
                'dummy.agnostic.group2.a2_tests.assert1',
                'dummy.agnostic.group2.a2_tests.assert2',
                'dummy.agnostic.group2.a3_tests.assert1',
                'dummy.agnostic.group2.a2_tests.assert3'
            ],
            '--no-detailed-test-id',
            'odd'
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            [],
            '--no-detailed-test-id',
            'odd even'
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            [
                'dummy.agnostic.group1.subgroup1.a1_1_tests.assert',
                'dummy.agnostic.group1.subgroup2.a1_2_tests.assert',
                'dummy.agnostic.group2.a2_tests.assert1',
                'dummy.agnostic.group2.a2_tests.assert2',
                'dummy.agnostic.group2.a3_tests.assert1',
                'dummy.agnostic.group2.a2_tests.assert3'
            ],
            '--no-detailed-test-id',
            'unknown_tag'
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            [
                'dummy.agnostic.group1.subgroup1.a1_1_tests.assert',
                'dummy.agnostic.group1.subgroup2.a1_2_tests.assert',
                'dummy.agnostic.group2.a2_tests.assert1',
                'dummy.agnostic.group2.a2_tests.assert2',
                'dummy.agnostic.group2.a3_tests.assert1',
                'dummy.agnostic.group2.a2_tests.assert3'
            ],
            '--detailed-test-id',
            ''
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            [
                'dummy.agnostic.group1.subgroup2.a1_2_tests.assert',
                'dummy.agnostic.group2.a2_tests.assert1',
                'dummy.agnostic.group2.a2_tests.assert2',
                'dummy.agnostic.group2.a3_tests.assert1',
                'dummy.agnostic.group2.a2_tests.assert3'
            ],
            '--detailed-test-id',
            'odd'
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            [],
            '--detailed-test-id',
            'odd even'
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            [
                'dummy.agnostic.group1.subgroup1.a1_1_tests.assert',
                'dummy.agnostic.group1.subgroup2.a1_2_tests.assert',
                'dummy.agnostic.group2.a2_tests.assert1',
                'dummy.agnostic.group2.a2_tests.assert2',
                'dummy.agnostic.group2.a3_tests.assert1',
                'dummy.agnostic.group2.a2_tests.assert3'
            ],
            '--detailed-test-id',
            'unknown_tag'
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'device'),
            [
                'dummy.device.group.d_tests.assert'
            ],
            '--no-detailed-test-id',
            ''
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'device'),
            [
                'dummy.device.group.d_tests.assert'
            ],
            '--detailed-test-id',
            ''
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
            '            ├── dummy.agnostic.group1.subgroup1.a1_1_tests.assert\n' \
            '            ├── dummy.agnostic.group1.subgroup2.a1_2_tests.assert\n' \
            '            ├── dummy.agnostic.group2.a2_tests.assert1\n' \
            '            ├── dummy.agnostic.group2.a2_tests.assert2\n' \
            '            ├── dummy.agnostic.group2.a2_tests.assert3\n' \
            '            └── dummy.agnostic.group2.a3_tests.assert1\n',
            '--no-detailed-test-id',
            ''
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            'Testsuite\n' \
            '├── Samples\n' \
            '└── Tests\n' \
            '    └── dummy\n' \
            '        └── agnostic\n' \
            '            ├── dummy.agnostic.group1.subgroup2.a1_2_tests.assert\n' \
            '            ├── dummy.agnostic.group2.a2_tests.assert1\n' \
            '            ├── dummy.agnostic.group2.a2_tests.assert2\n' \
            '            ├── dummy.agnostic.group2.a2_tests.assert3\n' \
            '            └── dummy.agnostic.group2.a3_tests.assert1\n',
            '--no-detailed-test-id',
            'odd'
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            'Testsuite\n' \
            '├── Samples\n' \
            '└── Tests\n' \
            '    └── dummy\n' \
            '        └── agnostic\n' \
            '            ├── dummy.agnostic.group1.subgroup1.a1_1_tests.assert\n' \
            '            ├── dummy.agnostic.group1.subgroup2.a1_2_tests.assert\n' \
            '            ├── dummy.agnostic.group2.a2_tests.assert1\n' \
            '            ├── dummy.agnostic.group2.a2_tests.assert2\n' \
            '            ├── dummy.agnostic.group2.a2_tests.assert3\n' \
            '            └── dummy.agnostic.group2.a3_tests.assert1\n',
            '--detailed-test-id',
            ''
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            'Testsuite\n' \
            '├── Samples\n' \
            '└── Tests\n' \
            '    └── dummy\n' \
            '        └── agnostic\n' \
            '            ├── dummy.agnostic.group1.subgroup2.a1_2_tests.assert\n' \
            '            ├── dummy.agnostic.group2.a2_tests.assert1\n' \
            '            ├── dummy.agnostic.group2.a2_tests.assert2\n' \
            '            ├── dummy.agnostic.group2.a2_tests.assert3\n' \
            '            └── dummy.agnostic.group2.a3_tests.assert1\n',
            '--detailed-test-id',
            'odd'
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            'Testsuite\n' \
            '├── Samples\n' \
            '└── Tests\n',
            '--detailed-test-id',
            'odd even'
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            'Testsuite\n' \
            '├── Samples\n' \
            '└── Tests\n' \
            '    └── dummy\n' \
            '        └── agnostic\n' \
            '            ├── dummy.agnostic.group1.subgroup1.a1_1_tests.assert\n' \
            '            ├── dummy.agnostic.group1.subgroup2.a1_2_tests.assert\n' \
            '            ├── dummy.agnostic.group2.a2_tests.assert1\n' \
            '            ├── dummy.agnostic.group2.a2_tests.assert2\n' \
            '            ├── dummy.agnostic.group2.a2_tests.assert3\n' \
            '            └── dummy.agnostic.group2.a3_tests.assert1\n',
            '--detailed-test-id',
            'unknown_tag'
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'device'),
            'Testsuite\n'
            '├── Samples\n'
            '└── Tests\n'
            '    └── dummy\n'
            '        └── device\n'
            '            └── dummy.device.group.d_tests.assert\n',
            '--no-detailed-test-id',
            ''
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'device'),
            'Testsuite\n'
            '├── Samples\n'
            '└── Tests\n'
            '    └── dummy\n'
            '        └── device\n'
            '            └── dummy.device.group.d_tests.assert\n',
            '--detailed-test-id',
            ''
        ),
    ]

    TESTDATA_4 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy'),
            ['qemu_x86', 'qemu_x86_64', 'intel_adl_crb']
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
        'test_path, expected',
        TESTDATA_1,
        ids=[
            'tests/dummy/agnostic',
            'tests/dummy/device',
        ]
    )
    def test_list_tags(self, capfd, out_path, test_path, expected):
        args = ['--outdir', out_path, '-T', test_path, '--list-tags']

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

    @pytest.mark.parametrize(
        'test_path, expected, detailed_id, exclude_tags',
        TESTDATA_2,
        ids=[
            'tests/dummy/agnostic no_detailed_id',
            'tests/dummy/agnostic no_detailed_id excl_tag',
            'tests/dummy/agnostic no_detailed_id excl_all_tags',
            'tests/dummy/agnostic no_detailed_id no_excl_tag',
            'tests/dummy/agnostic detailed_id',
            'tests/dummy/agnostic detailed_id excl_tag',
            'tests/dummy/agnostic detailed_id excl_all_tags',
            'tests/dummy/agnostic detailed_id no_excl_tag',
            'tests/dummy/device',
            'tests/dummy/device detailed_id',
        ]
    )
    def test_list_tests(self, capfd, out_path, test_path, expected, detailed_id, exclude_tags):
        args = ['--outdir', out_path, '-T', test_path, '--list-tests', detailed_id]
        for tag in exclude_tags.split():
            args += ['--exclude-tag', tag]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        printed_tests = [test.strip() for test in out.split('- ')[1:]]
        if printed_tests:
            printed_tests[-1] = printed_tests[-1].split('\n')[0]

        assert all([test in printed_tests for test in expected])
        assert all([test in expected for test in printed_tests])

        assert str(sys_exit.value) == '0'

    @pytest.mark.parametrize(
        'test_path, expected, detailed_id, exclude_tags',
        TESTDATA_3,
        ids=[
            'tests/dummy/agnostic no_detailed_id',
            'tests/dummy/agnostic no_detailed_id excl_tag',
            'tests/dummy/agnostic detailed_id',
            'tests/dummy/agnostic detailed_id excl_tag',
            'tests/dummy/agnostic detailed_id excl_all_tags',
            'tests/dummy/agnostic detailed_id no_excl_tag',
            'tests/dummy/device',
            'tests/dummy/device detailed_id',
        ]
    )
    def test_tree(self, capfd, out_path, test_path, expected, detailed_id, exclude_tags):
        args = ['--outdir', out_path, '-T', test_path, '--test-tree', detailed_id]
        for tag in exclude_tags.split():
            args += ['--exclude-tag', tag]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        assert expected in out
        assert str(sys_exit.value) == '0'

    @pytest.mark.parametrize(
        'test_path, test_platforms',
        TESTDATA_4,
        ids=['tests']
    )
    def test_timestamps(self, capfd, out_path, test_path, test_platforms):

        args = ['-i', '--outdir', out_path, '-T', test_path, '--timestamps', '-v'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'

        info_regex = r'\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2},\d{3} - (?:INFO|DEBUG|ERROR)'

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        output = err.split('\n')

        # Will give false positives on lines with newlines inside of them
        err_lines = []
        for line in output:
            if line.strip():

                match = re.search(info_regex, line)
                if match is None:
                    err_lines.append(line)

        if err_lines:
            assert False, f'No timestamp in line {err_lines}'

    @pytest.mark.parametrize(
        'flag',
        ['--abcd', '--1234', '-%', '-1']
    )
    def test_broken_parameter(self, capfd, flag):

        args = [flag]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        if flag == '-1':
            assert str(sys_exit.value) == '1'
        else:
            assert str(sys_exit.value) == '2'

    @pytest.mark.parametrize(
        'flag',
        ['--help', '-h']
    )
    def test_help(self, capfd, flag):
        args = [flag]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        assert str(sys_exit.value) == '0'

    @pytest.mark.parametrize(
        'test_path, test_platforms',
        TESTDATA_4,
        ids=['tests']
    )
    def test_force_color(self, capfd, out_path, test_path, test_platforms):

        args = ['-i', '--outdir', out_path, '-T', test_path, '--force-color'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        assert str(sys_exit.value) == '0'

    @mock.patch.object(TestPlan, 'SAMPLE_FILENAME', sample_filename_mock)
    def test_size(self, capfd, out_path):
        test_platforms = ['qemu_x86', 'intel_adl_crb']
        path = os.path.join(TEST_DATA, 'samples', 'hello_world')
        args = ['-i', '--outdir', out_path, '-T', path] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'

        clear_log_in_test()
        capfd.readouterr()

        p = os.path.relpath(path, ZEPHYR_BASE)
        prev_path = os.path.join(out_path, 'qemu_x86_atom', 'zephyr', p,
                                 'sample.basic.helloworld', 'zephyr', 'zephyr.elf')
        args = ['--size', prev_path]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        # Header and footer should be the most constant out of the report format.
        header_pattern = r'SECTION NAME\s+VMA\s+LMA\s+SIZE\s+HEX SZ\s+TYPE\s*\n'
        res = re.search(header_pattern, out)
        assert res, 'No stdout size report header found.'

        footer_pattern = r'Totals:\s+(?P<rom>[0-9]+)\s+bytes\s+\(ROM\),\s+' \
                         r'(?P<ram>[0-9]+)\s+bytes\s+\(RAM\)\s*\n'
        res = re.search(footer_pattern, out)
        assert res, 'No stdout size report footer found.'
