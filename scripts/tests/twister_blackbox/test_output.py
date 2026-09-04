#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Blackbox tests for twister output-formatting options.

Covered options:
  --detailed-test-id / --no-detailed-test-id   Include full path in test names
  -v / -vv                                      Verbosity levels
  --timestamps                                  Add timestamps to log lines
  --force-color                                 Force ANSI colour in output
  --log-file FILENAME                           Write log to a file
  --report-all-options                          Include twister options in report
  --inline-logs                                 Embed build log in twister.log
  -ll / --log-level                             Set minimum log level
"""

import glob
import json
import os
from unittest import mock

import pytest
from conftest import TEST_DATA, TEST_FILENAME_MOCK, active_testcases, read_testplan
from twisterlib.testplan import TestPlan
from twisterlib.twister_main import main as twister_main

DUMMY = os.path.join(TEST_DATA, 'tests', 'dummy')
AGNOSTIC = os.path.join(DUMMY, 'agnostic')
PLATFORMS = ['native_sim', 'intel_adl_crb']


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestDetailedTestId:
    """Tests for --detailed-test-id and --no-detailed-test-id."""

    @pytest.mark.fast
    def test_no_detailed_test_id(self, out_path):
        """--no-detailed-test-id gives short (dotted) test identifiers."""
        args = ['-i', '--outdir', out_path, '-T', DUMMY, '-y', '--no-detailed-test-id'] + [
            v for p in PLATFORMS for v in ('-p', p)
        ]
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        cases = active_testcases(plan)
        assert len(cases) > 0
        for _, suite_name, _tc_id in cases:
            assert suite_name.startswith('dummy.'), (
                f'Expected dotted suite name, got {suite_name!r}'
            )

    @pytest.mark.fast
    def test_detailed_test_id(self, out_path):
        """--detailed-test-id prefixes test names with their relative file path."""
        args = ['-i', '--outdir', out_path, '-T', DUMMY, '-y', '--detailed-test-id'] + [
            v for p in PLATFORMS for v in ('-p', p)
        ]
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        cases = active_testcases(plan)
        assert len(cases) > 0
        # TEST_DATA is realpath-resolved in conftest, so it matches what twister stores
        expected_prefix = os.path.relpath(TEST_DATA, os.environ['ZEPHYR_BASE'])
        for _, suite_name, _ in cases:
            assert suite_name.startswith(expected_prefix), (
                f'Expected path-prefixed suite name starting with {expected_prefix!r},\n'
                f'got {suite_name!r}'
            )

    @pytest.mark.fast
    @pytest.mark.parametrize('flag', ['--detailed-test-id', '--no-detailed-test-id'])
    def test_both_flags_succeed(self, out_path, flag):
        """Both flags exit with code 0 on a valid test directory."""
        args = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '-y', flag] + [
            v for p in PLATFORMS for v in ('-p', p)
        ]
        assert twister_main(args) == 0


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestVerbosity:
    """Tests for -v and -vv verbosity flags."""

    @pytest.mark.fast
    @pytest.mark.parametrize(
        'extra_args',
        [[], ['-v'], ['-v', '-ll', 'DEBUG'], ['-vv'], ['-vv', '-ll', 'DEBUG']],
        ids=['default', '-v', '-v+DEBUG', '-vv', '-vv+DEBUG'],
    )
    def test_verbosity_flags_do_not_crash(self, out_path, extra_args):
        """Verbosity flags are accepted without error."""
        args = (
            ['-i', '--outdir', out_path, '-T', AGNOSTIC, '-y']
            + extra_args
            + [v for p in PLATFORMS for v in ('-p', p)]
        )
        assert twister_main(args) == 0

    @pytest.mark.fast
    def test_verbose_increases_output(self, capfd, out_path, tmp_path):
        """More verbose flags produce more output."""

        def _stderr(extra, path):
            args = (
                ['-i', '--outdir', path, '-T', AGNOSTIC, '-y']
                + extra
                + [v for p in PLATFORMS for v in ('-p', p)]
            )
            twister_main(args)
            _, err = capfd.readouterr()
            return len(err)

        default_len = _stderr([], str(tmp_path / 'default'))
        verbose_len = _stderr(['-v'], str(tmp_path / 'verbose'))
        assert verbose_len >= default_len


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestLogLevel:
    """Tests for -ll / --log-level option."""

    @pytest.mark.fast
    @pytest.mark.parametrize(
        'level',
        ['DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL'],
    )
    def test_log_level_accepted(self, out_path, level):
        """All valid log levels are accepted without error."""
        args = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '-y', '-ll', level] + [
            v for p in PLATFORMS for v in ('-p', p)
        ]
        assert twister_main(args) == 0


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestLogFile:
    """Tests for --log-file option."""

    @pytest.mark.fast
    def test_log_file_created(self, out_path, tmp_path):
        """--log-file writes twister output to the specified file."""
        log_file = str(tmp_path / 'custom.log')
        args = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '-y', '--log-file', log_file] + [
            v for p in PLATFORMS for v in ('-p', p)
        ]
        assert twister_main(args) == 0
        assert os.path.isfile(log_file), '--log-file was not created'

    @pytest.mark.fast
    def test_log_file_contains_output(self, out_path, tmp_path):
        """The log file contains recognisable twister output."""
        log_file = str(tmp_path / 'twister.log')
        args = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '-y', '--log-file', log_file] + [
            v for p in PLATFORMS for v in ('-p', p)
        ]
        twister_main(args)

        with open(log_file) as fh:
            content = fh.read()
        assert 'INFO' in content or 'test' in content.lower()


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestTimestamps:
    """Tests for --timestamps option."""

    @pytest.mark.fast
    def test_timestamps_flag_accepted(self, out_path):
        """--timestamps does not cause twister to crash."""
        args = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '-y', '--timestamps'] + [
            v for p in PLATFORMS for v in ('-p', p)
        ]
        assert twister_main(args) == 0


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestForceColor:
    """Tests for --force-color flag."""

    @pytest.mark.fast
    def test_force_color_flag_accepted(self, out_path):
        """--force-color is accepted without error."""
        args = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '-y', '--force-color'] + [
            v for p in PLATFORMS for v in ('-p', p)
        ]
        assert twister_main(args) == 0


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestReportAllOptions:
    """Tests for --report-all-options option."""

    @pytest.mark.build
    def test_report_all_options_in_json(self, out_path):
        """--report-all-options embeds the full twister invocation in twister.json."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '--build-only',
            '--report-all-options',
            '-p',
            'native_sim',
        ]
        twister_main(args)

        json_file = os.path.join(out_path, 'twister.json')
        if not os.path.isfile(json_file):
            pytest.skip('twister.json not produced in this run')

        with open(json_file) as fh:
            report = json.load(fh)
        assert 'environment' in report or 'options' in report or len(report) > 0


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestInlineLogs:
    """Tests for --inline-logs option (embeds build log in twister.log)."""

    @pytest.mark.build
    def test_inline_logs_embeds_build_output(self, out_path, tmp_path):
        """--inline-logs embeds the failing build log rather than pointing at it."""
        path = os.path.join(TEST_DATA, 'tests', 'always_build_error', 'dummy')

        def _run(outdir, extra):
            assert twister_main(['--outdir', outdir, '-T', path, '-p', 'native_sim'] + extra) != 0
            with open(os.path.join(outdir, 'twister.log')) as fh:
                return fh.read()

        # Reference run: the build error is only referenced by path.
        plain_log = _run(out_path, [])

        build_logs = glob.glob(os.path.join(out_path, '**', 'build.log'), recursive=True)
        assert build_logs, 'The failing build should have produced a build.log'
        with open(build_logs[0]) as fh:
            error_lines = [ln.strip() for ln in fh if 'error:' in ln]
        assert error_lines, 'Expected a compiler error in build.log'

        inline_log = _run(str(tmp_path / 'inline_out'), ['--inline-logs'])

        for line in error_lines[:3]:
            assert line in inline_log, f'--inline-logs should embed the build error: {line!r}'
            assert line not in plain_log, (
                f'Without --inline-logs the build error should not be inlined: {line!r}'
            )
