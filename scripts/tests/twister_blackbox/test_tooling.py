#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Blackbox tests for twister's command line functions related to Twister's tooling.
"""
# pylint: disable=duplicate-code

import importlib
import mock
import os
import pytest
import sys
import json

from conftest import ZEPHYR_BASE, TEST_DATA, sample_filename_mock, testsuite_filename_mock
from twisterlib.testplan import TestPlan


class TestTooling:
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
        'jobs',
        ['1', '2'],
        ids=['single job', 'two jobs']
    )
    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', testsuite_filename_mock)
    def test_jobs(self, out_path, jobs):
        test_platforms = ['qemu_x86', 'frdm_k64f']
        path = os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic', 'group2')
        args = ['-i', '--outdir', out_path, '-T', path] + \
               ['--jobs', jobs] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        with open(os.path.join(out_path, 'twister.log')) as f:
            log = f.read()
            assert f'JOBS: {jobs}' in log

        assert str(sys_exit.value) == '0'

    @mock.patch.object(TestPlan, 'SAMPLE_FILENAME', sample_filename_mock)
    def test_force_toolchain(self, out_path):
        # nsim_vpx5 is one of the rare platforms that do not support the zephyr toolchain
        test_platforms = ['nsim/nsim_vpx5']
        path = os.path.join(TEST_DATA, 'samples', 'hello_world')
        args = ['-i', '--outdir', out_path, '-T', path, '-y'] + \
               ['--force-toolchain'] + \
               [] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'

        with open(os.path.join(out_path, 'testplan.json')) as f:
            j = json.load(f)
        filtered_j = [
            (ts['platform'], ts['name'], tc['identifier'], tc['status']) \
                for ts in j['testsuites'] \
                for tc in ts['testcases']
        ]

        # Normally, board not supporting our toolchain would be filtered, so we check against that
        assert len(filtered_j) == 1
        assert filtered_j[0][3] != 'filtered'

    @pytest.mark.parametrize(
        'test_path, test_platforms',
        [
            (
                os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
                ['qemu_x86'],
            )
        ],
        ids=[
            'ninja',
        ]
    )
    @pytest.mark.parametrize(
        'flag',
        ['--ninja', '-N']
    )
    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', testsuite_filename_mock)
    def test_ninja(self, capfd, out_path, test_path, test_platforms, flag):
        args = ['--outdir', out_path, '-T', test_path, flag] + \
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
