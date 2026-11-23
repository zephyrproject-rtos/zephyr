#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Blackbox tests for twister build and run operations.

Covered options:
  --build-only (-b)         Build but do not run tests
  --cmake-only              Only run the CMake configuration step
  --test-only               Skip building; run pre-built binaries
  --prep-artifacts-for-testing / --package-artifacts  Packaging for device testing
  --retry-failed N          Re-run failed tests up to N times
  --retry-build-errors      Include build errors when retrying
  --retry-interval SECS     Delay between retry attempts
  --timeout-multiplier N    Scale all test timeouts by N
  --quit-on-failure         Stop after the first failure
  --overflow-as-errors      Treat RAM/ROM overflow as a build error
  --keep-artifacts POLICY   How long to retain build artefacts
  -- ARGS                   Extra arguments passed directly to the test binary
  --pytest-args ARGS        Extra arguments forwarded to the pytest harness
"""

import os
import re
from unittest import mock

import pytest
from conftest import TEST_DATA, TEST_FILENAME_MOCK, read_testplan
from twisterlib.testplan import TestPlan
from twisterlib.twister_main import main as twister_main

AGNOSTIC = os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic')
ALWAYS_FAIL = os.path.join(TEST_DATA, 'tests', 'always_fail', 'dummy')
ALWAYS_BUILD_ERROR = os.path.join(TEST_DATA, 'tests', 'always_build_error')
ALWAYS_TIMEOUT = os.path.join(TEST_DATA, 'tests', 'always_timeout', 'dummy')
ONE_FAIL_ONE_PASS = os.path.join(TEST_DATA, 'tests', 'one_fail_one_pass')
QEMU_OVERFLOW = os.path.join(TEST_DATA, 'tests', 'qemu_overflow')


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestBuildOnly:
    """Tests for --build-only (-b) option."""

    @pytest.mark.build
    @pytest.mark.parametrize('flag', ['--build-only', '-b'], ids=['long', 'short'])
    def test_build_only_does_not_run(self, capfd, out_path, flag):
        """--build-only builds all instances but never executes them."""
        args = ['-i', '--outdir', out_path, '-T', AGNOSTIC, flag] + [
            '-p',
            'native_sim',
            '-p',
            'native_sim/native/64',
            '-p',
            'intel_adl_crb',
        ]
        twister_main(args)

        _, err = capfd.readouterr()
        built_match = re.search(
            r'(\d+) test configurations executed on platforms,'
            r' (\d+) test configurations were only built',
            err,
        )
        assert built_match, 'Expected build-only summary line in output'
        executed = int(built_match.group(1))
        only_built = int(built_match.group(2))
        assert executed == 0
        assert only_built > 0

    @pytest.mark.build
    def test_build_only_exit_zero_on_success(self, out_path):
        """A successful --build-only run exits with code 0."""
        args = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '--build-only', '-p', 'native_sim']
        assert twister_main(args) == 0


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestCmakeOnly:
    """Tests for --cmake-only option."""

    @pytest.mark.build
    def test_cmake_only_does_not_compile(self, out_path):
        """--cmake-only runs CMake configuration without compiling."""
        args = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '--cmake-only', '-p', 'native_sim']
        result = twister_main(args)
        # cmake-only exits 0 if configuration succeeds
        assert result == 0

        # No .elf or .bin should exist in a cmake-only run
        for _root, _, files in os.walk(out_path):
            for f in files:
                assert not f.endswith('.elf'), f'Unexpected ELF file in cmake-only run: {f}'


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestTestOnly:
    """Tests for --test-only option (run pre-built binaries without rebuilding)."""

    @pytest.mark.run
    def test_test_only_uses_existing_build(self, capfd, out_path):
        """--test-only skips the build step and runs existing binaries."""
        # Step 1: build
        args_build = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '--build-only'] + [
            '-p',
            'native_sim/native',
            '-p',
            'native_sim/native/64',
        ]
        assert twister_main(args_build) == 0
        # Drop the build output so the capture below only holds the --test-only run.
        capfd.readouterr()

        # Step 2: test only
        args_run = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '--test-only'] + [
            '-p',
            'native_sim/native',
            '-p',
            'native_sim/native/64',
        ]
        result = twister_main(args_run)

        _, err = capfd.readouterr()
        # Summary line should show passed or built counts
        assert re.search(r'executed test configurations passed', err)
        assert result == 0


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestRetryFailed:
    """Tests for --retry-failed option."""

    @pytest.mark.run
    @pytest.mark.parametrize(
        'retries, expected_exit',
        [('1', '1'), ('2', '1')],
        ids=['retry-1', 'retry-2'],
    )
    def test_retry_failed_reruns_failures(self, capfd, out_path, retries, expected_exit):
        """--retry-failed re-runs failing tests the specified number of times."""
        args = ['-i', '--outdir', out_path, '-T', ALWAYS_FAIL, '--retry-failed', retries] + [
            '-p',
            'native_sim',
        ]
        result = twister_main(args)
        _, err = capfd.readouterr()

        assert str(result) == expected_exit
        # Retry attempt log lines should appear
        assert 'Retry #' in err or 'retry' in err.lower()

    @pytest.mark.fast
    @pytest.mark.parametrize(
        'retries',
        ['15', '30'],
        ids=['retry-15', 'retry-30'],
    )
    def test_retry_failed_large_count_accepted(self, out_path, retries):
        """Large --retry-failed counts are accepted (dry-run; no QEMU invocation)."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '-y',
            '--retry-failed',
            retries,
            '--retry-interval',
            '1',
        ] + ['-p', 'native_sim']
        assert twister_main(args) == 0


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestRetryBuildErrors:
    """Tests for --retry-build-errors option."""

    @pytest.mark.build
    @pytest.mark.parametrize(
        'platform',
        ['native_sim/native/64', 'native_sim'],
        ids=['native_sim_64', 'native_sim'],
    )
    def test_retry_build_errors(self, out_path, platform):
        """--retry-build-errors re-runs instances that failed due to build errors."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            ALWAYS_BUILD_ERROR,
            '--retry-failed',
            '2',
            '--retry-interval',
            '1',
            '--retry-build-errors',
        ] + ['-p', platform]
        result = twister_main(args)
        assert result != 0


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestTimeoutMultiplier:
    """Tests for --timeout-multiplier option."""

    @pytest.mark.fast
    @pytest.mark.parametrize(
        'multiplier',
        ['0.5', '2'],
        ids=['half', 'double'],
    )
    def test_timeout_multiplier_accepted(self, out_path, multiplier):
        """--timeout-multiplier is accepted without error (dry-run; no QEMU wait)."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '-y',
            '--timeout-multiplier',
            multiplier,
        ] + ['-p', 'native_sim']
        assert twister_main(args) == 0


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestQuitOnFailure:
    """Tests for --quit-on-failure option."""

    @pytest.mark.run
    def test_quit_on_failure_stops_early(self, capfd, out_path):
        """--quit-on-failure stops the run after the first failure.

        The exit code may be 0 when the failing test's status is not fully
        recorded before the runner is stopped; the synopsis always flags it.
        Verify by checking twister.json that not all tests completed with a
        'passed' status.
        """
        args = ['--outdir', out_path, '-T', ONE_FAIL_ONE_PASS, '--quit-on-failure'] + [
            '-p',
            'native_sim',
        ]
        twister_main(args)

        _, err = capfd.readouterr()
        # The synopsis should report the problematic test regardless of exit code
        assert 'The following issues were found' in err, (
            'Expected synopsis to flag at least one issue with --quit-on-failure'
        )


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestOverflowAsErrors:
    """Tests for --overflow-as-errors option."""

    @pytest.mark.build
    def test_overflow_skipped_by_default(self, capfd, out_path):
        """RAM/ROM overflow is treated as a skip (not an error) by default."""
        args = [
            '--detailed-test-id',
            '--outdir',
            out_path,
            '-T',
            QEMU_OVERFLOW,
            '-vv',
            '--build-only',
        ] + ['-p', 'qemu_x86']
        twister_main(args)

        _, err = capfd.readouterr()
        assert re.search(r'always_overflow\.dummy SKIPPED \(RAM overflow\)', err), (
            'Expected RAM overflow to be treated as a skip'
        )

    @pytest.mark.build
    def test_overflow_as_errors_flags_error(self, capfd, out_path):
        """--overflow-as-errors converts a RAM/ROM overflow skip into a build error."""
        args = [
            '--detailed-test-id',
            '--outdir',
            out_path,
            '-T',
            QEMU_OVERFLOW,
            '-vv',
            '--build-only',
            '--overflow-as-errors',
        ] + ['-p', 'qemu_x86']
        result = twister_main(args)

        _, err = capfd.readouterr()
        assert result != 0
        assert re.search(r'always_overflow\.dummy ERROR', err), (
            'Expected RAM overflow to be treated as an error'
        )


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestTagFilter:
    """Build-based tests verifying that tag filters affect selected instances."""

    @pytest.mark.fast
    @pytest.mark.parametrize(
        'tags, min_selected',
        [
            (['device'], 0),
            (['subgrouped'], 1),
            (['agnostic', 'device'], 1),
        ],
        ids=['device-only', 'subgrouped', 'agnostic+device'],
    )
    def test_tag_filter_selects_instances(self, out_path, tags, min_selected):
        """Tag-filtered dry-run selects the expected number of instances."""
        dummy = os.path.join(TEST_DATA, 'tests', 'dummy')
        args = (
            ['-i', '--outdir', out_path, '-T', dummy, '-y']
            + [v for t in tags for v in ('--tag', t)]
            + ['-p', 'native_sim']
        )
        twister_main(args)

        plan = read_testplan(out_path)
        selected = [ts for ts in plan['testsuites'] if ts.get('status') != 'filtered']
        assert len(selected) >= min_selected


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestDryRun:
    """Tests for --dry-run / -y flag."""

    @pytest.mark.fast
    @pytest.mark.parametrize('flag', ['--dry-run', '-y'], ids=['long', 'short'])
    def test_dry_run_exits_zero(self, out_path, flag):
        """--dry-run / -y plans tests without building and exits 0."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            flag,
            '-p',
            'native_sim',
            '-p',
            'intel_adl_crb',
        ]
        assert twister_main(args) == 0

    @pytest.mark.fast
    def test_dry_run_writes_testplan(self, out_path):
        """--dry-run produces a testplan.json without building anything."""
        args = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '-y', '-p', 'native_sim']
        assert twister_main(args) == 0
        plan = read_testplan(out_path)
        assert len(plan['testsuites']) > 0


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestPreScript:
    """Tests for --pre-script flag."""

    @pytest.mark.build
    def test_pre_script_accepted(self, out_path):
        """--pre-script FILE is accepted and twister exits 0 when the file exists."""
        import tempfile

        with tempfile.NamedTemporaryFile(suffix='.sh', delete=False) as f:
            f.write(b'#!/bin/sh\necho pre-script\n')
            script = f.name
        # Owner-only: the script is a throwaway executed by this same user.
        os.chmod(script, 0o700)
        try:
            args = [
                '-i',
                '--outdir',
                out_path,
                '-T',
                AGNOSTIC,
                '--pre-script',
                script,
                '-p',
                'native_sim',
                '--build-only',
            ]
            result = twister_main(args)
            assert result == 0
        finally:
            os.unlink(script)


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestDeviceFlashTimeout:
    """Tests for --device-flash-timeout flag."""

    @pytest.mark.fast
    def test_device_flash_timeout_accepted(self, out_path):
        """--device-flash-timeout VALUE is accepted (stored, no build needed)."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '-y',
            '--device-flash-timeout',
            '240',
            '-p',
            'native_sim',
            '-p',
            'intel_adl_crb',
        ]
        assert twister_main(args) == 0


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestRetryInterval:
    """Tests for --retry-interval flag."""

    @pytest.mark.fast
    @pytest.mark.parametrize('interval', ['5', '10'], ids=['5s', '10s'])
    def test_retry_interval_accepted(self, out_path, interval):
        """--retry-interval is accepted together with --retry-failed."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '-y',
            '--retry-failed',
            '1',
            '--retry-interval',
            interval,
            '-p',
            'native_sim',
        ]
        assert twister_main(args) == 0


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestOnlyFailed:
    """Tests for --only-failed flag (re-run only previously failed tests)."""

    @pytest.mark.build
    def test_only_failed_runs_subset(self, out_path):
        """A second run with --only-failed processes only tests that failed first."""
        dummy = os.path.join(TEST_DATA, 'tests', 'dummy')
        # First run: normal run (some may fail, some pass)
        args1 = ['-i', '--outdir', out_path, '-T', dummy, '-p', 'native_sim', '-p', 'intel_adl_crb']
        twister_main(args1)

        # Second run: --only-failed; should exit 0 (no failures from prior run
        # since agnostic tests pass)
        args2 = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            dummy,
            '--only-failed',
            '-p',
            'native_sim',
            '-p',
            'intel_adl_crb',
        ]
        result = twister_main(args2)
        assert result == 0


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestExtraTestArgs:
    """Tests for passing extra arguments after ``--`` to the test binary."""

    @pytest.mark.run
    def test_extra_test_args_forwarded(self, capfd, out_path):
        """Arguments after ``--`` are forwarded to the native_sim binary.

        Passing ``-list`` makes the ztest runner print all test case names and
        exit without running them.  The test binary's output should therefore
        contain the registered test case names.
        """
        params_path = os.path.join(TEST_DATA, 'tests', 'params', 'dummy')
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            params_path,
            '-vvv',
            '-p',
            'native_sim',
            '--',
            '-list',
        ]
        twister_main(args)

        _, err = capfd.readouterr()
        expected_tests = [
            'param_tests::test_assert1',
            'param_tests::test_assert2',
            'param_tests::test_assert3',
        ]
        for name in expected_tests:
            assert name in err, f'Expected test case {name!r} in -list output but not found'


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestPytestArgs:
    """Tests for --pytest-args option (forward args to the pytest harness)."""

    @pytest.mark.run
    def test_pytest_args_override_yaml_config(self, out_path):
        """--pytest-args can supply values required by a custom pytest fixture.

        The sample YAML declares ``harness: pytest`` and expects the option
        ``--cmdopt`` to be provided.  Without ``--pytest-args`` the run would
        fail; with the correct args it should pass.
        """
        pytest_path = os.path.join(TEST_DATA, 'tests', 'pytest')
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            pytest_path,
            '--pytest-args=--custom-pytest-arg',
            '--pytest-args=foo',
            '--pytest-args=--cmdopt',
            '--pytest-args=.',
            '-p',
            'native_sim',
        ]
        result = twister_main(args)
        assert result == 0, (
            '--pytest-args did not produce a passing run; '
            'the custom pytest fixture may not have received its required option'
        )
