#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Blackbox tests for twister's command line functions related to the quarantine.
"""

import importlib
import mock
import os
import pytest
import re
import sys
import json

from conftest import ZEPHYR_BASE, TEST_DATA, testsuite_filename_mock
from twisterlib.testplan import TestPlan


class TestQuarantine:
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
    def test_quarantine_verify(self, out_path):
        test_platforms = ['qemu_x86', 'intel_adl_crb']
        path = os.path.join(TEST_DATA, 'tests', 'dummy')
        quarantine_path = os.path.join(TEST_DATA, 'twister-quarantine-list.yml')
        args = ['-i', '--outdir', out_path, '-T', path, '-y'] + \
               ['--quarantine-verify'] + \
               ['--quarantine-list', quarantine_path] + \
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

        assert len(filtered_j) == 2

    @pytest.mark.parametrize(
        'test_path, test_platforms, quarantine_directory',
        [
            (
                os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
                ['qemu_x86', 'qemu_x86_64', 'intel_adl_crb'],
                os.path.join(TEST_DATA, 'twister-quarantine-list.yml'),
            ),
        ],
        ids=[
            'quarantine',
        ],
    )
    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', testsuite_filename_mock)
    def test_quarantine_list(self, capfd, out_path, test_path, test_platforms, quarantine_directory):
        args = ['--outdir', out_path, '-T', test_path] +\
               ['--quarantine-list', quarantine_directory] + \
               ['-vv'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
            pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        frdm_match = re.search('agnostic/group2/dummy.agnostic.group2 SKIPPED: Quarantine: test '
                               'intel_adl_crb', err)
        frdm_match2 = re.search(
            'agnostic/group1/subgroup2/dummy.agnostic.group1.subgroup2 SKIPPED: Quarantine: test '
            'intel_adl_crb',
            err)
        qemu_64_match = re.search(
            'agnostic/group1/subgroup2/dummy.agnostic.group1.subgroup2 SKIPPED: Quarantine: test '
            'qemu_x86_64',
            err)
        all_platforms_match = re.search(
            'agnostic/group1/subgroup1/dummy.agnostic.group1.subgroup1 SKIPPED: Quarantine: test '
            'all platforms',
            err)
        all_platforms_match2 = re.search(
            'agnostic/group1/subgroup1/dummy.agnostic.group1.subgroup1 SKIPPED: Quarantine: test '
            'all platforms',
            err)
        all_platforms_match3 = re.search(
            'agnostic/group1/subgroup1/dummy.agnostic.group1.subgroup1 SKIPPED: Quarantine: test '
            'all platforms',
            err)

        assert frdm_match and frdm_match2, 'platform quarantine not work properly'
        assert qemu_64_match, 'platform quarantine on scenario not work properly'
        assert all_platforms_match and all_platforms_match2 and all_platforms_match3, 'scenario ' \
                                                                                      'quarantine' \
                                                                                      ' not work ' \
                                                                                      'properly'

        assert str(sys_exit.value) == '0'
