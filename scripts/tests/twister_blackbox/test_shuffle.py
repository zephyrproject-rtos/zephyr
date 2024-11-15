#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Blackbox tests for twister's command line functions related to the shuffling of the test order.
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


class TestShuffle:
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
        'seed, ratio, expected_order',
        [
            ('123', '1/2', ['dummy.agnostic.group1.subgroup1', 'dummy.agnostic.group1.subgroup2']),
            ('123', '2/2', ['dummy.agnostic.group2', 'dummy.device.group']),
            ('321', '1/2', ['dummy.agnostic.group1.subgroup1', 'dummy.agnostic.group2']),
            ('321', '2/2', ['dummy.device.group', 'dummy.agnostic.group1.subgroup2']),
            ('123', '1/3', ['dummy.agnostic.group1.subgroup1', 'dummy.agnostic.group1.subgroup2']),
            ('123', '2/3', ['dummy.agnostic.group2']),
            ('123', '3/3', ['dummy.device.group']),
            ('321', '1/3', ['dummy.agnostic.group1.subgroup1', 'dummy.agnostic.group2']),
            ('321', '2/3', ['dummy.device.group']),
            ('321', '3/3', ['dummy.agnostic.group1.subgroup2'])
        ],
        ids=['first half, 123', 'second half, 123', 'first half, 321', 'second half, 321',
             'first third, 123', 'middle third, 123', 'last third, 123',
             'first third, 321', 'middle third, 321', 'last third, 321']
    )
    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', testsuite_filename_mock)
    def test_shuffle_tests(self, out_path, seed, ratio, expected_order):
        test_platforms = ['qemu_x86', 'intel_adl_crb']
        path = os.path.join(TEST_DATA, 'tests', 'dummy')
        args = ['-i', '--outdir', out_path, '-T', path, '-y'] + \
               ['--shuffle-tests', '--shuffle-tests-seed', seed] + \
               ['--subset', ratio] + \
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

        testcases = [re.sub(r'\.assert[^\.]*?$', '', j[2]) for j in filtered_j]
        testsuites = list(dict.fromkeys(testcases))

        assert testsuites == expected_order

        assert str(sys_exit.value) == '0'
