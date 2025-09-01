#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Blackbox tests for twister's command line functions related to disable features.
"""

import pytest
from unittest import mock
import os
import sys
import re

# pylint: disable=no-name-in-module
from conftest import TEST_DATA, suite_filename_mock
from twisterlib.testplan import TestPlan
from twisterlib.twister_main import main as twister_main


@mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', suite_filename_mock)
class TestDisable:
    TESTDATA_1 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86'],
            '--disable-suite-name-check',
            [r"Expected suite names:\[['\w+'\[,\s]*\]", r"Detected suite names:\[['\w+'\[,\s]*\]"],
            True
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86'],
            '-v',
            [r"Expected suite names:\[['(\w+)'[, ]*]+", r"Detected suite names:\[['(\w+)'[, ]*]+"],
            False
        ),
    ]
    TESTDATA_2 = [
        (
            os.path.join(TEST_DATA, 'tests', 'always_warning'),
            ['qemu_x86'],
            '--disable-warnings-as-errors',
            0
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'always_warning'),
            ['qemu_x86'],
            '-v',
            1
        ),
    ]


    @pytest.mark.parametrize(
        'test_path, test_platforms, flag, expected, expected_none',
        TESTDATA_1,
        ids=[
            'disable-suite-name-check',
            'suite-name-check'
        ],
    )

    def test_disable_suite_name_check(self, capfd, out_path, test_path, test_platforms, flag, expected, expected_none):
        args = ['-i', '--outdir', out_path, '-T', test_path] + \
               [flag] + \
               ['-vv', '-ll', 'DEBUG'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]


        return_value = twister_main(args)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        assert return_value == 0
        if expected_none:
            assert re.search(expected[0], err) is None, f"Not expected string in log: {expected[0]}"
            assert re.search(expected[1], err) is None, f"Not expected: {expected[1]}"
        else:
            assert re.search(expected[0], err) is not None, f"Expected string in log: {expected[0]}"
            assert re.search(expected[1], err) is not None, f"Expected string in log: {expected[1]}"


    @pytest.mark.parametrize(
        'test_path, test_platforms, flag, expected_exit_code',
        TESTDATA_2,
        ids=[
            'disable-warnings-as-errors',
            'warnings-as-errors'
        ],
    )

    def test_disable_warnings_as_errors(self, capfd, out_path, test_path, test_platforms, flag, expected_exit_code):
        args = ['-i', '--outdir', out_path, '-T', test_path] + \
               [flag] + \
               ['-vv'] + \
               ['--build-only'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]


        return_value = twister_main(args)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        assert return_value == expected_exit_code, \
            f"Twister return not expected ({expected_exit_code}) exit code: ({return_value})"
