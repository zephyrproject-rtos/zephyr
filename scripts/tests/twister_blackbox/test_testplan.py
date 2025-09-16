#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Blackbox tests for twister's command line functions - those requiring testplan.json
"""

from unittest import mock
import os
import pytest
import json

# pylint: disable=no-name-in-module
from conftest import TEST_DATA, suite_filename_mock
from twisterlib.testplan import TestPlan
from twisterlib.error import TwisterRuntimeError
from twisterlib.twister_main import main as twister_main


class TestTestPlan:
    TESTDATA_1 = [
        ('dummy.agnostic.group2.a2_tests.assert1', None, 4),
        (
            os.path.join('scripts', 'tests', 'twister_blackbox', 'test_data', 'tests',
                         'dummy', 'agnostic', 'group1', 'subgroup1',
                         'dummy.agnostic.group2.assert1'),
            TwisterRuntimeError,
            None
        ),
    ]
    TESTDATA_2 = [
        ('buildable', 7),
        ('runnable', 5),
    ]
    TESTDATA_3 = [
        (True, 1),
        (False, 7),
    ]

    @pytest.mark.parametrize(
        'test, expected_exception, expected_subtest_count',
        TESTDATA_1,
        ids=['valid', 'not found']
    )
    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', suite_filename_mock)
    def test_subtest(self, out_path, test, expected_exception, expected_subtest_count):
        test_platforms = ['qemu_x86', 'intel_adl_crb']
        path = os.path.join(TEST_DATA, 'tests', 'dummy')
        args = ['--detailed-test-id',
                '-i', '--outdir', out_path, '-T', path, '--sub-test', test, '-y'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        if expected_exception:
            with pytest.raises(expected_exception):
                twister_main(args)
        else:
            return_value = twister_main(args)
            with open(os.path.join(out_path, 'testplan.json')) as f:
                j = json.load(f)
            filtered_j = [
                (ts['platform'], ts['name'], tc['identifier']) \
                    for ts in j['testsuites'] \
                    for tc in ts['testcases'] if 'reason' not in tc
            ]

            assert return_value == 0
            assert len(filtered_j) == expected_subtest_count

    @pytest.mark.parametrize(
        'filter, expected_count',
        TESTDATA_2,
        ids=['buildable', 'runnable']
    )
    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', suite_filename_mock)
    def test_filter(self, out_path, filter, expected_count):
        test_platforms = ['qemu_x86', 'intel_adl_crb']
        path = os.path.join(TEST_DATA, 'tests', 'dummy')
        args = ['-i', '--outdir', out_path, '-T', path, '--filter', filter, '-y'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        assert twister_main(args)  == 0

        import pprint
        with open(os.path.join(out_path, 'testplan.json')) as f:
            j = json.load(f)
            pprint.pprint(j)
        filtered_j = [
            (ts['platform'], ts['name'], tc['identifier']) \
                for ts in j['testsuites'] \
                for tc in ts['testcases'] if 'reason' not in tc
        ]
        pprint.pprint(filtered_j)

        assert expected_count == len(filtered_j)

    @pytest.mark.parametrize(
        'integration, expected_count',
        TESTDATA_3,
        ids=['integration', 'no integration']
    )
    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', suite_filename_mock)
    @mock.patch.object(TestPlan, 'SAMPLE_FILENAME', '')
    def test_integration(self, out_path, integration, expected_count):
        test_platforms = ['qemu_x86', 'intel_adl_crb']
        path = os.path.join(TEST_DATA, 'tests', 'dummy')
        args = ['-i', '--outdir', out_path, '-T', path, '-y'] + \
               (['--integration'] if integration else []) + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        assert twister_main(args) == 0

        with open(os.path.join(out_path, 'testplan.json')) as f:
            j = json.load(f)
        filtered_j = [
            (ts['platform'], ts['name'], tc['identifier']) \
                for ts in j['testsuites'] \
                for tc in ts['testcases'] if 'reason' not in tc
        ]

        assert expected_count == len(filtered_j)
