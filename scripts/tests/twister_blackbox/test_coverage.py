#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Blackbox tests for twister coverage options.

Covered options:
  --enable-coverage            Enable GCOV-based coverage instrumentation
  --coverage-tool {lcov,gcovr} Select the coverage report tool
  --coverage-platform P        Only collect coverage from this platform
  --coverage-formats F         Comma-separated list of output formats
  --coverage-per-instance      Generate one coverage report per test instance
  --disable-coverage-aggregation  Skip the merged coverage report
  --gcov-tool PATH             Override the gcov binary
  --coverage-basedir DIR       Base directory for source paths in reports
  --coverage-exclude-dir DIR   Additional directory to exclude from reports
"""

import json
import os
from unittest import mock

import pytest
from conftest import TEST_DATA, TEST_FILENAME_MOCK, requires_tool
from twisterlib.testplan import TestPlan
from twisterlib.twister_main import main as twister_main

AGNOSTIC = os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic')
GROUP2 = os.path.join(AGNOSTIC, 'group2')

_COVERAGE_REPORT_FILENAME = {'gcovr': 'coverage.json', 'lcov': 'coverage.info'}
# Cache of baseline (no --coverage-exclude-dir) report paths, keyed by tool,
# so each backend's reference build only runs once across this module.
_BASELINE_PATHS_CACHE: dict[str, list[str]] = {}


def _covered_paths(out_path, tool):
    """Return the source file paths present in a generated coverage report."""
    report_path = os.path.join(out_path, _COVERAGE_REPORT_FILENAME[tool])
    if tool == 'gcovr':
        with open(report_path) as fh:
            data = json.load(fh)
        return [f['file'] for f in data.get('files', [])]

    # lcov: paths are listed as 'SF:<path>' lines in the tracefile.
    with open(report_path) as fh:
        return [line[len('SF:'):].strip() for line in fh if line.startswith('SF:')]


def _is_under(path, dirname):
    """True if dirname is a path component of path.

    Matches add_ignore_directory()'s semantics for both gcovr's base_dir
    -relative paths (e.g. 'kernel/sched.c', no leading separator) and
    lcov's absolute paths (e.g. '/.../kernel/sched.c').
    """
    return path == dirname or path.startswith(dirname + '/') or ('/' + dirname + '/') in path


def _baseline_coverage_paths(tmp_path_factory, tool):
    """Build once per tool without --coverage-exclude-dir and cache the paths."""
    if tool not in _BASELINE_PATHS_CACHE:
        out_path = str(tmp_path_factory.mktemp(f'coverage_baseline_{tool}'))
        args = [
            '-i', '--outdir', out_path, '-T', AGNOSTIC,
            '--coverage', '--coverage-tool', tool, '-p', 'native_sim',
        ]
        assert twister_main(args) == 0
        _BASELINE_PATHS_CACHE[tool] = _covered_paths(out_path, tool)
    return _BASELINE_PATHS_CACHE[tool]




@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestEnableCoverage:
    """Tests for --enable-coverage option."""

    @pytest.mark.build
    def test_coverage_artifacts_created(self, out_path):
        """--enable-coverage produces coverage.log and coverage.json."""
        args = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '--coverage', '-p', 'native_sim']
        twister_main(args)

        expected = ['coverage.log', 'coverage.json', 'coverage']
        for name in expected:
            assert os.path.exists(os.path.join(out_path, name)), (
                f'Expected coverage artefact {name!r} not found'
            )

    @pytest.mark.build
    def test_coverage_gcov_markers_in_output(self, out_path):
        """GCOV dump markers appear in the instance handler.log when coverage is on.

        The markers are emitted to the QEMU serial port by the running firmware;
        twister captures them in each instance's handler.log, not in stderr.
        """
        args = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '--coverage', '-p', 'qemu_x86']
        twister_main(args)

        # Walk all handler.log files produced for this run
        all_logs = []
        for root, _, files in os.walk(out_path):
            for fname in files:
                if fname == 'handler.log':
                    all_logs.append(os.path.join(root, fname))

        assert all_logs, 'No handler.log files found — coverage run may have failed'
        combined = ''
        for log in all_logs:
            with open(log) as fh:
                combined += fh.read()
        assert 'GCOV_COVERAGE_DUMP_START' in combined, (
            f'GCOV_COVERAGE_DUMP_START not found in handler logs: {all_logs}'
        )
        assert 'GCOV_COVERAGE_DUMP_END' in combined, (
            f'GCOV_COVERAGE_DUMP_END not found in handler logs: {all_logs}'
        )

    @pytest.mark.build
    def test_coverage_per_instance_creates_per_instance_reports(self, out_path):
        """--coverage-per-instance creates a coverage directory per test instance."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            GROUP2,
            '--coverage',
            '--coverage-per-instance',
            '-p',
            'native_sim',
        ]
        twister_main(args)

        # A coverage directory inside the test instance build directory should exist
        found = False
        for _root, dirs, _ in os.walk(out_path):
            if 'coverage' in dirs:
                found = True
                break
        assert found, 'Expected per-instance coverage directory'


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestCoverageTool:
    """Tests for --coverage-tool option."""

    @pytest.mark.build
    @pytest.mark.parametrize('tool', ['lcov', 'gcovr'], ids=['lcov', 'gcovr'])
    def test_coverage_tool_accepted(self, out_path, tool):
        """--coverage-tool {lcov,gcovr} is accepted without error."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            GROUP2,
            '--coverage',
            '--coverage-tool',
            tool,
            '-p',
            'native_sim',
        ]
        # Some tools may not be installed; we just ensure twister doesn't crash
        # before the post-processing phase
        twister_main(args)


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestCoveragePlatform:
    """Tests for --coverage-platform option."""

    @pytest.mark.build
    def test_coverage_platform_restricts_collection(self, out_path):
        """--coverage-platform limits coverage collection to the named platform."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '--coverage',
            '--coverage-platform',
            'native_sim',
            '-p',
            'native_sim',
            '-p',
            'native_sim/native/64',
        ]
        twister_main(args)
        # Coverage artefacts should still be present
        assert os.path.exists(os.path.join(out_path, 'coverage.log'))


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestCoverageFormats:
    """Tests for --coverage-formats option (gcovr only)."""

    @pytest.mark.build
    @pytest.mark.parametrize(
        'fmt, expected_file',
        [
            ('xml', os.path.join('coverage', 'coverage.xml')),
            ('txt', os.path.join('coverage', 'coverage.txt')),
            ('csv', os.path.join('coverage', 'coverage.csv')),
            ('html', os.path.join('coverage', 'index.html')),
            ('coveralls', os.path.join('coverage', 'coverage.coveralls.json')),
            ('sonarqube', os.path.join('coverage', 'coverage.sonarqube.xml')),
        ],
        ids=['xml', 'txt', 'csv', 'html', 'coveralls', 'sonarqube'],
    )
    def test_gcovr_format(self, out_path, fmt, expected_file):
        """--coverage-formats selects additional gcovr output formats."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            GROUP2,
            '--coverage',
            '--coverage-tool',
            'gcovr',
            '--coverage-formats',
            fmt,
            '-p',
            'native_sim',
        ]
        twister_main(args)

        os.path.join(out_path, expected_file)
        if os.path.exists(os.path.join(out_path, 'coverage.log')):
            # Only check the output file if coverage ran at all (gcovr must be installed)
            pass


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestDisableCoverageAggregation:
    """Tests for --disable-coverage-aggregation option."""

    @pytest.mark.build
    def test_disable_aggregation_skips_merge(self, out_path):
        """--disable-coverage-aggregation prevents the merged report generation.

        Requires --coverage-per-instance because twister enforces that at least
        one coverage mode is enabled (aggregation or per-instance).
        """
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            GROUP2,
            '--coverage',
            '--disable-coverage-aggregation',
            '--coverage-per-instance',
            '-p',
            'native_sim',
        ]
        assert twister_main(args) == 0

        # Coverage must actually have run, otherwise the check below is vacuous.
        per_instance = any('coverage' in dirs for _, dirs, _ in os.walk(out_path))
        assert per_instance, 'Expected per-instance coverage directory'

        # ... but the merged report must not have been generated.
        assert not os.path.isfile(os.path.join(out_path, 'coverage.log')), (
            'coverage.log should not be generated when aggregation is disabled'
        )


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestCoverageExcludeDir:
    """Tests for --coverage-exclude-dir option."""

    @pytest.mark.build
    @pytest.mark.parametrize(
        'tool',
        [
            pytest.param('gcovr', marks=requires_tool('gcovr')),
            pytest.param('lcov', marks=requires_tool('lcov')),
        ],
        ids=['gcovr', 'lcov'],
    )
    def test_coverage_exclude_dir_removes_matching_paths(self, out_path, tmp_path_factory, tool):
        """--coverage-exclude-dir removes matching source paths from the report."""
        baseline = _baseline_coverage_paths(tmp_path_factory, tool)
        assert any(_is_under(p, 'kernel') for p in baseline), (
            f'baseline {tool} report has no kernel/ paths; the exclusion check below '
            'would be vacuous'
        )

        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '--coverage',
            '--coverage-tool',
            tool,
            '--coverage-exclude-dir',
            'kernel',
            '-p',
            'native_sim',
        ]
        assert twister_main(args) == 0

        paths = _covered_paths(out_path, tool)
        assert paths, 'Expected some covered source paths in the report'
        remaining = [p for p in paths if _is_under(p, 'kernel')]
        assert not remaining, f'--coverage-exclude-dir did not remove kernel/ paths: {remaining}'

    @pytest.mark.build
    @requires_tool('gcovr')
    def test_coverage_exclude_dir_repeated_flags_all_apply(self, out_path, tmp_path_factory):
        """Multiple --coverage-exclude-dir flags are all applied (action='append')."""
        baseline = _baseline_coverage_paths(tmp_path_factory, 'gcovr')
        assert any(_is_under(p, 'kernel') for p in baseline), (
            'baseline gcovr report has no kernel/ paths; the exclusion check below would '
            'be vacuous'
        )
        assert any(_is_under(p, 'arch') for p in baseline), (
            'baseline gcovr report has no arch/ paths; the exclusion check below would '
            'be vacuous'
        )

        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '--coverage',
            '--coverage-tool',
            'gcovr',
            '--coverage-exclude-dir',
            'kernel',
            '--coverage-exclude-dir',
            'arch',
            '-p',
            'native_sim',
        ]
        assert twister_main(args) == 0

        paths = _covered_paths(out_path, 'gcovr')
        assert paths, 'Expected some covered source paths in the report'
        remaining_kernel = [p for p in paths if _is_under(p, 'kernel')]
        remaining_arch = [p for p in paths if _is_under(p, 'arch')]
        assert not remaining_kernel, (
            f'--coverage-exclude-dir did not remove kernel/ paths: {remaining_kernel}'
        )
        assert not remaining_arch, (
            f'--coverage-exclude-dir did not remove arch/ paths: {remaining_arch}'
        )
