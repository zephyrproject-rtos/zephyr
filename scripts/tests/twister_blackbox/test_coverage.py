#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Blackbox tests for twister's command line functions
"""
import importlib
import re
import mock
import os
import pytest
import sys
import json

from conftest import TEST_DATA, ZEPHYR_BASE, testsuite_filename_mock
from twisterlib.testplan import TestPlan


@mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', testsuite_filename_mock)
class TestCoverage:
    TESTDATA_1 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86'],
            [
                'coverage.log', 'coverage.json',
                'coverage'
            ],
        ),
    ]
    TESTDATA_2 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
            ['qemu_x86'],
            [
                'GCOV_COVERAGE_DUMP_START', 'GCOV_COVERAGE_DUMP_END'
            ],
        ),
    ]
    TESTDATA_3 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic', 'group2'),
            ['qemu_x86'],
            [
                'coverage.log', 'coverage.json',
                'coverage'
            ],
            r'{"files": \[], "gcovr/format_version": ".*"}'
        ),
    ]
    TESTDATA_4 = [
        (
            'gcovr',
            [
                'coverage.log', 'coverage.json',
                'coverage', os.path.join('coverage','coverage.xml')
            ],
            'xml'
        ),
        (
            'gcovr',
            [
                'coverage.log', 'coverage.json',
                'coverage', os.path.join('coverage','coverage.sonarqube.xml')
            ],
            'sonarqube'
        ),
        (
            'gcovr',
            [
                'coverage.log', 'coverage.json',
                'coverage', os.path.join('coverage','coverage.txt')
            ],
            'txt'
        ),
        (
            'gcovr',
            [
                'coverage.log', 'coverage.json',
                'coverage', os.path.join('coverage','coverage.csv')
            ],
            'csv'
        ),
        (
            'gcovr',
            [
                'coverage.log', 'coverage.json',
                'coverage', os.path.join('coverage','coverage.coveralls.json')
            ],
            'coveralls'
        ),
        (
            'gcovr',
            [
                'coverage.log', 'coverage.json',
                'coverage', os.path.join('coverage','index.html')
            ],
            'html'
        ),
        (
            'lcov',
            [
                'coverage.log', 'coverage.info',
                'ztest.info', 'coverage',
                os.path.join('coverage','index.html')
            ],
            'html'
        ),
        (
            'lcov',
            [
                'coverage.log', 'coverage.info',
                'ztest.info'
            ],
            'lcov'
        ),
    ]
    TESTDATA_5 = [
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic', 'group2'),
            ['qemu_x86'],
            'gcovr',
            'Running gcovr -r'
        ),
        (
            os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic', 'group2'),
            ['qemu_x86'],
            'lcov',
            'Running lcov --gcov-tool'
        )
    ]

    @classmethod
    def setup_class(cls):
        apath = os.path.join(ZEPHYR_BASE, 'scripts', 'twister')
        cls.loader = importlib.machinery.SourceFileLoader('__main__', apath)
        cls.spec = importlib.util.spec_from_loader(cls.loader.name, cls.loader)
        cls.twister_module = importlib.util.module_from_spec(cls.spec)

    @pytest.mark.parametrize(
        'test_path, test_platforms, file_name',
        TESTDATA_1,
        ids=[
            'coverage',
        ]
    )
    def test_coverage(self, capfd, test_path, test_platforms, out_path, file_name):
        args = ['-i','--outdir', out_path, '-T', test_path] + \
            ['--coverage', '--coverage-tool', 'gcovr'] + \
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

        for f_name in file_name:
            path = os.path.join(out_path, f_name)
            assert os.path.exists(path), f'file not found {f_name}'

    @pytest.mark.parametrize(
        'test_path, test_platforms, expected',
        TESTDATA_2,
        ids=[
            'enable_coverage',
        ]
    )
    def test_enable_coverage(self, capfd, test_path, test_platforms, out_path, expected):
        args = ['-i','--outdir', out_path, '-T', test_path] + \
        ['--enable-coverage', '-vv'] + \
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

        for line in expected:
            match = re.search(line, err)
            assert match, f'line not found: {line}'

    @pytest.mark.parametrize(
        'test_path, test_platforms, file_name, expected_content',
        TESTDATA_3,
        ids=[
            'coverage_basedir',
        ]
    )
    def test_coverage_basedir(self, capfd, test_path, test_platforms, out_path, file_name, expected_content):
        base_dir = os.path.join(TEST_DATA, "test_dir")
        if os.path.exists(base_dir):
            os.rmdir(base_dir)
        os.mkdir(base_dir)
        args = ['--outdir', out_path,'-i', '-T', test_path] + \
        ['--coverage', '--coverage-tool', 'gcovr',  '-v', '--coverage-basedir', base_dir] + \
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

        for f_name in file_name:
            path = os.path.join(out_path, f_name)
            assert os.path.exists(path), f'file not found {f_name}'
            if f_name == 'coverage.json':
                with open(path, "r") as json_file:
                    json_content = json.load(json_file)
                    pattern = re.compile(expected_content)
                    assert pattern.match(json.dumps(json_content))
        if os.path.exists(base_dir):
            os.rmdir(base_dir)

    @pytest.mark.parametrize(
        'cov_tool, file_name, cov_format',
        TESTDATA_4,
        ids=[
            'coverage_format gcovr xml',
            'coverage_format gcovr sonarqube',
            'coverage_format gcovr txt',
            'coverage_format gcovr csv',
            'coverage_format gcovr coveralls',
            'coverage_format gcovr html',
            'coverage_format lcov html',
            'coverage_format lcov lcov',
        ]
    )
    def test_coverage_format(self, capfd, out_path, cov_tool, file_name, cov_format):
        test_path = os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic', 'group2')
        test_platforms = ['qemu_x86']
        args = ['--outdir', out_path,'-i', '-T', test_path] + \
        ['--coverage', '--coverage-tool', cov_tool, '--coverage-formats', cov_format, '-v'] + \
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

        for f_name in file_name:
            path = os.path.join(out_path, f_name)
            assert os.path.exists(path), f'file not found {f_name}, probably format {cov_format} not work properly'



    @pytest.mark.parametrize(
        'test_path, test_platforms, cov_tool, expected_content',
        TESTDATA_5,
        ids=[
            'coverage_tool gcovr',
            'coverage_tool lcov'
        ]
    )
    def test_coverage_tool(self, capfd, caplog, test_path, test_platforms, out_path, cov_tool, expected_content):
        args = ['--outdir', out_path,'-i', '-T', test_path] + \
            ['--coverage', '--coverage-tool', cov_tool,  '-v'] + \
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

        assert re.search(expected_content, caplog.text), f'{cov_tool} line not found'
