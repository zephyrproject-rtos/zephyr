#!/usr/bin/env python3
# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Blackbox tests for twister's command line functions
"""

import importlib
import json
import mock
import os
import pytest
import shutil
import sys
import re

from lxml import etree

# pylint: disable=no-name-in-module
from conftest import TEST_DATA, ZEPHYR_BASE, testsuite_filename_mock, clear_log_in_test
from twisterlib.testplan import TestPlan


@mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', testsuite_filename_mock)
class TestReport:
    TESTDATA_1 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86', 'mps2/an385'],
            [
                'qemu_x86.xml', 'mps2_an385.xml',
                'testplan.json', 'twister.json',
                'twister.log', 'twister_report.xml',
                'twister_suite_report.xml', 'twister.xml'
            ]
        ),
    ]
    TESTDATA_2 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86', 'mps2/an385'],
            [
                'mps2_an385_TEST.xml', 'qemu_x86_TEST.xml',
                'twister_TEST.json', 'twister_TEST_report.xml',
                'twister_TEST_suite_report.xml', 'twister_TEST.xml'
            ]
        ),
    ]
    TESTDATA_3 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86', 'mps2/an385'],
            ['--report-name', 'abcd'],
            [
                'abcd.json', 'abcd_report.xml',
                'abcd_suite_report.xml', 'abcd.xml'
            ]
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86', 'mps2/an385'],
            ['--report-name', '1234', '--platform-reports'],
            [
                'mps2_an385.xml', 'qemu_x86.xml',
                '1234.json', '1234_report.xml',
                '1234_suite_report.xml', '1234.xml'
            ]
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86', 'mps2/an385'],
            ['--report-name', 'Final', '--platform-reports', '--report-suffix=Test'],
            [
                'mps2_an385_Test.xml', 'qemu_x86_Test.xml',
                'Final_Test.json', 'Final_Test_report.xml',
                'Final_Test_suite_report.xml', 'Final_Test.xml'
            ]
        ),
    ]
    TESTDATA_4 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86'],
            [
                'twister.json', 'twister_report.xml',
                'twister_suite_report.xml', 'twister.xml'
            ],
            "TEST_DIR"
        ),
    ]
    TESTDATA_5 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86'],
            [
                'testplan.json', 'twister.log',
                'twister.json', 'twister_report.xml',
                'twister_suite_report.xml', 'twister.xml'
            ],
            "OUT_DIR"
        ),
    ]
    TESTDATA_6 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86'],
            "TEST_LOG_FILE.log"
        ),
    ]
    TESTDATA_7 = [
        (
            os.path.join(TEST_DATA, 'tests', 'one_fail_two_error_one_pass'),
            ['qemu_x86'],
            [r'one_fail_two_error_one_pass.agnostic.group1.subgroup2 on qemu_x86 FAILED \(.*\)',
            r'one_fail_two_error_one_pass.agnostic.group1.subgroup3 on qemu_x86 ERROR \(Build failure\)',
            r'one_fail_two_error_one_pass.agnostic.group1.subgroup4 on qemu_x86 ERROR \(Build failure\)'],
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
        'test_path, test_platforms, file_name',
        TESTDATA_1,
        ids=[
            'platform_reports'
        ]
    )
    def test_platform_reports(self, capfd, out_path, test_path, test_platforms, file_name):
        args = ['-i', '--outdir', out_path, '-T', test_path, '--platform-reports'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        for f_name in file_name:
            path = os.path.join(out_path, f_name)
            assert os.path.exists(path), 'file not found'

            if path.endswith(".json"):
                with open(path, "r") as json_file:
                    data = json.load(json_file)
                    assert data, f"JSON file '{path}' is empty"

            elif path.endswith(".xml"):
                tree = etree.parse(path)
                xml_text = etree.tostring(tree, encoding="utf-8").decode("utf-8")
                assert xml_text.strip(), f"XML file '{path}' is empty"

            elif path.endswith(".log"):
                with open(path, "r") as log_file:
                    text_content = log_file.read()
                    assert text_content.strip(), f"LOG file '{path}' is empty"

            else:
                pytest.fail(f"Unsupported file type: '{path}'")

        for f_platform in test_platforms:
            platform_path = os.path.join(out_path, f_platform.replace("/", "_"))
            assert os.path.exists(platform_path), f'file not found {f_platform}'

        assert str(sys_exit.value) == '0'

    @pytest.mark.parametrize(
        'test_path, test_platforms, file_name',
        TESTDATA_2,
        ids=[
            'report_suffix',
        ]
    )
    def test_report_suffix(self, capfd, out_path, test_path, test_platforms, file_name):
        args = ['-i', '--outdir', out_path, '-T', test_path, '--platform-reports', '--report-suffix=TEST'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        for f_name in file_name:
            path = os.path.join(out_path, f_name)
            assert os.path.exists(path), f'file not found {f_name}'

        assert str(sys_exit.value) == '0'

    @pytest.mark.parametrize(
        'test_path, test_platforms, report_arg, file_name',
        TESTDATA_3,
        ids=[
            'only_report_name',
            'report_name + platform_reports',
            'report-name + platform-reports + report-suffix'
        ]
    )
    def test_report_name(self, capfd, out_path, test_path, test_platforms, report_arg, file_name):
        args = ['-i', '--outdir', out_path, '-T', test_path] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair] + \
               [val for pair in zip(
                   report_arg
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        for f_name in file_name:
            path = os.path.join(out_path, f_name)
            assert os.path.exists(path), f'file not found {f_name}'

        assert str(sys_exit.value) == '0'

    @pytest.mark.parametrize(
        'test_path, test_platforms, file_name, dir_name',
        TESTDATA_4,
        ids=[
            'report_dir',
        ]
    )
    def test_report_dir(self, capfd, out_path, test_path, test_platforms, file_name, dir_name):
        args = ['-i', '--outdir', out_path, '-T', test_path, "--report-dir", dir_name] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        twister_path = os.path.join(ZEPHYR_BASE, dir_name)
        if os.path.exists(twister_path):
            shutil.rmtree(twister_path)

        try:
            with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                    pytest.raises(SystemExit) as sys_exit:
                self.loader.exec_module(self.twister_module)

            out, err = capfd.readouterr()
            sys.stdout.write(out)
            sys.stderr.write(err)

            for f_name in file_name:
                path = os.path.join(twister_path, f_name)
                assert os.path.exists(path), f'file not found {f_name}'

            assert str(sys_exit.value) == '0'
        finally:
            twister_path = os.path.join(ZEPHYR_BASE, dir_name)
            if os.path.exists(twister_path):
                shutil.rmtree(twister_path)

    @pytest.mark.noclearout
    @pytest.mark.parametrize(
        'test_path, test_platforms, file_name, dir_name',
        TESTDATA_5,
        ids=[
            'outdir',
        ]
    )
    def test_outdir(self, capfd, test_path, test_platforms, file_name, dir_name):
        args = ['-i', '-T', test_path, "--outdir", dir_name] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        twister_path = os.path.join(ZEPHYR_BASE, dir_name)
        if os.path.exists(twister_path):
            shutil.rmtree(twister_path)

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        try:
            for f_name in file_name:
                path = os.path.join(twister_path, f_name)
                assert os.path.exists(path), 'file not found {f_name}'

            for f_platform in test_platforms:
                platform_path = os.path.join(twister_path, f_platform)
                assert os.path.exists(platform_path), f'file not found {f_platform}'

            assert str(sys_exit.value) == '0'
        finally:
            twister_path = os.path.join(ZEPHYR_BASE, dir_name)
            if os.path.exists(twister_path):
                shutil.rmtree(twister_path)

    @pytest.mark.parametrize(
        'test_path, test_platforms, file_name',
        TESTDATA_6,
        ids=[
            'log_file',
        ]
    )
    def test_log_file(self, capfd, test_path, test_platforms, out_path, file_name):
        args = ['-i','--outdir', out_path, '-T', test_path, "--log-file", file_name] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        file_path = os.path.join(ZEPHYR_BASE, file_name)
        if os.path.exists(file_path):
            os.remove(file_path)

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        assert os.path.exists(file_path), 'file not found {f_name}'

        assert str(sys_exit.value) == '0'

    @pytest.mark.parametrize(
        'test_path, flags, expected_testcase_counts',
        [
            (
                os.path.join(TEST_DATA, 'tests', 'dummy'),
                ['--detailed-skipped-report'],
                {'qemu_x86': 5, 'frdm_k64f': 1}
            ),
            (
                os.path.join(TEST_DATA, 'tests', 'dummy'),
                ['--detailed-skipped-report', '--report-filtered'],
                {'qemu_x86': 6, 'frdm_k64f': 6}
            ),
        ],
        ids=['dummy tests', 'dummy tests with filtered']
    )
    def test_detailed_skipped_report(self, out_path, test_path, flags, expected_testcase_counts):
        test_platforms = ['qemu_x86', 'frdm_k64f']
        args = ['-i', '--outdir', out_path, '-T', test_path] + \
               flags + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'

        testsuite_counter = 0
        xml_data = etree.parse(os.path.join(out_path, 'twister_report.xml')).getroot()
        for ts in xml_data.iter('testsuite'):
            testsuite_counter += 1
            # Without the tested flag, filtered testcases would be missing from the report
            testcase_count = len(list(ts.iter('testcase')))
            expected_tc_count = expected_testcase_counts[ts.get('name')]
            assert testcase_count == expected_tc_count, \
                   f'Not all expected testcases appear in the report.' \
                   f' (In {ts.get("name")}, expected {expected_tc_count}, got {testcase_count}.)'

        assert testsuite_counter == len(test_platforms), \
               'Some platforms are missing from the XML report.'

    @pytest.mark.parametrize(
        'test_path, report_filtered, expected_filtered_count',
        [
            (os.path.join(TEST_DATA, 'tests', 'dummy'), False, 0),
            (os.path.join(TEST_DATA, 'tests', 'dummy'), True, 4),
        ],
        ids=['no filtered', 'with filtered']
    )
    def test_report_filtered(self, out_path, test_path, report_filtered, expected_filtered_count):
        test_platforms = ['qemu_x86', 'frdm_k64f']
        args = ['-i', '--outdir', out_path, '-T', test_path] + \
               (['--report-filtered'] if report_filtered else []) + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'

        with open(os.path.join(out_path, 'twister.json')) as f:
            j = json.load(f)

        testsuites = j.get('testsuites')
        assert testsuites, 'No testsuites found.'
        statuses = [testsuite.get('status') for testsuite in testsuites]
        filtered_status_count = statuses.count("filtered")
        assert filtered_status_count == expected_filtered_count, \
            f'Expected {expected_filtered_count} filtered statuses, got {filtered_status_count}.'

    def test_enable_size_report(self, out_path):
        test_platforms = ['qemu_x86', 'frdm_k64f']
        path = os.path.join(TEST_DATA, 'tests', 'dummy', 'device', 'group')
        args = ['-i', '--outdir', out_path, '-T', path] + \
               ['--enable-size-report'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'

        with open(os.path.join(out_path, 'twister.json')) as f:
            j = json.load(f)

        expected_rel_path = os.path.relpath(os.path.join(path, 'dummy.device.group'), ZEPHYR_BASE)

        # twister.json will contain [used/available]_[ram/rom] keys if the flag works
        # except for those keys that would have values of 0.
        # In this testcase, availables are equal to 0, so they are missing.
        assert all(
            [
                'used_ram' in ts for ts in j['testsuites'] \
                if ts['name'] == expected_rel_path and not 'reason' in ts
            ]
        )
        assert all(
            [
                'used_rom' in ts for ts in j['testsuites'] \
                if ts['name'] == expected_rel_path and not 'reason' in ts
            ]
        )

    @pytest.mark.parametrize(
        'test_path, test_platforms, expected_content',
        TESTDATA_7,
        ids=[
            'Report summary test'
        ]
    )

    def test_report_summary(self, out_path, capfd, test_path, test_platforms, expected_content):
        args = ['-i', '--outdir', out_path, '-T', test_path] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '1'

        capfd.readouterr()

        clear_log_in_test()

        args += ['--report-summary']

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        for line in expected_content:
            result = re.search(line, err)
            assert result, f'missing information in log: {line}'

        capfd.readouterr()

        clear_log_in_test()

        args = ['-i', '--outdir', out_path, '-T', test_path] + \
               ['--report-summary', '2'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        lines=0
        for line in expected_content:
            result = re.search(line, err)
            if result: lines += 1
        assert lines == 2, f'too many or too few lines'
