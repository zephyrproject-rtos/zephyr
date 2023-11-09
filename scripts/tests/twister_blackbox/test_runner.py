#!/usr/bin/env python3
# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Blackbox tests for twister's command line functions
"""

import importlib
import mock
import os
import pytest
import re
import sys
import time

from conftest import TEST_DATA, ZEPHYR_BASE, testsuite_filename_mock, clear_log_in_test
from twisterlib.testplan import TestPlan


@mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', testsuite_filename_mock)
class TestRunner:
    TESTDATA_1 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86', 'qemu_x86_64', 'frdm_k64f'],
            {
                'selected_test_scenarios': 3,
                'selected_test_instances': 9,
                'skipped_configurations': 3,
                'skipped_by_static_filter': 3,
                'skipped_at_runtime': 0,
                'passed_configurations': 6,
                'failed_configurations': 0,
                'errored_configurations': 0,
                'executed_test_cases': 10,
                'skipped_test_cases': 5,
                'platform_count': 3,
                'executed_on_platform': 4,
                'only_built': 2
            }
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'device'),
            ['qemu_x86', 'qemu_x86_64', 'frdm_k64f'],
            {
                'selected_test_scenarios': 1,
                'selected_test_instances': 3,
                'skipped_configurations': 3,
                'skipped_by_static_filter': 3,
                'skipped_at_runtime': 0,
                'passed_configurations': 0,
                'failed_configurations': 0,
                'errored_configurations': 0,
                'executed_test_cases': 0,
                'skipped_test_cases': 3,
                'platform_count': 3,
                'executed_on_platform': 0,
                'only_built': 0
            }
        ),
    ]
    TESTDATA_2 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86', 'qemu_x86_64', 'frdm_k64f'],
            {
                'executed_on_platform': 0,
                'only_built': 6
            }
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'device'),
            ['qemu_x86', 'qemu_x86_64', 'frdm_k64f'],
            {
                'executed_on_platform': 0,
                'only_built': 1
            }
        ),
    ]
    TESTDATA_3 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86', 'qemu_x86_64', 'frdm_k64f'],
            {
                'selected_test_scenarios': 3,
                'selected_test_instances': 9,
                'skipped_configurations': 3,
                'skipped_by_static_filter': 3,
                'skipped_at_runtime': 0,
                'passed_configurations': 4,
                'failed_configurations': 0,
                'errored_configurations': 0,
                'executed_test_cases': 8,
                'skipped_test_cases': 5,
                'platform_count': 0,
                'executed_on_platform': 4,
                'only_built': 2
            }
        )
    ]
    TESTDATA_4 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86'],
        ),
    ]
    TESTDATA_5 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86', 'qemu_x86_64'],
            {
                'passed_configurations': 6,
                'selected_test_instances': 6,
                'executed_on_platform': 0,
                'only_built': 6,
            }
        ),
    ]
    TESTDATA_6 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86'],
            os.path.join(TEST_DATA, "pre_script.sh")
        ),
    ]
    TESTDATA_7 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86', 'qemu_x86_64'],
            {
                'passed_configurations': 3,
                'selected_test_instances': 6,
                'executed_on_platform': 2,
                'only_built': 1,
            }
        ),
    ]
    TESTDATA_8 = [
        (
            os.path.join(TEST_DATA, 'tests', 'always_fail', 'dummy'),
            ['qemu_x86_64'],
            '1',
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'always_fail', 'dummy'),
            ['qemu_x86'],
            '2',
        ),
    ]
    TESTDATA_9 = [
        (
            os.path.join(TEST_DATA, 'tests', 'always_fail', 'dummy'),
            ['qemu_x86'],
            '15',
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'always_fail', 'dummy'),
            ['qemu_x86'],
            '30',
        ),
    ]
    TESTDATA_10 = [
        (
            os.path.join(TEST_DATA, 'tests', 'always_timeout', 'dummy'),
            ['qemu_x86'],
            '2',
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'always_timeout', 'dummy'),
            ['qemu_x86'],
            '0.5',
        ),
    ]
    TESTDATA_11 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86', 'qemu_x86_64', 'frdm_k64f'],
            os.path.join(TEST_DATA, 'twister-quarantine-list.yml'),
        ),
    ]
    TESTDATA_12 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy'),
            ['qemu_x86'],
            ['device'],
            ['dummy.agnostic.group2 SKIPPED: Command line testsuite tag filter',
             'dummy.agnostic.group1.subgroup2 SKIPPED: Command line testsuite tag filter',
             'dummy.agnostic.group1.subgroup1 SKIPPED: Command line testsuite tag filter',
             r'0 of 4 test configurations passed \(0.00%\), 0 failed, 0 errored, 4 skipped'
             ]
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy'),
            ['qemu_x86'],
            ['subgrouped'],
            ['dummy.agnostic.group2 SKIPPED: Command line testsuite tag filter',
             r'2 of 4 test configurations passed \(100.00%\), 0 failed, 0 errored, 2 skipped'
             ]
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy'),
            ['qemu_x86'],
            ['agnostic', 'device'],
            [r'3 of 4 test configurations passed \(100.00%\), 0 failed, 0 errored, 1 skipped']
        ),
    ]
    TESTDATA_13 = [
        (
            os.path.join(TEST_DATA, 'tests', 'one_fail_one_pass'),
            ['qemu_x86'],
            {
                'selected_test_instances': 2,
                'skipped_configurations': 0,
                'passed_configurations': 0,
                'failed_configurations': 1,
                'errored_configurations': 0,
            }
        )
    ]

    TESTDATA_14 = [
        (
            os.path.join(TEST_DATA, 'tests', 'always_build_error'),
            ['qemu_x86_64'],
            '1',
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'always_build_error'),
            ['qemu_x86'],
            '4',
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
        'test_path, test_platforms, expected',
        TESTDATA_1,
        ids=[
            'emulation_only tests/dummy/agnostic',
            'emulation_only tests/dummy/device',
        ]
    )
    def test_emulation_only(self, capfd, out_path, test_path, test_platforms, expected):
        args = ['-i', '--outdir', out_path, '-T', test_path, '--emulation-only'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
            pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        select_regex = r'^INFO    - (?P<test_scenarios>[0-9]+) test scenarios' \
                       r' \((?P<test_instances>[0-9]+) test instances\) selected,' \
                       r' (?P<skipped_configurations>[0-9]+) configurations skipped' \
                       r' \((?P<skipped_by_static_filter>[0-9]+) by static filter,' \
                       r' (?P<skipped_at_runtime>[0-9]+) at runtime\)\.$'

        pass_regex = r'^INFO    - (?P<passed_configurations>[0-9]+) of' \
                     r' (?P<test_instances>[0-9]+) test configurations passed' \
                     r' \([0-9]+\.[0-9]+%\), (?P<failed_configurations>[0-9]+) failed,' \
                     r' (?P<errored_configurations>[0-9]+) errored,' \
                     r' (?P<skipped_configurations>[0-9]+) skipped with' \
                     r' [0-9]+ warnings in [0-9]+\.[0-9]+ seconds$'

        case_regex = r'^INFO    - In total (?P<executed_test_cases>[0-9]+)' \
                     r' test cases were executed, (?P<skipped_test_cases>[0-9]+) skipped' \
                     r' on (?P<platform_count>[0-9]+) out of total [0-9]+ platforms' \
                     r' \([0-9]+\.[0-9]+%\)$'

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
        assert int(pass_search.group('test_instances')) == \
               expected['selected_test_instances']
        assert int(pass_search.group('failed_configurations')) == \
               expected['failed_configurations']
        assert int(pass_search.group('errored_configurations')) == \
               expected['errored_configurations']
        assert int(pass_search.group('skipped_configurations')) == \
               expected['skipped_configurations']

        case_search = re.search(case_regex, err, re.MULTILINE)

        assert case_search
        assert int(case_search.group('executed_test_cases')) == \
               expected['executed_test_cases']
        assert int(case_search.group('skipped_test_cases')) == \
               expected['skipped_test_cases']
        assert int(case_search.group('platform_count')) == \
               expected['platform_count']

        built_search = re.search(built_regex, err, re.MULTILINE)

        assert built_search
        assert int(built_search.group('executed_on_platform')) == \
               expected['executed_on_platform']
        assert int(built_search.group('only_built')) == \
               expected['only_built']

        assert str(sys_exit.value) == '0'

    @pytest.mark.parametrize(
        'test_path, test_platforms, expected',
        TESTDATA_2,
        ids=[
            'build_only tests/dummy/agnostic',
            'build_only tests/dummy/device',
        ],
    )
    @pytest.mark.parametrize(
        'flag',
        ['--build-only', '-b']
    )
    def test_build_only(self, capfd, out_path, test_path, test_platforms, expected, flag):
        args = ['-i', '--outdir', out_path, '-T', test_path, flag] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
            pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        built_regex = r'^INFO    - (?P<executed_on_platform>[0-9]+)' \
                      r' test configurations executed on platforms, (?P<only_built>[0-9]+)' \
                      r' test configurations were only built.$'

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        built_search = re.search(built_regex, err, re.MULTILINE)

        assert built_search
        assert int(built_search.group('executed_on_platform')) == \
               expected['executed_on_platform']
        assert int(built_search.group('only_built')) == \
               expected['only_built']

        assert str(sys_exit.value) == '0'

    @pytest.mark.parametrize(
        'test_path, test_platforms, expected',
        TESTDATA_3,
        ids=[
            'test_only'
        ],
    )
    def test_runtest_only(self, capfd, out_path, test_path, test_platforms, expected):

        args = ['--outdir', out_path,'-i', '-T', test_path, '--build-only'] + \
            [val for pair in zip(
                ['-p'] * len(test_platforms), test_platforms
            ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
            pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        capfd.readouterr()

        clear_log_in_test()

        args = ['--outdir', out_path,'-i', '-T', test_path, '--test-only'] + \
            [val for pair in zip(
                ['-p'] * len(test_platforms), test_platforms
            ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
            pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)


        select_regex = r'^INFO    - (?P<test_scenarios>[0-9]+) test scenarios' \
                       r' \((?P<test_instances>[0-9]+) test instances\) selected,' \
                       r' (?P<skipped_configurations>[0-9]+) configurations skipped' \
                       r' \((?P<skipped_by_static_filter>[0-9]+) by static filter,' \
                       r' (?P<skipped_at_runtime>[0-9]+) at runtime\)\.$'

        pass_regex = r'^INFO    - (?P<passed_configurations>[0-9]+) of' \
                     r' (?P<test_instances>[0-9]+) test configurations passed' \
                     r' \([0-9]+\.[0-9]+%\), (?P<failed_configurations>[0-9]+) failed,' \
                     r' (?P<errored_configurations>[0-9]+) errored,' \
                     r' (?P<skipped_configurations>[0-9]+) skipped with' \
                     r' [0-9]+ warnings in [0-9]+\.[0-9]+ seconds$'

        case_regex = r'^INFO    - In total (?P<executed_test_cases>[0-9]+)' \
                     r' test cases were executed, (?P<skipped_test_cases>[0-9]+) skipped' \
                     r' on (?P<platform_count>[0-9]+) out of total [0-9]+ platforms' \
                     r' \([0-9]+\.[0-9]+%\)$'
        built_regex = r'^INFO    - (?P<executed_on_platform>[0-9]+)' \
                      r' test configurations executed on platforms, (?P<only_built>[0-9]+)' \
                      r' test configurations were only built.$'

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        path = os.path.join(out_path, 'twister.log')
        with open(path, "r") as log_file:
            text_content = log_file.read()
        print(text_content)

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
        assert int(pass_search.group('test_instances')) == \
            expected['selected_test_instances']
        assert int(pass_search.group('failed_configurations')) == \
            expected['failed_configurations']
        assert int(pass_search.group('errored_configurations')) == \
            expected['errored_configurations']
        assert int(pass_search.group('skipped_configurations')) == \
            expected['skipped_configurations']

        case_search = re.search(case_regex, err, re.MULTILINE)

        assert case_search
        assert int(case_search.group('executed_test_cases')) == \
            expected['executed_test_cases']
        assert int(case_search.group('skipped_test_cases')) == \
            expected['skipped_test_cases']
        assert int(case_search.group('platform_count')) == \
            expected['platform_count']

        built_search = re.search(built_regex, err, re.MULTILINE)

        assert built_search
        assert int(built_search.group('executed_on_platform')) == \
               expected['executed_on_platform']
        assert int(built_search.group('only_built')) == \
               expected['only_built']

        assert str(sys_exit.value) == '0'

    @pytest.mark.parametrize(
        'test_path, test_platforms',
        TESTDATA_4,
        ids=[
            'ninja',
        ]
    )
    @pytest.mark.parametrize(
        'flag',
        ['--ninja', '-N']
    )
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

    @pytest.mark.parametrize(
        'test_path, test_platforms',
        TESTDATA_4,
        ids=[
            'dry_run',
        ],
    )
    @pytest.mark.parametrize(
        'flag',
        ['--dry-run', '-y']
    )
    def test_dry_run(self, capfd, out_path, test_path, test_platforms, flag):
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

    @pytest.mark.parametrize(
        'test_path, test_platforms',
        TESTDATA_4,
        ids=[
            'any_platform',
        ],
    )
    @pytest.mark.parametrize(
        'flag',
        ['-l', '--all']
    )
    def test_any_platform(self, capfd, out_path, test_path, test_platforms, flag):
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

    @pytest.mark.parametrize(
        'test_path, test_platforms, expected',
        TESTDATA_5,
        ids=[
            'cmake_only',
        ],
    )
    def test_cmake_only(self, capfd, out_path, test_path, test_platforms, expected):
        args = ['--outdir', out_path, '-T', test_path, '--cmake-only'] + \
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
                     r' (?P<test_instances>[0-9]+) test configurations passed'

        built_regex = r'^INFO    - (?P<executed_on_platform>[0-9]+)' \
                      r' test configurations executed on platforms, (?P<only_built>[0-9]+)' \
                      r' test configurations were only built.$'

        pass_search = re.search(pass_regex, err, re.MULTILINE)

        assert pass_search
        assert int(pass_search.group('passed_configurations')) == \
               expected['passed_configurations']
        assert int(pass_search.group('test_instances')) == \
               expected['selected_test_instances']

        built_search = re.search(built_regex, err, re.MULTILINE)

        assert built_search
        assert int(built_search.group('executed_on_platform')) == \
               expected['executed_on_platform']
        assert int(built_search.group('only_built')) == \
               expected['only_built']

        assert str(sys_exit.value) == '0'

    @pytest.mark.parametrize(
        'test_path, test_platforms, file_name',
        TESTDATA_6,
        ids=[
            'pre_script',
        ],
    )
    def test_pre_script(self, capfd, out_path, test_path, test_platforms, file_name):
        args = ['--outdir', out_path, '-T', test_path, '--pre-script', file_name] + \
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
        TESTDATA_7,
        ids=[
            'exclude_platform',
        ],
    )
    def test_exclude_platform(self, capfd, out_path, test_path, test_platforms, expected):
        args = ['--outdir', out_path, '-T', test_path, '--exclude-platform', "qemu_x86"] + \
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
                     r' (?P<test_instances>[0-9]+) test configurations passed'

        built_regex = r'^INFO    - (?P<executed_on_platform>[0-9]+)' \
                      r' test configurations executed on platforms, (?P<only_built>[0-9]+)' \
                      r' test configurations were only built.$'

        pass_search = re.search(pass_regex, err, re.MULTILINE)

        assert pass_search
        assert int(pass_search.group('passed_configurations')) == \
               expected['passed_configurations']
        assert int(pass_search.group('test_instances')) == \
               expected['selected_test_instances']

        built_search = re.search(built_regex, err, re.MULTILINE)

        assert built_search
        assert int(built_search.group('executed_on_platform')) == \
               expected['executed_on_platform']
        assert int(built_search.group('only_built')) == \
               expected['only_built']

        assert str(sys_exit.value) == '0'

    @pytest.mark.parametrize(
        'test_path, test_platforms',
        TESTDATA_4,
        ids=[
            'device_flash_timeout',
        ],
    )
    def test_device_flash_timeout(self, capfd, out_path, test_path, test_platforms):
        args = ['--outdir', out_path, '-T', test_path, '--device-flash-timeout', "240"] + \
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
        'test_path, test_platforms, iterations',
        TESTDATA_8,
        ids=[
            'retry 2',
            'retry 3'
        ],
    )
    def test_retry(self, capfd, out_path, test_path, test_platforms, iterations):
        args = ['--outdir', out_path, '-T', test_path, '--retry-failed', iterations, '--retry-interval', '1'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
            pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        pattern = re.compile(r'INFO\s+-\s+(\d+)\s+Iteration:[\s\S]*?ERROR\s+-\s+(\w+)')
        matches = pattern.findall(err)

        if matches:
            last_iteration = max(int(match[0]) for match in matches)
            last_match = next(match for match in matches if int(match[0]) == last_iteration)
            iteration_number, platform_name = int(last_match[0]), last_match[1]
            assert int(iteration_number) == int(iterations) + 1
            assert [platform_name] == test_platforms
        else:
            assert 'Pattern not found in the output'

        assert str(sys_exit.value) == '1'

    @pytest.mark.parametrize(
        'test_path, test_platforms, interval',
        TESTDATA_9,
        ids=[
            'retry interval 15',
            'retry interval 30'
        ],
    )
    def test_retry_interval(self, capfd, out_path, test_path, test_platforms, interval):
        args = ['--outdir', out_path, '-T', test_path, '--retry-failed', '1', '--retry-interval', interval] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        start_time = time.time()

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
            pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        end_time = time.time()
        elapsed_time = end_time - start_time
        print(f"Time elapsed: {elapsed_time:.2f} seconds")
        if elapsed_time < int(interval):
            assert 'interval was too short'

        assert str(sys_exit.value) == '1'

    @pytest.mark.parametrize(
        'test_path, test_platforms, timeout',
        TESTDATA_10,
        ids=[
            'timeout-multiplier 2 - 20s',
            'timeout-multiplier 0.5 - 5s'
        ],
    )
    def test_timeout_multiplier(self, capfd, out_path, test_path, test_platforms, timeout):
        args = ['--outdir', out_path, '-T', test_path, '--timeout-multiplier', timeout, '-v'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        tolerance = 1.0

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
            pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        elapsed_time = float(re.search(r'Timeout \(qemu (\d+\.\d+)s\)', err).group(1))

        assert abs(
            elapsed_time - float(timeout) * 10) <= tolerance, f"Time is different from expected"

        assert str(sys_exit.value) == '1'

    @pytest.mark.parametrize(
        'test_path, test_platforms, quarantine_directory',
        TESTDATA_11,
        ids=[
            'quarantine',
        ],
    )
    def test_quarantine_list(self, capfd, out_path, test_path, test_platforms, quarantine_directory):
        args = ['--outdir', out_path, '-T', test_path, '--quarantine-list', quarantine_directory, '-vv'] + \
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
                               'frdm_k64f', err)
        frdm_match2 = re.search(
            'agnostic/group1/subgroup2/dummy.agnostic.group1.subgroup2 SKIPPED: Quarantine: test '
            'frdm_k64f',
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

    @pytest.mark.parametrize(
        'test_path, test_platforms, tags, expected',
        TESTDATA_12,
        ids=[
            'tags device',
            'tags subgruped',
            'tag agnostic and device'
        ],
    )
    def test_tag(self, capfd, out_path, test_path, test_platforms, tags, expected):
        args = ['--outdir', out_path, '-T', test_path, '-vv'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair] + \
               [val for pairs in zip(
                   ['-t'] * len(tags), tags
               ) for val in pairs]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
            pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        for line in expected:
            print(line)
            assert re.search(line, err)

        assert str(sys_exit.value) == '0'

    @pytest.mark.parametrize(
        'test_path, test_platforms, expected',
        TESTDATA_13,
        ids=[
            'only_failed'
        ],
    )

    def test_only_failed(self, capfd, out_path, test_path, test_platforms, expected):
        args = ['--outdir', out_path,'-i', '-T', test_path, '-v'] + \
            [val for pair in zip(
                ['-p'] * len(test_platforms), test_platforms
            ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
            pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        capfd.readouterr()

        clear_log_in_test()

        args = ['--outdir', out_path,'-i', '-T', test_path, '--only-failed'] + \
            [val for pair in zip(
                ['-p'] * len(test_platforms), test_platforms
            ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
            pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)


        pass_regex = r'^INFO    - (?P<passed_configurations>[0-9]+) of' \
                     r' (?P<test_instances>[0-9]+) test configurations passed' \
                     r' \([0-9]+\.[0-9]+%\), (?P<failed_configurations>[0-9]+) failed,' \
                     r' (?P<errored_configurations>[0-9]+) errored,' \
                     r' (?P<skipped_configurations>[0-9]+) skipped with' \
                     r' [0-9]+ warnings in [0-9]+\.[0-9]+ seconds$'

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)


        assert re.search(
            r'one_fail_one_pass.agnostic.group1.subgroup2 on qemu_x86 failed \(Failed\)', err)

        pass_search = re.search(pass_regex, err, re.MULTILINE)

        assert pass_search
        assert int(pass_search.group('passed_configurations')) == \
                expected['passed_configurations']
        assert int(pass_search.group('test_instances')) == \
                expected['selected_test_instances']
        assert int(pass_search.group('failed_configurations')) == \
                expected['failed_configurations']
        assert int(pass_search.group('errored_configurations')) == \
                expected['errored_configurations']
        assert int(pass_search.group('skipped_configurations')) == \
                expected['skipped_configurations']

        assert str(sys_exit.value) == '1'

    @pytest.mark.parametrize(
        'test_path, test_platforms, iterations',
        TESTDATA_14,
        ids=[
            'retry 2',
            'retry 3'
        ],
    )
    def test_retry_build_errors(self, capfd, out_path, test_path, test_platforms, iterations):
        args = ['--outdir', out_path, '-T', test_path, '--retry-build-errors', '--retry-failed', iterations,
                '--retry-interval', '10'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
            pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        pattern = re.compile(r'INFO\s+-\s+(\d+)\s+Iteration:[\s\S]*?ERROR\s+-\s+(\w+)')
        matches = pattern.findall(err)

        if matches:
            last_iteration = max(int(match[0]) for match in matches)
            last_match = next(match for match in matches if int(match[0]) == last_iteration)
            iteration_number, platform_name = int(last_match[0]), last_match[1]
            assert int(iteration_number) == int(iterations) + 1
            assert [platform_name] == test_platforms
        else:
            assert 'Pattern not found in the output'

        assert str(sys_exit.value) == '1'
