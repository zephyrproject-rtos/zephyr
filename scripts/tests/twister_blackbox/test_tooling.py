#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Blackbox tests for twister toolchain and build-system options.

Covered options:
  --force-toolchain        Do not check whether a toolchain supports the target
  -N / --ninja             Use Ninja as the CMake generator (the default)
  -k / --make              Use Unix Makefiles as the CMake generator
  --short-build-path       Use shortened build directories
  -j / --jobs              Number of parallel build jobs
  --extra-args (-x)        Pass extra CMake definitions
"""

import glob
import os
from unittest import mock

import pytest
from conftest import TEST_DATA, TEST_FILENAME_MOCK, read_testplan
from twisterlib.testplan import TestPlan
from twisterlib.twister_main import main as twister_main

AGNOSTIC = os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic')
GROUP2 = os.path.join(AGNOSTIC, 'group2')

# xt-sim declares 'toolchain: [xcc]', so it is rejected under the Zephyr SDK
# unless --force-toolchain is given. --force-platform is needed alongside it to
# get past the suites' platform_allow list.
UNSUPPORTED_TOOLCHAIN_PLATFORM = 'xt-sim'


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestForceToolchain:
    """Tests for --force-toolchain option."""

    def _plan_size(self, out_path, extra):
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '-y',
            '-p',
            UNSUPPORTED_TOOLCHAIN_PLATFORM,
            '--force-platform',
        ] + extra
        assert twister_main(args) == 0
        return len(read_testplan(out_path)['testsuites'])

    @pytest.mark.fast
    def test_platform_filtered_without_force_toolchain(self, out_path):
        """A platform that does not list the active toolchain is filtered out."""
        assert self._plan_size(out_path, []) == 0

    @pytest.mark.fast
    def test_force_toolchain_keeps_unsupported_platform(self, out_path):
        """--force-toolchain suppresses the toolchain compatibility filter."""
        assert self._plan_size(out_path, ['--force-toolchain']) > 0


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestGenerator:
    """Tests for the -N/--ninja and -k/--make generator options."""

    @pytest.mark.build
    @pytest.mark.parametrize(
        'flags, expected, unexpected',
        [
            ([], 'build.ninja', 'Makefile'),
            (['-N'], 'build.ninja', 'Makefile'),
            (['--ninja'], 'build.ninja', 'Makefile'),
            (['-k'], 'Makefile', 'build.ninja'),
            (['--make'], 'Makefile', 'build.ninja'),
        ],
        ids=['default', '-N', '--ninja', '-k', '--make'],
    )
    def test_generator_selects_build_system(self, out_path, flags, expected, unexpected):
        """The generator flags decide which build system CMake emits."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            GROUP2,
            '--cmake-only',
            '-p',
            'native_sim',
        ] + flags
        assert twister_main(args) == 0

        def _found(name):
            return glob.glob(os.path.join(out_path, '**', name), recursive=True)

        assert _found(expected), f'{flags or "(default)"} should generate {expected}'
        assert not _found(unexpected), f'{flags or "(default)"} should not generate {unexpected}'


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestShortBuildPath:
    """Tests for --short-build-path option.

    The option does not make the output tree shallower; it builds via a
    'twister_links' directory of short symlinks that point at the long build
    directories, so that the path handed to CMake stays short (this matters on
    Windows).
    """

    LINKS_DIR = 'twister_links'

    def _run(self, outdir, extra):
        args = [
            '-i',
            '--outdir',
            outdir,
            '-T',
            GROUP2,
            '--cmake-only',
            '-p',
            'native_sim',
        ] + extra
        assert twister_main(args) == 0

    @pytest.mark.build
    def test_no_link_dir_by_default(self, out_path):
        """Without --short-build-path no link directory is created."""
        self._run(out_path, [])
        assert not os.path.exists(os.path.join(out_path, self.LINKS_DIR))

    @pytest.mark.build
    def test_short_build_path_creates_symlinks(self, out_path):
        """--short-build-path links each build directory under twister_links."""
        self._run(out_path, ['--short-build-path'])

        links_dir = os.path.join(out_path, self.LINKS_DIR)
        assert os.path.isdir(links_dir), f'{self.LINKS_DIR} was not created'

        links = os.listdir(links_dir)
        assert links, f'{self.LINKS_DIR} is empty'
        for link in links:
            link_path = os.path.join(links_dir, link)
            assert os.path.islink(link_path), f'{link} is not a symlink'
            # The link must be shorter than the build directory it stands in for.
            assert len(link_path) < len(os.path.realpath(link_path))


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestParallelJobs:
    """Tests for -j / --jobs option."""

    @pytest.mark.build
    @pytest.mark.parametrize('jobs', ['1', '2', '4'], ids=['1-job', '2-jobs', '4-jobs'])
    def test_jobs_flag_sets_job_count(self, out_path, jobs):
        """-j / --jobs sets the number of parallel jobs twister reports."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            GROUP2,
            '--cmake-only',
            '-j',
            jobs,
            '-p',
            'native_sim',
        ]
        assert twister_main(args) == 0

        with open(os.path.join(out_path, 'twister.log')) as fh:
            log_text = fh.read()
        assert f'JOBS: {jobs}' in log_text, f'Expected "JOBS: {jobs}" in twister.log'


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestExtraArgs:
    """Tests for -x / --extra-args option."""

    @pytest.mark.build
    def test_extra_args_appear_in_cmake_invocation(self, out_path):
        """--extra-args values appear verbatim in the cmake invocation logged to
        twister.log."""
        args = [
            '--outdir',
            out_path,
            '-T',
            GROUP2,
            '--extra-args',
            'USE_CCACHE=0',
            '--extra-args',
            'DUMMY=1',
            '-p',
            'native_sim',
            '-p',
            'intel_adl_crb',
        ]
        assert twister_main(args) == 0

        log_path = os.path.join(out_path, 'twister.log')
        if not os.path.isfile(log_path):
            pytest.skip('twister.log not produced')
        with open(log_path) as fh:
            log_text = fh.read()

        assert ' -DUSE_CCACHE=0 ' in log_text, (
            '--extra-args USE_CCACHE=0 not found in cmake invocation in twister.log'
        )
        assert ' -DDUMMY=1 ' in log_text, (
            '--extra-args DUMMY=1 not found in cmake invocation in twister.log'
        )
