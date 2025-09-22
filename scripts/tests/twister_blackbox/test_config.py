#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Blackbox tests for twister's command line functions related to test configuration files.
"""

import importlib
import mock
import os
import pytest
import sys
import json

# pylint: disable=no-name-in-module
from conftest import ZEPHYR_BASE, TEST_DATA, testsuite_filename_mock
from twisterlib.testplan import TestPlan


class TestConfig:
    @classmethod
    def setup_class(cls):
        apath = os.path.join(ZEPHYR_BASE, 'scripts', 'twister')
        cls.loader = importlib.machinery.SourceFileLoader('__main__', apath)
        cls.spec = importlib.util.spec_from_loader(cls.loader.name, cls.loader)
        cls.twister_module = importlib.util.module_from_spec(cls.spec)

    @classmethod
    def teardown_class(cls):
        pass

    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', testsuite_filename_mock)
    def test_alt_config_root(self, out_path):
        test_platforms = ['qemu_x86', 'intel_adl_crb']
        path = os.path.join(TEST_DATA, 'tests', 'dummy')
        alt_config_root = os.path.join(TEST_DATA, 'alt-test-configs', 'dummy')
        args = ['-i', '--outdir', out_path, '-T', path, '-y'] + \
               ['--alt-config-root', alt_config_root] + \
               ['--tag', 'alternate-config-root'] + \
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

        assert len(filtered_j) == 4

    @pytest.mark.parametrize(
        'level, expected_tests',
        [
            ('smoke', 6),
            ('acceptance', 7),
        ],
        ids=['smoke', 'acceptance']
    )
    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', testsuite_filename_mock)
    def test_level(self, out_path, level, expected_tests):
        test_platforms = ['qemu_x86', 'intel_adl_crb']
        path = os.path.join(TEST_DATA, 'tests', 'dummy')
        config_path = os.path.join(TEST_DATA, 'test_config.yaml')
        args = ['-i','--outdir', out_path, '-T', path, '--level', level, '-y',
                '--test-config', config_path] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)
        import pprint
        with open(os.path.join(out_path, 'testplan.json')) as f:
            j = json.load(f)
            pprint.pprint(j)
        filtered_j = [
            (ts['platform'], ts['name'], tc['identifier']) \
                for ts in j['testsuites'] \
                for tc in ts['testcases'] if 'reason' not in tc
        ]

        assert str(sys_exit.value) == '0'

        assert expected_tests == len(filtered_j)
