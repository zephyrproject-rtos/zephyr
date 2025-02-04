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

# pylint: disable=no-name-in-module
from conftest import ZEPHYR_BASE, TEST_DATA, testsuite_filename_mock
from twisterlib.testplan import TestPlan


@mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', testsuite_filename_mock)
class TestPlatform:
    TESTDATA_1 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86', 'qemu_x86_64', 'intel_adl_crb'],
            {
                'selected_test_scenarios': 3,
                'selected_test_instances': 9,
                'executed_test_instances': 6,
                'skipped_configurations': 3,
                'skipped_by_static_filter': 3,
                'skipped_at_runtime': 0,
                'passed_configurations': 4,
                'built_configurations': 2,
                'failed_configurations': 0,
                'errored_configurations': 0,
                'executed_test_cases': 8,
                'skipped_test_cases': 2,
                'platform_count': 2,
                'executed_on_platform': 4,
                'only_built': 2
            }
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'device'),
            ['qemu_x86', 'qemu_x86_64', 'intel_adl_crb'],
            {
                'selected_test_scenarios': 1,
                'selected_test_instances': 3,
                'executed_test_instances': 0,
                'skipped_configurations': 3,
                'skipped_by_static_filter': 3,
                'skipped_at_runtime': 0,
                'passed_configurations': 0,
                'built_configurations': 0,
                'failed_configurations': 0,
                'errored_configurations': 0,
                'executed_test_cases': 0,
                'skipped_test_cases': 0,
                'platform_count': 0,
                'executed_on_platform': 0,
                'only_built': 0
            }
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
        'board_root, expected_returncode',
        [(True, '0'), (False, '2')],
        ids=['dummy in additional board root', 'no additional board root, crash']
    )
    def test_board_root(self, out_path, board_root, expected_returncode):
        test_platforms = ['qemu_x86', 'dummy/unit_testing']
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
            error_regex = r'ERROR.*platform_filter\s+-\s+unrecognized\s+platform\s+-\s+dummy/unit_testing$'
            board_error = re.search(error_regex, log)
            assert board_error if not board_root else not board_error

        assert str(sys_exit.value) == expected_returncode

    def test_force_platform(self, out_path):
        test_platforms = ['qemu_x86', 'intel_adl_crb']
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

        assert all([platform == 'qemu_x86/atom' for platform, _, _ in filtered_j])

    @pytest.mark.parametrize(
        'test_path, test_platforms',
        [
            (
                os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
                ['qemu_x86'],
            ),
        ],
        ids=[
            'any_platform',
        ],
    )
    @pytest.mark.parametrize(
        'flag',
        ['-l', '--all']
    )
    def test_any_platform(self, capfd, out_path, test_path, test_platforms, flag):
        args = ['--outdir', out_path, '-T', test_path, '-y'] + \
               [flag] + \
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

    @pytest.mark.parametrize(
        'test_path, test_platforms, expected',
        [
            (
                os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
                ['qemu_x86', 'qemu_x86_64'],
                {
                    'selected_test_instances': 6,
                    'passed_configurations': 2,
                    'executed_test_instances': 3,
                    'executed_on_platform': 2,
                    'only_built': 1,
                }
            ),
        ],
        ids=[
            'exclude_platform',
        ],
    )
    def test_exclude_platform(self, capfd, out_path, test_path, test_platforms, expected):
        args = ['--outdir', out_path, '-T', test_path] + \
               ['--exclude-platform', "qemu_x86"] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
            pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        pass_regex = r'^INFO    - (?P<passed_configurations>[0-9]+) of' \
                     r' (?P<test_instances>[0-9]+) executed test configurations passed'

        built_regex = r'^INFO    - (?P<executed_on_platform>[0-9]+)' \
                      r' test configurations executed on platforms, (?P<only_built>[0-9]+)' \
                      r' test configurations were only built.$'

        pass_search = re.search(pass_regex, err, re.MULTILINE)

        assert pass_search
        assert int(pass_search.group('passed_configurations')) == \
               expected['passed_configurations']
        assert int(pass_search.group('test_instances')) == \
               expected['executed_test_instances']

        built_search = re.search(built_regex, err, re.MULTILINE)

        assert built_search
        assert int(built_search.group('executed_on_platform')) == \
               expected['executed_on_platform']
        assert int(built_search.group('only_built')) == \
               expected['only_built']

        assert str(sys_exit.value) == '0'

    @pytest.mark.parametrize(
        'test_path, test_platforms, expected',
        TESTDATA_1,
        ids=[
            'emulation_only tests/dummy/agnostic',
            'emulation_only tests/dummy/device',
        ]
    )
    def test_emulation_only(self, capfd, out_path, test_path, test_platforms, expected):
        args = ['-i', '--outdir', out_path, '-T', test_path] + \
               ['--emulation-only'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
            pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        select_regex = r'^INFO    - (?P<test_scenarios>[0-9]+) test scenarios' \
                       r' \((?P<test_instances>[0-9]+) configurations\) selected,' \
                       r' (?P<skipped_configurations>[0-9]+) configurations filtered' \
                       r' \((?P<skipped_by_static_filter>[0-9]+) by static filter,' \
                       r' (?P<skipped_at_runtime>[0-9]+) at runtime\)\.$'

        pass_regex = r'^INFO    - (?P<passed_configurations>[0-9]+) of' \
                     r' (?P<executed_test_instances>[0-9]+) executed test configurations passed' \
                     r' \([0-9]+\.[0-9]+%\), (?P<built_configurations>[0-9]+) built \(not run\),' \
                     r' (?P<failed_configurations>[0-9]+) failed,' \
                     r' (?P<errored_configurations>[0-9]+) errored,' \
                     r' with (?:[0-9]+|no) warnings in [0-9]+\.[0-9]+ seconds.$'

        case_regex = r'^INFO    - (?P<passed_cases>[0-9]+) of' \
                     r' (?P<executed_test_cases>[0-9]+) executed test cases passed' \
                     r' \([0-9]+\.[0-9]+%\)' \
                     r'(?:, (?P<blocked_cases>[0-9]+) blocked)?' \
                     r'(?:, (?P<failed_cases>[0-9]+) failed)?' \
                     r'(?:, (?P<errored_cases>[0-9]+) errored)?' \
                     r'(?:, (?P<none_cases>[0-9]+) without a status)?' \
                     r' on (?P<platform_count>[0-9]+) out of total' \
                     r' (?P<total_platform_count>[0-9]+) platforms \([0-9]+\.[0-9]+%\)'

        skip_regex = r'(?P<skipped_test_cases>[0-9]+) selected test cases not executed:' \
                     r'(?: (?P<skipped_cases>[0-9]+) skipped)?' \
                     r'(?:, (?P<filtered_cases>[0-9]+) filtered)?' \
                     r'.'

        built_regex = r'^INFO    - (?P<executed_on_platform>[0-9]+)' \
                      r' test configurations executed on platforms, (?P<only_built>[0-9]+)' \
                      r' test configurations were only built.$'

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        select_search = re.search(select_regex, err, re.MULTILINE)

        assert select_search
        assert int(select_search.group('test_scenarios')) == \
               expected['selected_test_scenarios']
        assert int(select_search.group('test_instances')) == \
               expected['selected_test_instances']
        assert int(select_search.group('skipped_configurations')) == \
               expected['skipped_configurations']
        assert int(select_search.group('skipped_by_static_filter')) == \
               expected['skipped_by_static_filter']
        assert int(select_search.group('skipped_at_runtime')) == \
               expected['skipped_at_runtime']

        pass_search = re.search(pass_regex, err, re.MULTILINE)

        assert pass_search
        assert int(pass_search.group('passed_configurations')) == \
               expected['passed_configurations']
        assert int(pass_search.group('built_configurations')) == \
               expected['built_configurations']
        assert int(pass_search.group('executed_test_instances')) == \
               expected['executed_test_instances']
        if expected['failed_configurations']:
            assert int(pass_search.group('failed_configurations')) == \
                   expected['failed_configurations']
        if expected['errored_configurations']:
            assert int(pass_search.group('errored_configurations')) == \
                   expected['errored_configurations']

        case_search = re.search(case_regex, err, re.MULTILINE)

        assert case_search
        assert int(case_search.group('executed_test_cases')) == \
               expected['executed_test_cases']
        assert int(case_search.group('platform_count')) == \
               expected['platform_count']

        if expected['skipped_test_cases']:
            skip_search = re.search(skip_regex, err, re.MULTILINE)

            assert skip_search
            assert int(skip_search.group('skipped_test_cases')) == \
                   expected['skipped_test_cases']

        built_search = re.search(built_regex, err, re.MULTILINE)

        assert built_search
        assert int(built_search.group('executed_on_platform')) == \
               expected['executed_on_platform']
        assert int(built_search.group('only_built')) == \
               expected['only_built']

        assert str(sys_exit.value) == '0'
