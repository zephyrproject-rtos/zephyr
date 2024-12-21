#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Blackbox tests for twister's command line functions related to test filtering.
"""

import importlib
import mock
import os
import pytest
import sys
import re

# pylint: disable=no-name-in-module
from conftest import ZEPHYR_BASE, TEST_DATA, testsuite_filename_mock
from twisterlib.testplan import TestPlan


class TestDevice:
    TESTDATA_1 = [
        (
            1234,
        ),
        (
            4321,
        ),
        (
            1324,
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
        'seed',
        TESTDATA_1,
        ids=[
            'seed 1234',
            'seed 4321',
            'seed 1324'
        ],
    )

    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', testsuite_filename_mock)
    def test_seed(self, capfd, out_path, seed):
        test_platforms = ['native_sim']
        path = os.path.join(TEST_DATA, 'tests', 'seed_native_sim')
        args = ['--outdir', out_path, '-i', '-T', path, '-vv',] + \
               ['--seed', f'{seed[0]}'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        assert str(sys_exit.value) == '1'

        expected_line = r'seed_native_sim.dummy FAILED Failed \(rc=1\) \(native (\d+\.\d+)s/seed: {}\)'.format(seed[0])
        assert re.search(expected_line, err)
