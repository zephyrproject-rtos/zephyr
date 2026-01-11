#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Blackbox tests for twister's command line functions related to saving and loading a testlist.
"""

from unittest import mock
import os
import json

# pylint: disable=no-name-in-module
from conftest import TEST_DATA, suite_filename_mock, clear_log_in_test
from twisterlib.testplan import TestPlan
from twisterlib.twister_main import main as twister_main


class TestTestlist:

    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', suite_filename_mock)
    def test_save_tests(self, out_path):
        test_platforms = ['qemu_x86', 'intel_adl_crb']
        path = os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic')
        saved_tests_file_path = os.path.realpath(os.path.join(out_path, '..', 'saved-tests.json'))
        args = ['-i', '--outdir', out_path, '-T', path, '-y'] + \
               ['--save-tests', saved_tests_file_path] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        # Save agnostics tests
        assert twister_main(args) == 0

        clear_log_in_test()

        # Load all
        path = os.path.join(TEST_DATA, 'tests', 'dummy')
        args = ['-i', '--outdir', out_path, '-T', path, '-y'] + \
               ['--load-tests', saved_tests_file_path] + \
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

        assert len(filtered_j) == 6
