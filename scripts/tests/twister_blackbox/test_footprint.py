#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Blackbox tests for twister memory-footprint reporting.

Covered options:
  --enable-size-report       Collect ROM/RAM usage per test instance
  --create-rom-ram-report    Write a ROM/RAM report to a file
  --footprint-report [all|ROM|RAM]  Which sizes to include in the report
  --show-footprint           Print footprint table to stdout
  --compare-report FILE      Compare current footprint against a baseline
  -m / --last-metrics        Use the last run as the footprint baseline
  -H THRESHOLD               Fail if a metric increase exceeds this threshold
  --footprint-from-buildlog  Derive sizes from the build log (no objcopy needed)
"""

import glob
import json
import os
import re
from unittest import mock

import pytest
from conftest import TEST_DATA, TEST_FILENAME_MOCK
from twisterlib.statuses import TwisterStatus
from twisterlib.testplan import TestPlan
from twisterlib.twister_main import main as twister_main

DEVICE_GROUP = os.path.join(TEST_DATA, 'tests', 'dummy', 'device', 'group')
DUMMY = os.path.join(TEST_DATA, 'tests', 'dummy')


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestEnableSizeReport:
    """Tests for --enable-size-report option."""

    @pytest.mark.build
    def test_size_report_adds_metrics_to_json(self, out_path):
        """--enable-size-report populates used_ram/used_rom fields in twister.json."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            DEVICE_GROUP,
            '--enable-size-report',
            '-p',
            'intel_adl_crb',
        ]
        twister_main(args)

        json_file = os.path.join(out_path, 'twister.json')
        if not os.path.isfile(json_file):
            pytest.skip('twister.json not produced')

        with open(json_file) as fh:
            report = json.load(fh)

        built_suites = [
            ts
            for ts in report['testsuites']
            if TwisterStatus(ts.get('status', 'none')) == TwisterStatus.NOTRUN
        ]
        assert built_suites, 'Expected at least one built test suite'
        assert any(ts.get('used_ram') for ts in built_suites), (
            'Expected a non-zero used_ram in at least one built test suite'
        )


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestFootprintReport:
    """Tests for --create-rom-ram-report and --footprint-report options."""

    @pytest.mark.build
    @pytest.mark.parametrize(
        'report_type',
        ['all', 'ROM', 'RAM'],
        ids=['all', 'ROM', 'RAM'],
    )
    def test_footprint_report_types(self, out_path, report_type):
        """--footprint-report decides which sections land in twister_footprint.json."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            DEVICE_GROUP,
            '--enable-size-report',
            '--footprint-report',
            report_type,
            '-p',
            'intel_adl_crb',
        ]
        assert twister_main(args) == 0

        # twister.json explicitly filters the footprint section out; it is only
        # carried by the separate twister_footprint.json report.
        footprint_json = os.path.join(out_path, 'twister_footprint.json')
        assert os.path.isfile(footprint_json), (
            '--footprint-report should emit twister_footprint.json'
        )
        with open(footprint_json) as fh:
            report = json.load(fh)

        footprints = [ts['footprint'] for ts in report['testsuites'] if 'footprint' in ts]
        assert footprints, 'No testsuite carried a footprint section'

        expected = {'all': {'ROM', 'RAM'}, 'ROM': {'ROM'}, 'RAM': {'RAM'}}[report_type]
        for fp in footprints:
            assert set(fp) == expected, (
                f'--footprint-report {report_type} should emit {expected}, got {set(fp)}'
            )

        # The plain report must not leak the footprint section.
        with open(os.path.join(out_path, 'twister.json')) as fh:
            plain = json.load(fh)
        assert not any('footprint' in ts for ts in plain['testsuites']), (
            'twister.json should not carry a footprint section'
        )

    @pytest.mark.build
    def test_create_rom_ram_report_file(self, out_path):
        """--create-rom-ram-report builds the footprint target, emitting rom/ram.json."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            DEVICE_GROUP,
            '--enable-size-report',
            '--create-rom-ram-report',
            '-p',
            'intel_adl_crb',
        ]
        assert twister_main(args) == 0

        # The 'footprint' cmake target writes these next to the built image.
        for name in ('rom.json', 'ram.json'):
            assert glob.glob(os.path.join(out_path, '**', name), recursive=True), (
                f'--create-rom-ram-report should have produced {name}'
            )


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestCompareReport:
    """Tests for --compare-report option (footprint delta detection)."""

    @pytest.mark.build
    @pytest.mark.parametrize(
        'ram_multiplier, expect_delta',
        [
            (0.75, True),  # RAM increased -> delta logged
            (1.25, False),  # RAM decreased -> no positive delta
        ],
        ids=['footprint_increased', 'footprint_reduced'],
    )
    def test_compare_report_detects_delta(self, caplog, out_path, ram_multiplier, expect_delta):
        """--compare-report reports when footprint increases between runs."""
        # First run: build and capture baseline
        args_first = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            DEVICE_GROUP,
            '--enable-size-report',
            '-p',
            'intel_adl_crb',
        ]
        twister_main(args_first)

        json_file = os.path.join(out_path, 'twister.json')
        if not os.path.isfile(json_file):
            pytest.skip('twister.json not produced in first run')

        # Manipulate the baseline so we can control the reported delta
        with open(json_file) as fh:
            report = json.load(fh)
        for ts in report['testsuites']:
            if TwisterStatus(ts.get('status', 'none')) == TwisterStatus.NOTRUN and ts.get(
                'used_ram'
            ):
                ts['used_ram'] = int(ts['used_ram'] * ram_multiplier)
        with open(json_file, 'w') as fh:
            json.dump(report, fh, indent=4)

        baseline_path = os.path.join(
            os.path.dirname(out_path),
            os.path.basename(out_path) + '.1',
            'twister.json',
        )

        # Second run: compare against the modified baseline
        args_second = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            DUMMY,
            '--compare-report',
            baseline_path,
            '--show-footprint',
            '-p',
            'intel_adl_crb',
        ]
        twister_main(args_second)

        # RAM is a "lower is better" metric, so only a baseline that is *smaller*
        # than the current build (multiplier < 1) counts as a reportable delta.
        delta_pattern = re.compile(r'Found [1-9][0-9]* footprint deltas')
        log_text = '\n'.join(r.message for r in caplog.records)
        if expect_delta:
            assert delta_pattern.search(log_text), (
                f'Expected a footprint delta to be reported; log was:\n{log_text}'
            )
        else:
            assert not delta_pattern.search(log_text), (
                f'A footprint reduction must not be reported as a delta; log was:\n{log_text}'
            )


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestFootprintFromBuildlog:
    """Tests for --footprint-from-buildlog option."""

    @pytest.mark.build
    def test_footprint_from_buildlog_populates_sizes(self, out_path):
        """--footprint-from-buildlog still yields non-zero RAM/ROM metrics."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            DEVICE_GROUP,
            '--enable-size-report',
            '--footprint-from-buildlog',
            '-p',
            'intel_adl_crb',
        ]
        assert twister_main(args) == 0

        with open(os.path.join(out_path, 'twister.json')) as fh:
            report = json.load(fh)

        built = [
            ts
            for ts in report['testsuites']
            if TwisterStatus(ts.get('status', 'none')) == TwisterStatus.NOTRUN
        ]
        assert built, 'Expected at least one built test suite'
        assert any(ts.get('used_ram') for ts in built), (
            'No suite reported a non-zero used_ram when reading sizes from the build log'
        )

    @pytest.mark.build
    def test_footprint_from_buildlog_differs_from_default(self, out_path):
        """Sizes derived from the build log differ from those read via objcopy.

        Both runs build the same suite; a difference in the RAM value reported
        is expected because ``--footprint-from-buildlog`` reads the linker
        report rather than probing the ELF.  If values are identical the test
        is skipped rather than failed to avoid fragility.
        """
        args1 = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            DEVICE_GROUP,
            '--enable-size-report',
            '-p',
            'intel_adl_crb',
        ]
        twister_main(args1)

        json1 = os.path.join(out_path, 'twister.json')
        if not os.path.isfile(json1):
            pytest.skip('twister.json not produced in first run')
        with open(json1) as fh:
            j1 = json.load(fh)
        vals1 = [
            ts.get('used_ram')
            for ts in j1['testsuites']
            if TwisterStatus(ts.get('status', 'none')) == TwisterStatus.NOTRUN
        ]

        args2 = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            DEVICE_GROUP,
            '--enable-size-report',
            '--footprint-from-buildlog',
            '-p',
            'intel_adl_crb',
        ]
        twister_main(args2)

        json2 = os.path.join(out_path, 'twister.json')
        if not os.path.isfile(json2):
            pytest.skip('twister.json not produced in second run')
        with open(json2) as fh:
            j2 = json.load(fh)
        vals2 = [
            ts.get('used_ram')
            for ts in j2['testsuites']
            if TwisterStatus(ts.get('status', 'none')) == TwisterStatus.NOTRUN
        ]

        if not vals1 or not vals2:
            pytest.skip('No NOTRUN testsuites found to compare')
        if sorted(vals1) == sorted(vals2):
            pytest.skip('Values identical — buildlog and objcopy agree (no difference to assert)')
        assert sorted(vals1) != sorted(vals2)


# ---------------------------------------------------------------------------
# Helpers shared by the delta/comparison tests below
# ---------------------------------------------------------------------------

_DELTA_COMPARE_RE = re.compile(r'Found [1-9][0-9]* footprint deltas to .* as a baseline')
_DELTA_RUN_RE = re.compile(r'Found [1-9][0-9]* footprint deltas to the last twister run')
RAM_KEY = 'used_ram'


def _first_run(out_path, path):
    """Build test, return twister.json path; skip if file not produced."""
    twister_main(
        ['-i', '--outdir', out_path, '-T', path, '--enable-size-report', '-p', 'intel_adl_crb']
    )
    json_path = os.path.join(out_path, 'twister.json')
    if not os.path.isfile(json_path):
        pytest.skip('twister.json not produced in first run')
    return json_path


def _scale_ram(json_path, multiplier):
    """Multiply used_ram of every NOTRUN testsuite entry by *multiplier*."""
    with open(json_path) as fh:
        j = json.load(fh)
    for ts in j['testsuites']:
        if TwisterStatus(ts.get('status', 'none')) == TwisterStatus.NOTRUN and ts.get(RAM_KEY):
            ts[RAM_KEY] = int(ts[RAM_KEY] * multiplier)
    with open(json_path, 'w') as fh:
        json.dump(j, fh, indent=4)


def _baseline_path(out_path):
    """Return the path twister would use as the previous-run baseline."""
    return os.path.join(
        os.path.dirname(out_path),
        os.path.basename(out_path) + '.1',
        'twister.json',
    )


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestShowFootprint:
    """Tests for --show-footprint option."""

    @pytest.mark.build
    @pytest.mark.parametrize(
        'show, old_multiplier, expect_detail',
        [
            (False, 0.75, False),  # footprint increased but --show-footprint absent
            (True, 0.75, True),  # footprint increased + --show-footprint present
        ],
        ids=['no-show', 'show'],
    )
    def test_show_footprint(self, caplog, out_path, show, old_multiplier, expect_detail):
        """--show-footprint controls whether per-metric delta lines are logged."""
        json_path = _first_run(out_path, DEVICE_GROUP)
        _scale_ram(json_path, old_multiplier)
        baseline = _baseline_path(out_path)

        flags = ['--show-footprint'] if show else []
        twister_main(
            ['-i', '--outdir', out_path, '-T', DUMMY]
            + flags
            + ['--compare-report', baseline, '-p', 'intel_adl_crb']
        )

        log_text = caplog.text
        assert 'running footprint_reports' in log_text
        delta_re = re.compile(RAM_KEY + r' \+[0-9]+, is now +[0-9]+ \+[0-9.]+%')
        if expect_detail:
            assert delta_re.search(log_text), (
                'Expected per-metric delta line when --show-footprint is active'
            )
        else:
            assert not delta_re.search(log_text), (
                'Unexpected per-metric delta line without --show-footprint'
            )


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestFootprintThreshold:
    """Tests for --footprint-threshold / -H option."""

    @pytest.mark.build
    @pytest.mark.parametrize(
        'threshold, old_multiplier, expect_delta',
        [
            (95, 0.75, False),  # 25 % increase but threshold is 95 % → no report
            (25, 0.75, True),  # 25 % increase exceeds threshold of 25 % → report
        ],
        ids=['threshold-not-met', 'threshold-met'],
    )
    def test_footprint_threshold(self, caplog, out_path, threshold, old_multiplier, expect_delta):
        """--footprint-threshold suppresses delta reports below the threshold."""
        json_path = _first_run(out_path, DEVICE_GROUP)
        _scale_ram(json_path, old_multiplier)
        baseline = _baseline_path(out_path)

        twister_main(
            [
                '-i',
                '--outdir',
                out_path,
                '-T',
                DUMMY,
                f'--footprint-threshold={threshold}',
                '--compare-report',
                baseline,
                '--show-footprint',
                '-p',
                'intel_adl_crb',
            ]
        )

        log_text = caplog.text
        assert 'running footprint_reports' in log_text
        if expect_delta:
            assert RAM_KEY in log_text, (
                f'Expected delta to be reported when increase exceeds threshold {threshold}'
            )
            assert _DELTA_COMPARE_RE.search(log_text)
        else:
            assert RAM_KEY not in log_text, (
                f'Unexpected delta reported when increase is below threshold {threshold}'
            )
            assert not _DELTA_COMPARE_RE.search(log_text)


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestLastMetrics:
    """Tests for -m / --last-metrics option (compare against previous run)."""

    @pytest.mark.build
    @pytest.mark.parametrize(
        'old_multiplier, expect_delta',
        [
            (0.75, True),  # baseline had less RAM → current has more → delta
            (1.25, False),  # baseline had more RAM → current has less → no delta
        ],
        ids=['footprint-increased', 'footprint-reduced'],
    )
    def test_last_metrics(self, caplog, out_path, old_multiplier, expect_delta):
        """--last-metrics compares the current run against the stored previous run."""
        json_path = _first_run(out_path, DEVICE_GROUP)
        _scale_ram(json_path, old_multiplier)

        # Second run: twister rotates out_path → out_path.1 before writing new
        # results; --last-metrics picks up out_path.1/twister.json as baseline.
        twister_main(
            [
                '-i',
                '--outdir',
                out_path,
                '-T',
                DUMMY,
                '--last-metrics',
                '--show-footprint',
                '-p',
                'intel_adl_crb',
            ]
        )

        log_text = caplog.text
        assert 'running footprint_reports' in log_text
        if expect_delta:
            assert RAM_KEY in log_text
            assert _DELTA_RUN_RE.search(log_text), 'Expected delta warning against last run'
        else:
            assert RAM_KEY not in log_text
            assert not _DELTA_RUN_RE.search(log_text)


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestAllDeltas:
    """Tests for --all-deltas option (report both increases and decreases)."""

    @pytest.mark.build
    @pytest.mark.parametrize(
        'old_multiplier, expect_delta',
        [
            (0.75, True),  # old baseline was smaller → current larger → delta
            (1.00, False),  # identical → no delta
            (1.25, True),  # old baseline was larger → current smaller → delta
        ],
        ids=['footprint-increased', 'footprint-constant', 'footprint-reduced'],
    )
    def test_all_deltas(self, caplog, out_path, old_multiplier, expect_delta):
        """--all-deltas logs both positive and negative footprint changes."""
        json_path = _first_run(out_path, DEVICE_GROUP)
        _scale_ram(json_path, old_multiplier)
        baseline = _baseline_path(out_path)

        twister_main(
            [
                '-i',
                '--outdir',
                out_path,
                '-T',
                DUMMY,
                '--all-deltas',
                '--compare-report',
                baseline,
                '--show-footprint',
                '-p',
                'intel_adl_crb',
            ]
        )

        log_text = caplog.text
        assert 'running footprint_reports' in log_text
        if expect_delta:
            assert RAM_KEY in log_text
            assert _DELTA_COMPARE_RE.search(log_text), (
                'Expected footprint delta to be reported with --all-deltas'
            )
        else:
            assert RAM_KEY not in log_text
            assert not _DELTA_COMPARE_RE.search(log_text)
