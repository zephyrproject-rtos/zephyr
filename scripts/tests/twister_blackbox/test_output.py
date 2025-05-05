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
        ([]),
        (['-ll', 'DEBUG']),
        (['-v']),
        (['-v', '-ll', 'DEBUG']),
        (['-vv']),
        (['-vv', '-ll', 'DEBUG']),
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
        test_platforms = ['qemu_x86', 'intel_adl_crb']
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
        assert all([testsuite.startswith(expected_start) for _, testsuite, _ in filtered_j])
        if expect_paths:
            assert all([(tc_name.count('.') > 1) for _, _, tc_name in filtered_j])
        else:
            assert all([(tc_name.count('.') == 1) for _, _, tc_name in filtered_j])


    def test_inline_logs(self, out_path):
        test_platforms = ['qemu_x86', 'intel_adl_crb']
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
        build_path = os.path.join(out_path, 'qemu_x86_atom', 'zephyr', rel_path, 'always_fail.dummy', 'build.log')
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
            r'^.*-- Cache files will be written to:.*$',
            # List of built C object may differ between runs.
            # See: Issue #87769.
            # Probable culprits: the cache mechanism, build error
            r'^Building C object .*$'
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
            regexes = len(regex_line)
            if len(columns) == regexes:
                for i, column in enumerate(columns):
                    match = re.fullmatch(regex_line[i], column)
                    if match:
                        matches.append(match)
                if len(matches) == regexes:
                    return matches
                else:
                    matches = []
        return matches


    @pytest.mark.parametrize(
        'flags',
        TESTDATA_1,
        ids=['not verbose', 'not verbose + debug', 'v', 'v + debug', 'vv', 'vv + debug']
    )
    def test_output_levels(self, capfd, out_path, flags):
        test_path = os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic')
        args = ['--outdir', out_path, '-T', test_path, *flags]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
            pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        assert str(sys_exit.value) == '0'

        regex_debug_line = r'^\s*DEBUG'
        debug_matches = re.search(regex_debug_line, err, re.MULTILINE)
        if '-ll' in flags and 'DEBUG' in flags:
            assert debug_matches is not None
        else:
            assert debug_matches is None

        # Summary requires verbosity > 1
        if '-vv' in flags:
            assert 'Total test suites: ' in out
        else:
            assert 'Total test suites: ' not in out

        # Brief summary shows up only on verbosity 0 - instance-by-instance otherwise
        regex_info_line = [r'INFO', r'-', r'\d+/\d+', r'\S+', r'\S+', r'[A-Z]+', r'\(\w+', r'[\d.]+s', r'<\S+>\)']
        info_matches = self._get_matches(err, regex_info_line)
        if not any(f in flags for f in ['-v', '-vv']):
            assert not info_matches
        else:
            assert info_matches
