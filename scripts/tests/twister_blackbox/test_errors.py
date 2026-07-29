#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Blackbox tests for twister error and failure conditions.

Covered scenarios:
  * Selecting a non-existent test suite (--test / -s)
  * Referencing an unknown platform
  * RAM/ROM overflow behaviour (--overflow-as-errors)
  * Build failure exit codes
  * --disable-suite-name-check suppresses mismatch messages
  * --disable-warnings-as-errors treats warnings as non-fatal
  * Native-sim random seed propagation (--seed)
"""

import os
import re
from unittest import mock

import pytest
from conftest import TEST_DATA, TEST_FILENAME_MOCK
from twisterlib.error import TwisterRuntimeError
from twisterlib.testplan import TestPlan
from twisterlib.twister_main import main as twister_main

DUMMY = os.path.join(TEST_DATA, 'tests', 'dummy')
AGNOSTIC = os.path.join(DUMMY, 'agnostic')
ALWAYS_BUILD_ERROR = os.path.join(TEST_DATA, 'tests', 'always_build_error')
QEMU_OVERFLOW = os.path.join(TEST_DATA, 'tests', 'qemu_overflow')
ALWAYS_WARNING = os.path.join(TEST_DATA, 'tests', 'always_warning')
SEED_NATIVE_SIM = os.path.join(TEST_DATA, 'tests', 'seed_native_sim')


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestInvalidTestSelection:
    """Tests for error conditions in test selection."""

    @pytest.mark.fast
    def test_unknown_test_suite_exits_nonzero(self, out_path, capsys):
        """--test with a non-existent suite name exits non-zero."""
        args = [
            '--detailed-test-id',
            '-i',
            '--outdir',
            out_path,
            '-y',
            '--test',
            'does.not.exist.anywhere',
            '-p',
            'native_sim',
        ]
        result = twister_main(args)

        captured = capsys.readouterr()
        assert result != 0
        assert 'No testsuites found' in captured.err

    @pytest.mark.fast
    def test_test_with_testroot_finds_suite(self, out_path):
        """--test succeeds when both -T and a matching suite name are given."""
        args = [
            '--detailed-test-id',
            '-i',
            '--outdir',
            out_path,
            '-y',
            '-T',
            DUMMY,
            '--test',
            'dummy.agnostic.group1.subgroup1',
            '-p',
            'native_sim',
            '-p',
            'intel_adl_crb',
        ]
        assert twister_main(args) == 0

    @pytest.mark.fast
    def test_test_path_form_finds_suite(self, out_path):
        """--test with an absolute path form of the identifier works."""
        # TEST_DATA is realpath-resolved in conftest; use that to form the path id
        zephyr_base = os.getenv('ZEPHYR_BASE')
        rel_test_data = os.path.relpath(TEST_DATA, zephyr_base)
        path_form = os.path.join(
            rel_test_data,
            'tests',
            'dummy',
            'agnostic',
            'group1',
            'subgroup1',
            'dummy.agnostic.group1.subgroup1',
        )
        args = [
            '--detailed-test-id',
            '-i',
            '--outdir',
            out_path,
            '-y',
            '-T',
            DUMMY,
            '--test',
            path_form,
            '-p',
            'native_sim',
        ]
        assert twister_main(args) == 0

    @pytest.mark.fast
    def test_invalid_subtest_path_raises(self, out_path):
        """--sub-test with a malformed path raises TwisterRuntimeError."""
        bad_id = os.path.join(
            'scripts',
            'tests',
            'twister_blackbox',
            'test_data',
            'tests',
            'dummy',
            'agnostic',
            'group1',
            'subgroup1',
            'dummy.agnostic.group2.assert1',  # wrong: group2 inside group1 path
        )
        args = [
            '--detailed-test-id',
            '-i',
            '--outdir',
            out_path,
            '-y',
            '-T',
            DUMMY,
            '--sub-test',
            bad_id,
            '-p',
            'native_sim',
        ]
        with pytest.raises(TwisterRuntimeError):
            twister_main(args)


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestBuildFailureExitCodes:
    """Tests for build failure exit codes."""

    @pytest.mark.build
    @pytest.mark.parametrize(
        'platforms, expected_exit',
        [
            (['native_sim/native/64'], '1'),
            (['native_sim', 'native_sim/native/64'], '1'),
        ],
        ids=['single-failure', 'multiple-failures'],
    )
    def test_build_error_exit_code(self, out_path, platforms, expected_exit):
        """Build errors produce exit code 1 regardless of the failure count."""
        args = ['-i', '--outdir', out_path, '-T', ALWAYS_BUILD_ERROR]
        for p in platforms:
            args += ['-p', p]
        result = twister_main(args)
        assert str(result) == expected_exit


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestOverflowHandling:
    """Tests for RAM/ROM overflow error handling."""

    @pytest.mark.build
    def test_overflow_default_is_skip(self, capfd, out_path):
        """By default, a RAM overflow is reported as a SKIPPED test."""
        args = [
            '--detailed-test-id',
            '--outdir',
            out_path,
            '-T',
            QEMU_OVERFLOW,
            '-vv',
            '--build-only',
            '-p',
            'qemu_x86',
        ]
        twister_main(args)

        _, err = capfd.readouterr()
        assert re.search(r'always_overflow\.dummy SKIPPED \(RAM overflow\)', err), (
            'Expected RAM overflow to be treated as a skip by default'
        )

    @pytest.mark.build
    def test_overflow_as_errors_converts_to_error(self, capfd, out_path):
        """--overflow-as-errors converts a RAM overflow skip into an error."""
        args = [
            '--detailed-test-id',
            '--outdir',
            out_path,
            '-T',
            QEMU_OVERFLOW,
            '-vv',
            '--build-only',
            '--overflow-as-errors',
            '-p',
            'qemu_x86',
        ]
        result = twister_main(args)

        _, err = capfd.readouterr()
        assert result != 0
        assert re.search(r'always_overflow\.dummy ERROR', err), (
            'Expected RAM overflow to be an error with --overflow-as-errors'
        )


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestNativeSeed:
    """Tests for --seed option (native_sim binary seed)."""

    @pytest.mark.run
    @pytest.mark.parametrize('seed', [1234, 4321, 1324])
    def test_seed_passed_to_native_sim(self, capfd, out_path, seed):
        """--seed is forwarded to the native_sim binary as a command-line argument."""
        args = [
            '--no-detailed-test-id',
            '--outdir',
            out_path,
            '-i',
            '-T',
            SEED_NATIVE_SIM,
            '-vv',
            '--seed',
            str(seed),
            '-p',
            'native_sim',
        ]
        result = twister_main(args)

        _, err = capfd.readouterr()
        assert result == 1  # always_fail with seed triggers a failure
        pattern = (
            r'seed_native_sim\.dummy\s+FAILED.*'
            rf'seed:\s*{seed}'
        )
        assert re.search(pattern, err), (
            f'Expected seed {seed} in failure message; output was:\n{err}'
        )


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestWarningsHandling:
    """Tests for --disable-warnings-as-errors option."""

    @pytest.mark.build
    def test_warnings_as_errors_by_default(self, out_path):
        """By default, compiler warnings cause a test failure."""

    @pytest.mark.build
    def test_disable_warnings_as_errors(self, out_path):
        """--disable-warnings-as-errors makes warnings non-fatal."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            ALWAYS_WARNING,
            '-vv',
            '--build-only',
            '--disable-warnings-as-errors',
            '-p',
            'native_sim',
        ]
        result = twister_main(args)
        assert result == 0


class TestCliArgErrors:
    """Tests for invalid CLI arguments and --help."""

    @pytest.mark.fast
    @pytest.mark.parametrize(
        'flag, expected_exit',
        [
            ('-%', 2),
            ('--1234', 2),
            ('--abcd', 2),
            ('-1', 1),
        ],
        ids=['percent', 'numeric-long', 'alpha-long', 'minus-one'],
    )
    def test_broken_parameter_exits_nonzero(self, flag, expected_exit):
        """Invalid/unknown flags must exit with a non-zero code."""
        with pytest.raises(SystemExit) as exc_info:
            twister_main([flag])
        assert exc_info.value.code == expected_exit, (
            f'Expected exit {expected_exit} for {flag!r}, got {exc_info.value.code!r}'
        )

    @pytest.mark.fast
    @pytest.mark.parametrize('flag', ['-h', '--help'], ids=['short', 'long'])
    def test_help_exits_zero(self, flag):
        """--help / -h must print usage and exit with code 0."""
        with pytest.raises(SystemExit) as exc_info:
            twister_main([flag])
        assert exc_info.value.code == 0, (
            f'Expected exit 0 for {flag!r}, got {exc_info.value.code!r}'
        )
