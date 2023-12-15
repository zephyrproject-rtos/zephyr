#!/usr/bin/env python3
# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Blackbox tests for twister's command line functions
"""

import importlib
import re

import mock
import os
import shutil
import pytest
import sys
from lxml import etree
import json

from conftest import TEST_DATA, ZEPHYR_BASE, testsuite_filename_mock
from twisterlib.testplan import TestPlan


@mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', testsuite_filename_mock)
class TestReport:
    TESTDATA_1 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86', 'qemu_x86_64'],
            [
                'qemu_x86_64.xml', 'qemu_x86.xml',
                'testplan.json', 'twister.json',
                'twister.log', 'twister_report.xml',
                'twister_suite_report.xml', 'twister.xml'
            ]
        ),
    ]
    TESTDATA_2 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86', 'qemu_x86_64'],
            [
                'qemu_x86_64_TEST.xml', 'qemu_x86_TEST.xml',
                'twister_TEST.json', 'twister_TEST_report.xml',
                'twister_TEST_suite_report.xml', 'twister_TEST.xml'
            ]
        ),
    ]
    TESTDATA_3 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86', 'qemu_x86_64'],
            ['--report-name', 'abcd'],
            [
                'abcd.json', 'abcd_report.xml',
                'abcd_suite_report.xml', 'abcd.xml'
            ]
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86', 'qemu_x86_64'],
            ['--report-name', '1234', '--platform-reports'],
            [
                'qemu_x86_64.xml', 'qemu_x86.xml',
                '1234.json', '1234_report.xml',
                '1234_suite_report.xml', '1234.xml'
            ]
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86', 'qemu_x86_64'],
            ['--report-name', 'Final', '--platform-reports', '--report-suffix=Test'],
            [
                'qemu_x86_64_Test.xml', 'qemu_x86_Test.xml',
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
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86'],
            [
                'coverage.log', 'coverage.json',
                'coverage'
            ],
        ),
    ]
    TESTDATA_8 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86'],
            [
                'GCOV_COVERAGE_DUMP_START', 'GCOV_COVERAGE_DUMP_END'
            ],
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
            platform_path = os.path.join(out_path, f_platform)
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
        'test_path, test_platforms, file_name',
        TESTDATA_7,
        ids=[
            'coverage',
        ]
    )
    def test_coverage(self, capfd, test_path, test_platforms, out_path, file_name):
        args = ['-i','--outdir', out_path, '-T', test_path, '--coverage', '--coverage-tool', 'gcovr'] + \
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
        'test_path, test_platforms, expected',
        TESTDATA_8,
        ids=[
            'enable_coverage',
        ]
    )
    def test_enable_coverage(self, capfd, test_path, test_platforms, out_path, expected):
        args = ['-i','--outdir', out_path, '-T', test_path, '--enable-coverage', '-vv'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        for line in expected:
            match = re.search(line, err)
            assert match, f'line not found: {line}'

        assert str(sys_exit.value) == '0'
