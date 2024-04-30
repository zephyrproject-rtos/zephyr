#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Blackbox tests for twister's command line functions changing test output.
"""

import importlib
import re
import mock
import os
import pytest
import sys
import json

# pylint: disable=no-name-in-module
from conftest import ZEPHYR_BASE, TEST_DATA, testsuite_filename_mock, clear_log_in_test
from twisterlib.testplan import TestPlan


@mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', testsuite_filename_mock)
class TestOutput:
    TESTDATA_1 = [
    (
        os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic')
    ),
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
        'flag, expect_paths',
        [
            ('--no-detailed-test-id', False),
            ('--detailed-test-id', True)
        ],
        ids=['no-detailed-test-id', 'detailed-test-id']
    )
    def test_detailed_test_id(self, out_path, flag, expect_paths):
        test_platforms = ['qemu_x86', 'frdm_k64f']
        path = os.path.join(TEST_DATA, 'tests', 'dummy')
        args = ['-i', '--outdir', out_path, '-T', path, '-y'] + \
               [flag] + \
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
            (ts['platform'], ts['name'], tc['identifier']) \
                for ts in j['testsuites'] \
                for tc in ts['testcases'] if 'reason' not in tc
        ]

        assert len(filtered_j) > 0, "No dummy tests found."

        expected_start = os.path.relpath(TEST_DATA, ZEPHYR_BASE) if expect_paths else 'dummy.'
        assert all([testsuite.startswith(expected_start)for _, testsuite, _ in filtered_j])

    def test_inline_logs(self, out_path):
        test_platforms = ['qemu_x86', 'frdm_k64f']
        path = os.path.join(TEST_DATA, 'tests', 'always_build_error', 'dummy')
        args = ['--outdir', out_path, '-T', path] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '1'

        rel_path = os.path.relpath(path, ZEPHYR_BASE)
        build_path = os.path.join(out_path, 'qemu_x86', rel_path, 'always_fail.dummy', 'build.log')
        with open(build_path) as f:
            build_log = f.read()

        clear_log_in_test()

        args = ['--outdir', out_path, '-T', path] + \
               ['--inline-logs'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '1'

        with open(os.path.join(out_path, 'twister.log')) as f:
            inline_twister_log = f.read()

        # Remove information that differs between the runs
        removal_patterns = [
            # Remove tmp filepaths, as they will differ
            r'(/|\\)tmp(/|\\)\S+',
            # Remove object creation order, as it can change
            r'^\[[0-9]+/[0-9]+\] ',
            # Remove variable CMake flag
            r'-DTC_RUNID=[0-9a-zA-Z]+',
            # Remove variable order CMake flags
            r'-I[0-9a-zA-Z/\\]+',
            # Remove duration-sensitive entries
            r'-- Configuring done \([0-9.]+s\)',
            r'-- Generating done \([0-9.]+s\)',
            # Cache location may vary between CI runs
            r'^.*-- Cache files will be written to:.*$'
        ]
        for pattern in removal_patterns:
            c_pattern = re.compile(pattern, flags=re.MULTILINE)
            inline_twister_log = re.sub(c_pattern, '', inline_twister_log)
            build_log = re.sub(c_pattern, '', build_log)

        split_build_log = build_log.split('\n')
        for r in split_build_log:
            assert r in inline_twister_log

    def _get_matches(self, err, regex_line):
        matches = []
        for line in err.split('\n'):
            columns = line.split()
            if len(columns) == 8:
                for i in range(8):
                    match = re.fullmatch(regex_line[i], columns[i])
                    if match:
                        matches.append(match)
                if len(matches) == 8:
                    return matches
                else:
                    matches = []
        return matches

    @pytest.mark.parametrize(
        'test_path',
        TESTDATA_1,
        ids=[
            'single_v',
        ]
    )
    def test_single_v(self, capfd, out_path, test_path):
        args = ['--outdir', out_path, '-T', test_path, '-v']

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
            pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)
        regex_line = [r'INFO', r'-', r'\d+/\d+', r'\S+', r'\S+', r'[A-Z]+', r'\(\w+', r'[\d.]+s\)']
        matches = self._get_matches(err, regex_line)
        print(matches)
        assert str(sys_exit.value) == '0'
        assert len(matches) > 0

    @pytest.mark.parametrize(
        'test_path',
        TESTDATA_1,
        ids=[
            'double_v',
        ]
    )
    def test_double_v(self, capfd, out_path, test_path):
        args = ['--outdir', out_path, '-T', test_path, '-vv']

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
            pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)
        regex_line = [r'INFO', r'-', r'\d+/\d+', r'\S+', r'\S+', r'[A-Z]+', r'\(\w+', r'[\d.]+s\)']
        matches = self._get_matches(err, regex_line)
        booting_zephyr_regex = re.compile(r'^DEBUG\s+-\s+([^*]+)\*\*\*\s+Booting\s+Zephyr\s+OS\s+build.*$', re.MULTILINE)
        info_debug_line_regex = r'^\s*(INFO|DEBUG)'

        assert str(sys_exit.value) == '0'
        assert re.search(booting_zephyr_regex, err) is not None
        assert re.search(info_debug_line_regex, err) is not None
        assert len(matches) > 0
