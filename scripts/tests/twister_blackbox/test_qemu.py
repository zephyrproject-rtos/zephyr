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


from conftest import TEST_DATA, ZEPHYR_BASE, testsuite_filename_mock
from twisterlib.testplan import TestPlan


@mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', testsuite_filename_mock)
class TestQEMU:
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


    @classmethod
    def setup_class(cls):
        apath = os.path.join(ZEPHYR_BASE, 'scripts', 'twister')
        cls.loader = importlib.machinery.SourceFileLoader('__main__', apath)
        cls.spec = importlib.util.spec_from_loader(cls.loader.name, cls.loader)
        cls.twister_module = importlib.util.module_from_spec(cls.spec)


    @classmethod
    def teardown_class(cls):
        pass


    @pytest.mark.usefixtures("clear_log")
    @pytest.mark.parametrize(
        'test_path, test_platforms, expected',
        TESTDATA_1,
        ids=[
            'tests/dummy/agnostic',
            'tests/dummy/device',
        ]
    )
    def test_emulation_only(self, capfd, test_path, test_platforms, expected):
        args = ['-i', '-T', test_path, '--emulation-only'] + \
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
