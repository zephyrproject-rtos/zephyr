#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Blackbox tests for twister's command line functions related to test filtering.
"""

from unittest import mock
import os
import pytest
import sys
import re

# pylint: disable=no-name-in-module
from conftest import TEST_DATA, suite_filename_mock
from twisterlib.testplan import TestPlan
from twisterlib.twister_main import main as twister_main


class TestDevice:

    @pytest.mark.parametrize(
        'seed',
        [1234, 4321, 1324],
    )
    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', suite_filename_mock)
    def test_seed(self, capfd, out_path, seed):
        test_platforms = ['native_sim']
        path = os.path.join(TEST_DATA, 'tests', 'seed_native_sim')

        args = [
            '--no-detailed-test-id', '--outdir', out_path, '-i', '-T', path, '-vv',
            '--seed', f'{seed}',
            *[val for pair in zip(['-p'] * len(test_platforms), test_platforms) for val in pair]
        ]

        return_value = twister_main(args)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        assert return_value == 1

        expected_line = r'seed_native_sim.dummy\s+FAILED rc=1 \(native (\d+\.\d+)s/seed: {} <host>\)'.format(seed)
        assert re.search(expected_line, err), f'Regex not found: r"{expected_line}"'
