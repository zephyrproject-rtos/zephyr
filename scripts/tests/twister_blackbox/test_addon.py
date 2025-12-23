#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Blackbox tests for twister's command line functions concerning addons to normal functions
"""

import importlib
from unittest import mock
import os
import pkg_resources
import pytest
import re
import shutil
import subprocess
import sys

# pylint: disable=no-name-in-module
from conftest import ZEPHYR_BASE, TEST_DATA, sample_filename_mock, suite_filename_mock
from twisterlib.testplan import TestPlan


class TestAddon:
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
        'ubsan_flags, expected_exit_value',
        [
            # No sanitiser, no problem
            ([], '0'),
            # Sanitiser catches a mistake, error is raised
            (['--enable-ubsan'], '1')
        ],
        ids=['no sanitiser', 'ubsan']
    )
    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', suite_filename_mock)
    def test_enable_ubsan(self, out_path, ubsan_flags, expected_exit_value):
        test_platforms = ['native_sim']
        test_path = os.path.join(TEST_DATA, 'tests', 'san', 'ubsan')
        args = ['-i', '--outdir', out_path, '-T', test_path] + \
               ubsan_flags + \
               [] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == expected_exit_value

    @pytest.mark.parametrize(
        'lsan_flags, expected_exit_value',
        [
            # No sanitiser, no problem
            ([], '0'),
            # Sanitiser catches a mistake, error is raised
            (['--enable-asan', '--enable-lsan'], '1')
        ],
        ids=['no sanitiser', 'lsan']
    )
    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', suite_filename_mock)
    def test_enable_lsan(self, out_path, lsan_flags, expected_exit_value):
        test_platforms = ['native_sim']
        test_path = os.path.join(TEST_DATA, 'tests', 'san', 'lsan')
        args = ['-i', '--outdir', out_path, '-T', test_path] + \
               lsan_flags + \
               [] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == expected_exit_value

    @pytest.mark.parametrize(
        'asan_flags, expected_exit_value, expect_asan',
        [
            # No sanitiser, no problem
            # Note that on some runs it may fail,
            # as the process is killed instead of ending normally.
            # This is not 100% repeatable, so this test is removed for now.
            # ([], '0', False),
            # Sanitiser catches a mistake, error is raised
            (['--enable-asan'], '1', True)
        ],
        ids=[
            #'no sanitiser',
            'asan'
        ]
    )
    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', suite_filename_mock)
    def test_enable_asan(self, capfd, out_path, asan_flags, expected_exit_value, expect_asan):
        test_platforms = ['native_sim']
        test_path = os.path.join(TEST_DATA, 'tests', 'san', 'asan')
        args = ['-i', '-W', '--outdir', out_path, '-T', test_path] + \
               asan_flags + \
               [] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == expected_exit_value

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        asan_template = r'^==\d+==ERROR:\s+AddressSanitizer:'
        assert expect_asan == bool(re.search(asan_template, err, re.MULTILINE))

    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', suite_filename_mock)
    def test_extra_test_args(self, capfd, out_path):
        test_platforms = ['native_sim']
        test_path = os.path.join(TEST_DATA, 'tests', 'params', 'dummy')
        args = ['-i', '--outdir', out_path, '-T', test_path] + \
               [] + \
               ['-vvv'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair] + \
               ['--', '-list']

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        # Use of -list makes tests not run.
        # Thus, the tests 'failed'.
        assert str(sys_exit.value) == '1'

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        expected_test_names = [
            'param_tests::test_assert1',
            'param_tests::test_assert2',
            'param_tests::test_assert3',
        ]
        assert all([testname in err for testname in expected_test_names])

    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', suite_filename_mock)
    def test_extra_args(self, caplog, out_path):
        test_platforms = ['qemu_x86', 'intel_adl_crb']
        path = os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic', 'group2')
        args = ['--outdir', out_path, '-T', path] + \
               ['--extra-args', 'USE_CCACHE=0', '--extra-args', 'DUMMY=1'] + \
               [] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'

        with open(os.path.join(out_path, 'twister.log')) as f:
            twister_log = f.read()

        pattern_cache = r'Calling cmake: [^\n]+ -DUSE_CCACHE=0 [^\n]+\n'
        pattern_dummy = r'Calling cmake: [^\n]+ -DDUMMY=1 [^\n]+\n'

        assert ' -DUSE_CCACHE=0 ' in twister_log
        res = re.search(pattern_cache, twister_log)
        assert res

        assert ' -DDUMMY=1 ' in twister_log
        res = re.search(pattern_dummy, twister_log)
        assert res

    # This test is not side-effect free.
    # It installs and uninstalls pytest-twister-harness using pip
    # It uses pip to check whether that plugin is previously installed
    # and reinstalls it if detected at the start of its run.
    # However, it does NOT restore the original plugin, ONLY reinstalls it.
    @pytest.mark.parametrize(
        'allow_flags, do_install, expected_exit_value, expected_logs',
        [
            ([], True, '1', ['By default Twister should work without pytest-twister-harness'
                             ' plugin being installed, so please, uninstall it by'
                             ' `pip uninstall pytest-twister-harness` and'
                             ' `git clean -dxf scripts/pylib/pytest-twister-harness`.']),
            (['--allow-installed-plugin'], True, '0', ['You work with installed version'
                                                       ' of pytest-twister-harness plugin.']),
            ([], False, '0', []),
            (['--allow-installed-plugin'], False, '0', []),
        ],
        ids=['installed, but not allowed', 'installed, allowed',
             'not installed, not allowed', 'not installed, but allowed']
    )
    @mock.patch.object(TestPlan, 'SAMPLE_FILENAME', sample_filename_mock)
    def test_allow_installed_plugin(self, caplog, out_path, allow_flags, do_install,
                                    expected_exit_value, expected_logs):
        environment_twister_module = importlib.import_module('twisterlib.environment')
        harness_twister_module = importlib.import_module('twisterlib.harness')
        runner_twister_module = importlib.import_module('twisterlib.runner')

        pth_path = os.path.join(ZEPHYR_BASE, 'scripts', 'pylib', 'pytest-twister-harness')
        check_installed_command = [sys.executable, '-m', 'pip', 'list']
        install_command = [sys.executable, '-m', 'pip', 'install', '--no-input', pth_path]
        uninstall_command = [sys.executable, '-m', 'pip', 'uninstall', '--yes',
                             'pytest-twister-harness']

        def big_uninstall():
            pth_path = os.path.join(ZEPHYR_BASE, 'scripts', 'pylib', 'pytest-twister-harness')

            subprocess.run(uninstall_command, check=True,)

            # For our registration to work, we have to delete the installation cache
            additional_cache_paths = [
                # Plugin cache
                os.path.join(pth_path, 'src', 'pytest_twister_harness.egg-info'),
                # Additional caches
                os.path.join(pth_path, 'src', 'pytest_twister_harness', '__pycache__'),
                os.path.join(pth_path, 'src', 'pytest_twister_harness', 'device', '__pycache__'),
                os.path.join(pth_path, 'src', 'pytest_twister_harness', 'helpers', '__pycache__'),
                os.path.join(pth_path, 'src', 'pytest_twister_harness', 'build'),
            ]

            for additional_cache_path in additional_cache_paths:
                if os.path.exists(additional_cache_path):
                    if os.path.isfile(additional_cache_path):
                        os.unlink(additional_cache_path)
                    else:
                        shutil.rmtree(additional_cache_path)

        # To refresh the PYTEST_PLUGIN_INSTALLED global variable
        def refresh_plugin_installed_variable():
            pkg_resources._initialize_master_working_set()
            importlib.reload(environment_twister_module)
            importlib.reload(harness_twister_module)
            importlib.reload(runner_twister_module)

        check_installed_result = subprocess.run(check_installed_command, check=True,
                                                capture_output=True, text=True)
        previously_installed = 'pytest-twister-harness' in check_installed_result.stdout

        # To ensure consistent test start
        big_uninstall()

        if do_install:
            subprocess.run(install_command, check=True)

        # Refresh before the test, no matter the testcase
        refresh_plugin_installed_variable()

        test_platforms = ['native_sim']
        test_path = os.path.join(TEST_DATA, 'samples', 'pytest', 'shell')
        args = ['-i', '--outdir', out_path, '-T', test_path] + \
               allow_flags + \
               [] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        # To ensure consistent test exit, prevent dehermetisation
        if do_install:
            big_uninstall()

        # To restore previously-installed plugin as well as we can
        if previously_installed:
            subprocess.run(install_command, check=True)

        if previously_installed or do_install:
            refresh_plugin_installed_variable()

        assert str(sys_exit.value) == expected_exit_value

        assert all([log in caplog.text for log in expected_logs])

    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', suite_filename_mock)
    def test_pytest_args(self, out_path):
        test_platforms = ['native_sim']
        test_path = os.path.join(TEST_DATA, 'tests', 'pytest')
        args = ['-i', '--outdir', out_path, '-T', test_path] + \
               ['--pytest-args=--custom-pytest-arg', '--pytest-args=foo',
                '--pytest-args=--cmdopt', '--pytest-args=.'] + \
               [] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        # YAML was modified so that the test will fail without command line override.
        assert str(sys_exit.value) == '0'

    @pytest.mark.parametrize(
        'valgrind_flags, expected_exit_value',
        [
            # No sanitiser, leak is ignored
            ([], '0'),
            # Sanitiser catches a mistake, error is raised
            (['--enable-valgrind'], '1')
        ],
        ids=['no valgrind', 'valgrind']
    )
    @mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', suite_filename_mock)
    def test_enable_valgrind(self, capfd, out_path, valgrind_flags, expected_exit_value):
        test_platforms = ['native_sim']
        test_path = os.path.join(TEST_DATA, 'tests', 'san', 'val')
        args = ['-i', '--outdir', out_path, '-T', test_path] + \
               valgrind_flags + \
               [] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == expected_exit_value
