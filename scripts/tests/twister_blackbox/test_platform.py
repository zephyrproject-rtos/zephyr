#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Blackbox tests for twister's command line functions related to Zephyr platforms.
"""

import importlib
import re
import mock
import os
import pytest
import sys
import json

from conftest import ZEPHYR_BASE, TEST_DATA, testsuite_filename_mock
from twisterlib.testplan import TestPlan


class TestPlatform:
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
        'board_root, expected_returncode',
        [(True, '0'), (False, '2')],
        ids=['dummy in additional board root', 'no additional board root, crash']
    )
    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', testsuite_filename_mock)
    def test_board_root(self, out_path, board_root, expected_returncode):
        test_platforms = ['qemu_x86', 'dummy_board/dummy_soc']
        board_root_path = os.path.join(TEST_DATA, 'boards')
        path = os.path.join(TEST_DATA, 'tests', 'dummy')
        args = ['-i', '--outdir', out_path, '-T', path, '-y'] + \
               (['--board-root', board_root_path] if board_root else []) + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        # Checking twister.log increases coupling,
        # but we need to differentiate crashes.
        with open(os.path.join(out_path, 'twister.log')) as f:
            log = f.read()
            error_regex = r'ERROR.*platform_filter\s+-\s+unrecognized\s+platform\s+-\s+dummy_board/dummy_soc$'
            board_error = re.search(error_regex, log)
            assert board_error if not board_root else not board_error

        assert str(sys_exit.value) == expected_returncode

    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', testsuite_filename_mock)
    def test_force_platform(self, out_path):
        test_platforms = ['qemu_x86', 'frdm_k64f']
        path = os.path.join(TEST_DATA, 'tests', 'dummy')
        args = ['-i', '--outdir', out_path, '-T', path, '-y'] + \
               ['--force-platform'] + \
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

        assert len(filtered_j) == 12

    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', testsuite_filename_mock)
    def test_platform(self, out_path):
        path = os.path.join(TEST_DATA, 'tests', 'dummy')
        args = ['-i', '--outdir', out_path, '-T', path, '-y'] + \
               ['--platform', 'qemu_x86']

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

        assert all([platform == 'qemu_x86' for platform, _, _ in filtered_j])
