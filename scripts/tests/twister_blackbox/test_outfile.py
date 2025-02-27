#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Blackbox tests for twister's command line functions changing the output files.
"""

import importlib
import re
import mock
import os
import shutil
import pytest
import sys
import tarfile

# pylint: disable=no-name-in-module
from conftest import ZEPHYR_BASE, TEST_DATA, sample_filename_mock, testsuite_filename_mock
from twisterlib.testplan import TestPlan


@mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', testsuite_filename_mock)
@mock.patch.object(TestPlan, 'SAMPLE_FILENAME', sample_filename_mock)
class TestOutfile:
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
        'flag_section, clobber, expect_straggler',
        [
            ([], True, False),
            (['--clobber-output'], False, False),
            (['--no-clean'], False, True),
            (['--clobber-output', '--no-clean'], False, True),
        ],
        ids=['clobber', 'do not clobber', 'do not clean', 'do not clobber, do not clean']
    )
    def test_clobber_output(self, out_path, flag_section, clobber, expect_straggler):
        test_platforms = ['qemu_x86', 'intel_adl_crb']
        path = os.path.join(TEST_DATA, 'tests', 'dummy')
        args = ['-i', '--outdir', out_path, '-T', path, '-y'] + \
               flag_section + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        # We create an empty 'blackbox-out' to trigger the clobbering
        os.mkdir(os.path.join(out_path))
        # We want to have a single straggler to check for
        straggler_name = 'atavi.sm'
        straggler_path = os.path.join(out_path, straggler_name)
        open(straggler_path, 'a').close()

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'

        expected_dirs = ['blackbox-out']
        if clobber:
            expected_dirs += ['blackbox-out.1']
        current_dirs = os.listdir(os.path.normpath(os.path.join(out_path, '..')))
        print(current_dirs)
        assert sorted(current_dirs) == sorted(expected_dirs)

        out_contents = os.listdir(os.path.join(out_path))
        print(out_contents)
        if expect_straggler:
            assert straggler_name in out_contents
        else:
            assert straggler_name not in out_contents

    def test_runtime_artifact_cleanup(self, out_path):
        test_platforms = ['qemu_x86', 'intel_adl_crb']
        path = os.path.join(TEST_DATA, 'samples', 'hello_world')
        args = ['-i', '--outdir', out_path, '-T', path] + \
               ['--runtime-artifact-cleanup'] + \
               [] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'

        relpath = os.path.relpath(path, ZEPHYR_BASE)
        sample_path = os.path.join(out_path, 'qemu_x86_atom', 'zephyr', relpath, 'sample.basic.helloworld')
        listdir = os.listdir(sample_path)
        zephyr_listdir = os.listdir(os.path.join(sample_path, 'zephyr'))

        expected_contents = ['CMakeFiles', 'handler.log', 'build.ninja', 'CMakeCache.txt',
                             'zephyr', 'build.log', 'build_info.yml']
        expected_zephyr_contents = ['.config', 'zephyr.dts']

        assert all([content in expected_zephyr_contents for content in zephyr_listdir]), \
               'Cleaned zephyr directory has unexpected files.'
        assert all([content in expected_contents for content in listdir]), \
               'Cleaned directory has unexpected files.'

    def test_short_build_path(self, out_path):
        test_platforms = ['qemu_x86']
        path = os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic', 'group2')
        # twister_links dir does not exist in a dry run.
        args = ['-i', '--outdir', out_path, '-T', path] + \
               ['--short-build-path'] + \
               ['--ninja'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        relative_test_path = os.path.relpath(path, ZEPHYR_BASE)
        test_result_path = os.path.join(out_path, 'qemu_x86_atom', 'zephyr',
                                        relative_test_path, 'dummy.agnostic.group2')

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'

        with open(os.path.join(out_path, 'twister.log')) as f:
            twister_log = f.read()

        pattern_running = r'Running\s+cmake\s+on\s+(?P<full_path>[\\\/].*)\s+for\s+qemu_x86/atom\s*\n'
        res_running = re.search(pattern_running, twister_log)
        assert res_running

        # Spaces, forward slashes, etc. in the path as well as CMake peculiarities
        # require us to forgo simple RegExes.
        pattern_calling_line = r'Calling cmake: [^\n]+$'
        res_calling = re.search(pattern_calling_line, twister_log[res_running.end():], re.MULTILINE)
        calling_line = res_calling.group()

        # HIGHLY DANGEROUS pattern!
        # If the checked text is not CMake flags only, it is exponential!
        # Where N is the length of non-flag space-delimited text section.
        flag_pattern = r'(?:\S+(?: \\)?)+- '
        cmake_path = shutil.which('cmake')
        if not cmake_path:
            assert False, 'CMake not found.'

        cmake_call_section = r'^Calling cmake: ' + re.escape(cmake_path)
        calling_line = re.sub(cmake_call_section, '', calling_line)
        calling_line = calling_line[::-1]
        flag_iterable = re.finditer(flag_pattern, calling_line)

        for match in flag_iterable:
            reversed_flag = match.group()
            flag = reversed_flag[::-1]

            # Build flag
            if flag.startswith(' -B'):
                flag_value = flag[3:]
                build_filename = os.path.basename(os.path.normpath(flag_value))
                unshortened_build_path = os.path.join(test_result_path, build_filename)
                assert flag_value != unshortened_build_path, 'Build path unchanged.'
                assert len(flag_value) < len(unshortened_build_path), 'Build path not shortened.'

            # Pipe flag
            if flag.startswith(' -DQEMU_PIPE='):
                flag_value = flag[13:]
                pipe_filename = os.path.basename(os.path.normpath(flag_value))
                unshortened_pipe_path = os.path.join(test_result_path, pipe_filename)
                assert flag_value != unshortened_pipe_path, 'Pipe path unchanged.'
                assert len(flag_value) < len(unshortened_pipe_path), 'Pipe path not shortened.'

    def test_prep_artifacts_for_testing(self, out_path):
        test_platforms = ['qemu_x86', 'intel_adl_crb']
        path = os.path.join(TEST_DATA, 'samples', 'hello_world')
        relative_test_path = os.path.relpath(path, ZEPHYR_BASE)
        zephyr_out_path = os.path.join(out_path, 'qemu_x86_atom', 'zephyr', relative_test_path,
                                       'sample.basic.helloworld', 'zephyr')
        args = ['-i', '--outdir', out_path, '-T', path] + \
               ['--prep-artifacts-for-testing'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'

        zephyr_artifact_list = os.listdir(zephyr_out_path)

        # --build-only and normal run leave more files than --prep-artifacts-for-testing
        # However, the cost of testing that this leaves less seems to outweigh the benefits.
        # So we'll only check for the most important artifact.
        assert 'zephyr.elf' in zephyr_artifact_list

    def test_package_artifacts(self, out_path):
        test_platforms = ['qemu_x86']
        path = os.path.join(TEST_DATA, 'samples', 'hello_world')
        package_name = 'PACKAGE'
        package_path = os.path.join(out_path, package_name)
        args = ['-i', '--outdir', out_path, '-T', path] + \
               ['--package-artifacts', package_path] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'

        # Check whether we have something as basic as zephyr.elf file
        with tarfile.open(package_path, "r") as tar:
            assert any([path.endswith('zephyr.elf') for path in tar.getnames()])

        # Delete everything but for the package
        for clean_up in os.listdir(os.path.join(out_path)):
            if not clean_up.endswith(package_name):
                clean_up_path = os.path.join(out_path, clean_up)
                if os.path.isfile(clean_up_path):
                    os.remove(clean_up_path)
                else:
                    shutil.rmtree(os.path.join(out_path, clean_up))

        # Unpack the package
        with tarfile.open(package_path, "r") as tar:
            tar.extractall(path=out_path)

        # Why does package.py put files inside the out_path folder?
        # It forces us to move files up one directory after extraction.
        file_names = os.listdir(os.path.join(out_path, os.path.basename(out_path)))
        for file_name in file_names:
            shutil.move(os.path.join(out_path, os.path.basename(out_path), file_name), out_path)

        args = ['-i', '--outdir', out_path, '-T', path] + \
               ['--test-only'] + \
               [val for pair in zip(
                   ['-p'] * len(test_platforms), test_platforms
               ) for val in pair]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        assert str(sys_exit.value) == '0'
