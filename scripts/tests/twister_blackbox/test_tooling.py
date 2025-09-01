#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Blackbox tests for twister's command line functions related to Twister's tooling.
"""
# pylint: disable=duplicate-code

from unittest import mock
import os
import pytest
import sys
import json

# pylint: disable=no-name-in-module
from conftest import TEST_DATA, sample_filename_mock, suite_filename_mock
from twisterlib.statuses import TwisterStatus
from twisterlib.testplan import TestPlan
from twisterlib.twister_main import main as twister_main


class TestTooling:

    @pytest.mark.parametrize(
        'jobs',
        ['1', '2'],
        ids=['single job', 'two jobs']
    )
    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', suite_filename_mock)
    def test_jobs(self, out_path, jobs):
        test_platforms = ['qemu_x86', 'intel_adl_crb']
        path = os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic', 'group2')
        args = ['-i', '--outdir', out_path, '-T', path] + \
               ['--jobs', jobs] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]


        return_value = twister_main(args)

        with open(os.path.join(out_path, 'twister.log')) as f:
            log = f.read()
            assert f'JOBS: {jobs}' in log

        assert return_value == 0

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

        return_value = twister_main(args)

        with open(os.path.join(out_path, 'testplan.json')) as f:
            j = json.load(f)
        filtered_j = [
            (ts['platform'], ts['name'], tc['identifier'], tc['status']) \
                for ts in j['testsuites'] \
                for tc in ts['testcases']
        ]

        # Normally, board not supporting our toolchain would be filtered, so we check against that
        assert len(filtered_j) == 1
        assert filtered_j[0][3] != TwisterStatus.FILTER

        assert return_value == 0

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
    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', suite_filename_mock)
    def test_ninja(self, capfd, out_path, test_path, test_platforms, flag):
        args = ['--outdir', out_path, '-T', test_path, flag] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        return_value = twister_main(args)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        assert return_value == 0
