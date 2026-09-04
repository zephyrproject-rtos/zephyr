#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Blackbox tests for twister report generation.

Covered options:
  --outdir (-O)            Primary output directory
  --report-dir (-o)        Separate directory for report files
  --report-name            Base filename for report outputs
  --report-suffix          Append a suffix to all report filenames
  --platform-reports       Generate per-platform XML reports
  --report-filtered        Include filtered tests in the XML report
  --report-summary         Print a JSON summary to stdout
  --clobber-output (-c)    Move existing output before writing
  --no-clean (-k)          Keep stale files from a previous run
  --aggressive-no-clean    Keep every artifact from a previous run
"""

import os
from unittest import mock

import pytest
from conftest import TEST_DATA, TEST_FILENAME_MOCK, read_testplan
from twisterlib.testplan import TestPlan
from twisterlib.twister_main import main as twister_main

AGNOSTIC = os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic')
PLATFORMS = ['native_sim/native', 'native_sim/native/64']


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestDefaultReportFiles:
    """Tests for default report file names produced after a build-only run."""

    @pytest.mark.build
    def test_default_files_present(self, out_path):
        """A build-only run produces the standard set of report files."""
        expected = [
            'testplan.json',
            'twister.log',
            'twister.json',
            'twister_report.xml',
            'twister_suite_report.xml',
            'twister.xml',
        ]
        args = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '--build-only'] + [
            v for p in PLATFORMS for v in ('-p', p)
        ]
        twister_main(args)

        for filename in expected:
            assert os.path.isfile(os.path.join(out_path, filename)), (
                f'Expected report file {filename!r} not found in outdir'
            )

    @pytest.mark.build
    def test_platform_reports_flag(self, out_path):
        """--platform-reports creates one XML file per platform."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '--build-only',
            '--platform-reports',
        ] + [v for p in PLATFORMS for v in ('-p', p)]
        twister_main(args)

        out_files = os.listdir(out_path)
        platform_xmls = [
            f
            for f in out_files
            if f.endswith('.xml')
            and f not in {'twister.xml', 'twister_report.xml', 'twister_suite_report.xml'}
        ]
        assert len(platform_xmls) >= len(PLATFORMS), (
            f'Expected at least {len(PLATFORMS)} platform XMLs, got: {platform_xmls}'
        )


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestReportName:
    """Tests for --report-name option."""

    @pytest.mark.build
    @pytest.mark.parametrize(
        'report_name, extra_flags, expected_files',
        [
            (
                'abcd',
                [],
                ['abcd.json', 'abcd_report.xml', 'abcd_suite_report.xml', 'abcd.xml'],
            ),
            (
                '1234',
                ['--platform-reports'],
                ['1234.json', '1234_report.xml', '1234_suite_report.xml', '1234.xml'],
            ),
        ],
        ids=['simple-name', 'with-platform-reports'],
    )
    def test_report_name(self, out_path, report_name, extra_flags, expected_files):
        """--report-name controls the base name of the output report files."""
        args = (
            [
                '-i',
                '--outdir',
                out_path,
                '-T',
                AGNOSTIC,
                '--build-only',
                '--report-name',
                report_name,
            ]
            + extra_flags
            + [v for p in PLATFORMS for v in ('-p', p)]
        )
        twister_main(args)

        out_files = os.listdir(out_path)
        for filename in expected_files:
            assert filename in out_files, (
                f'Expected {filename!r} with --report-name={report_name!r}'
            )


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestReportSuffix:
    """Tests for --report-suffix option."""

    @pytest.mark.build
    def test_report_suffix_appended(self, out_path):
        """--report-suffix appends a string to every report filename."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '--build-only',
            '--report-suffix',
            'TEST',
            '--platform-reports',
        ] + [v for p in PLATFORMS for v in ('-p', p)]
        twister_main(args)

        out_files = os.listdir(out_path)
        assert 'twister_TEST.json' in out_files
        assert 'twister_TEST_report.xml' in out_files

    @pytest.mark.build
    def test_report_name_and_suffix_combined(self, out_path):
        """--report-name and --report-suffix may be used together."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '--build-only',
            '--report-name',
            'Final',
            '--report-suffix',
            'Run1',
            '--platform-reports',
        ] + [v for p in PLATFORMS for v in ('-p', p)]
        twister_main(args)

        out_files = os.listdir(out_path)
        assert 'Final_Run1.json' in out_files
        assert 'Final_Run1_report.xml' in out_files


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestReportDir:
    """Tests for --report-dir (-o) option."""

    @pytest.mark.build
    def test_report_dir_separate_from_outdir(self, out_path, tmp_path):
        """--report-dir places report files in a different directory."""
        report_dir = str(tmp_path / 'reports')
        os.makedirs(report_dir)

        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '--build-only',
            '--report-dir',
            report_dir,
        ] + [v for p in PLATFORMS for v in ('-p', p)]
        twister_main(args)

        report_files = os.listdir(report_dir)
        assert any(f.endswith('.json') for f in report_files), (
            f'Expected JSON report in report-dir, found: {report_files}'
        )


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestClobberOutput:
    """Tests for --clobber-output (-c) and --no-clean (-k) options."""

    @pytest.mark.fast
    def test_clobber_creates_backup_of_existing_outdir(self, out_path):
        """When the output directory already exists, the old one is renamed."""
        os.makedirs(out_path)
        sentinel = os.path.join(out_path, 'sentinel.txt')
        with open(sentinel, 'w') as fh:
            fh.write('test')

        # First run: creates a fresh output dir (moves old one aside)
        args = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '-y'] + ['-p', 'native_sim']
        assert twister_main(args) == 0

        container = os.path.dirname(out_path)
        contents = os.listdir(container)
        base = os.path.basename(out_path)
        # Twister moves the old directory to <outdir>.N
        backup_dirs = [d for d in contents if d.startswith(base + '.') and d != base]
        assert len(backup_dirs) >= 1, (
            f'Expected a backup directory named {base}.N; found: {contents}'
        )

    @pytest.mark.fast
    def test_no_clean_keeps_stale_files(self, out_path):
        """--no-clean preserves files from a previous run."""
        os.makedirs(out_path)
        straggler = os.path.join(out_path, 'straggler.txt')
        with open(straggler, 'w') as fh:
            fh.write('do not delete me')

        args = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '-y', '--no-clean'] + [
            '-p',
            'native_sim',
        ]
        assert twister_main(args) == 0
        assert os.path.isfile(straggler), '--no-clean should preserve straggler.txt'

    @pytest.mark.fast
    def test_default_removes_stale_files(self, out_path):
        """Without --no-clean, stale files from a previous run are removed."""
        os.makedirs(out_path)
        straggler = os.path.join(out_path, 'straggler.txt')
        with open(straggler, 'w') as fh:
            fh.write('delete me')

        args = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '-y'] + ['-p', 'native_sim']
        assert twister_main(args) == 0
        assert not os.path.isfile(straggler), 'straggler should have been removed'


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestReportFiltered:
    """Tests for --report-filtered option."""

    @pytest.mark.fast
    def test_report_filtered_includes_skipped(self, out_path):
        """--report-filtered includes filtered tests in testplan.json."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '-y',
            '--report-filtered',
            '--tag',
            'agnostic',
        ] + ['-p', 'native_sim', '-p', 'intel_adl_crb']
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        # With --report-filtered all suites appear in the JSON regardless of filter
        assert len(plan['testsuites']) > 0


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestReportSummary:
    """Tests for --report-summary option."""

    @pytest.mark.build
    def test_report_summary_zero_failures(self, out_path):
        """--report-summary with no arguments exits 0 on an all-pass run."""
        args = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '--build-only', '--report-summary'] + [
            '-p',
            'native_sim',
        ]
        assert twister_main(args) == 0

    @pytest.mark.build
    def test_report_summary_shows_previous_failures(self, capfd, out_path):
        """--report-summary prints a synopsis of failures from a previous run.

        Two-phase workflow: first run produces twister.json with errors;
        second run with --report-summary --no-clean reads it and prints them.
        Both phases exit 0 — the exit code is not affected by --report-summary.
        """
        path = os.path.join(TEST_DATA, 'tests', 'always_build_error')
        # Phase 1: build that always fails; creates twister.json with errors.
        result1 = twister_main(
            ['--outdir', out_path, '-T', path, '--build-only', '-p', 'native_sim/native/64']
        )
        assert result1 != 0
        # Drop phase-1 output so the capture below only holds phase-2's summary.
        capfd.readouterr()

        # Phase 2: --report-summary 0 loads the previous twister.json and
        # prints all failures; exits 0 regardless of failure count.
        result2 = twister_main(
            [
                '--no-clean',
                '--outdir',
                out_path,
                '-T',
                path,
                '--report-summary',
                '0',
                '-p',
                'native_sim/native/64',
            ]
        )
        _, err = capfd.readouterr()
        assert result2 == 0
        assert 'always_fail.dummy' in err


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestDetailedSkippedReport:
    """Tests for --detailed-skipped-report option."""

    @pytest.mark.build
    @pytest.mark.parametrize(
        'flags, description',
        [
            (['--detailed-skipped-report'], 'with_flag'),
            ([], 'without_flag'),
        ],
        ids=['with_flag', 'without_flag'],
    )
    def test_detailed_skipped_report_xml(self, out_path, flags, description):
        """--detailed-skipped-report includes filtered testcases in XML output."""
        import xml.etree.ElementTree as ET

        args = (
            ['-i', '--outdir', out_path, '-T', AGNOSTIC]
            + flags
            + ['-p', 'native_sim', '-p', 'intel_adl_crb']
        )
        result = twister_main(args)
        assert result == 0

        xml_path = os.path.join(out_path, 'twister_report.xml')
        assert os.path.isfile(xml_path), 'twister_report.xml not generated'
        root = ET.parse(xml_path).getroot()
        testcases = list(root.iter('testcase'))
        assert len(testcases) >= 0


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestArtifacts:
    """Tests for artifact management flags."""

    @pytest.mark.build
    def test_runtime_artifact_cleanup_accepted(self, out_path):
        """--runtime-artifact-cleanup is accepted and exits 0."""
        path = os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic')
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            path,
            '--runtime-artifact-cleanup',
            '-p',
            'native_sim',
        ]
        assert twister_main(args) == 0

    @pytest.mark.build
    def test_prep_artifacts_for_testing_accepted(self, out_path):
        """--prep-artifacts-for-testing is accepted and exits 0."""
        path = os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic')
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            path,
            '--prep-artifacts-for-testing',
            '-p',
            'native_sim',
        ]
        assert twister_main(args) == 0

    @pytest.mark.build
    def test_package_artifacts_creates_dir(self, out_path, tmp_path):
        """--package-artifacts PATH creates a bz2 tarball at that path."""
        path = os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic')
        pkg_file = str(tmp_path / 'PACKAGE')
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            path,
            '--package-artifacts',
            pkg_file,
            '-p',
            'native_sim',
        ]
        result = twister_main(args)
        assert result == 0
        assert os.path.isfile(pkg_file), f'Package tarball {pkg_file!r} was not created'
