#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Blackbox tests for twister test-selection options.

Covered options:
  -s / --test            Select a specific test suite by name or path
  --sub-test             Select individual test cases within a suite
  --list-tests           Print available test cases and exit
  --list-tags            Print available tags and exit
  --test-tree            Print a tree of available tests and exit
  --save-tests / --load-tests  Persist and reload a test list
  -B / --subset          Run a fraction of the overall test plan
  --size ELF             Print ROM/RAM size report for a pre-built ELF
"""

import json
import os
import re
from unittest import mock

import pytest
from conftest import TEST_DATA, TEST_FILENAME_MOCK, active_testcases, read_testplan
from twisterlib.error import TwisterRuntimeError
from twisterlib.testplan import TestPlan
from twisterlib.twister_main import main as twister_main

DUMMY = os.path.join(TEST_DATA, 'tests', 'dummy')
AGNOSTIC = os.path.join(DUMMY, 'agnostic')
PLATFORMS = ['native_sim', 'intel_adl_crb']


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestTestSelection:
    """Tests for -s / --test option."""

    @pytest.mark.fast
    def test_select_by_short_name(self, out_path):
        """-s selects a suite by its short (dotted) name."""
        # Do NOT use --detailed-test-id so names are short dotted form
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            DUMMY,
            '-y',
            '--test',
            'dummy.agnostic.group1.subgroup1',
        ] + [v for p in PLATFORMS for v in ('-p', p)]
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        names = {ts['name'] for ts in plan['testsuites']}
        assert 'dummy.agnostic.group1.subgroup1' in names
        assert 'dummy.device.group' not in names

    @pytest.mark.fast
    def test_select_by_path(self, out_path):
        """-s accepts a path form of the test identifier."""
        # The path form uses the ZEPHYR_BASE-relative path (with symlinks resolved)
        # because twister normalises the path when storing --detailed-test-id names.
        from conftest import ZEPHYR_BASE

        rel_test_data = os.path.relpath(TEST_DATA, ZEPHYR_BASE)
        test_id = os.path.join(
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
            '-T',
            DUMMY,
            '-y',
            '--test',
            test_id,
        ] + [v for p in PLATFORMS for v in ('-p', p)]
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        names = {ts['name'] for ts in plan['testsuites']}
        # With --detailed-test-id names are path-prefixed; check any name ends with the suite id
        assert any('dummy.agnostic.group1.subgroup1' in n for n in names)

    @pytest.mark.fast
    def test_select_nonexistent_suite_errors(self, out_path, capsys):
        """--test with a non-existent suite returns an error."""
        args = [
            '--detailed-test-id',
            '-i',
            '--outdir',
            out_path,
            '-y',
            '--test',
            'does.not.exist',
        ] + [v for p in PLATFORMS for v in ('-p', p)]
        result = twister_main(args)

        captured = capsys.readouterr()
        assert result != 0
        assert 'No testsuites found' in captured.err


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestSubtestSelection:
    """Tests for --sub-test option."""

    @pytest.mark.fast
    def test_valid_subtest(self, out_path):
        """--sub-test selects individual test cases within a suite."""
        args = [
            '--detailed-test-id',
            '-i',
            '--outdir',
            out_path,
            '-T',
            DUMMY,
            '--sub-test',
            'dummy.agnostic.group2.a2_tests.assert1',
            '-y',
        ] + [v for p in PLATFORMS for v in ('-p', p)]
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        cases = active_testcases(plan)
        assert len(cases) == 4  # 2 platforms * 2 (one per platform in allow list)

    @pytest.mark.fast
    def test_invalid_subtest_raises(self, out_path):
        """--sub-test with an invalid identifier raises TwisterRuntimeError."""
        invalid_id = os.path.join(
            'scripts',
            'tests',
            'twister_blackbox',
            'test_data',
            'tests',
            'dummy',
            'agnostic',
            'group1',
            'subgroup1',
            'dummy.agnostic.group2.assert1',
        )
        args = [
            '--detailed-test-id',
            '-i',
            '--outdir',
            out_path,
            '-T',
            DUMMY,
            '--sub-test',
            invalid_id,
            '-y',
        ] + [v for p in PLATFORMS for v in ('-p', p)]
        with pytest.raises(TwisterRuntimeError):
            twister_main(args)


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestListCommands:
    """Tests for --list-tests, --list-tags, and --test-tree output commands."""

    @pytest.mark.fast
    def test_list_tests(self, out_path, capsys):
        """--list-tests prints test case identifiers and exits."""
        args = ['--outdir', out_path, '-T', AGNOSTIC, '--list-tests', '--no-detailed-test-id']
        result = twister_main(args)

        out, _ = capsys.readouterr()
        assert result == 0
        assert 'dummy.agnostic.group1.subgroup1' in out
        assert 'dummy.agnostic.group2' in out

    @pytest.mark.fast
    @pytest.mark.parametrize(
        'path, expected_tags',
        [
            (AGNOSTIC, ['agnostic', 'subgrouped', 'even', 'odd']),
            (DUMMY + '/device', ['device']),
        ],
        ids=['agnostic_tags', 'device_tags'],
    )
    def test_list_tags(self, out_path, capsys, path, expected_tags):
        """--list-tags prints the tags used by the selected suites."""
        args = ['--outdir', out_path, '-T', path, '--list-tags']
        twister_main(args)

        out, _ = capsys.readouterr()
        for tag in expected_tags:
            assert tag in out, f'Expected tag {tag!r} in --list-tags output'

    @pytest.mark.fast
    def test_test_tree(self, out_path, capsys):
        """--test-tree prints a tree representation of available tests."""
        args = ['--outdir', out_path, '-T', AGNOSTIC, '--test-tree', '--no-detailed-test-id']
        twister_main(args)

        out, _ = capsys.readouterr()
        assert 'Testsuite' in out
        assert 'dummy' in out
        assert 'agnostic' in out


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestSaveLoadTests:
    """Tests for --save-tests and --load-tests options."""

    @pytest.mark.fast
    def test_save_and_load_tests(self, out_path, tmp_path):
        """--save-tests persists a test list; --load-tests reloads it."""
        saved = str(tmp_path / 'saved_tests.json')

        # Save only agnostic tests
        args_save = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '-y', '--save-tests', saved] + [
            v for p in PLATFORMS for v in ('-p', p)
        ]
        assert twister_main(args_save) == 0

        # Load into a broader test run
        args_load = ['-i', '--outdir', out_path, '-T', DUMMY, '-y', '--load-tests', saved] + [
            v for p in PLATFORMS for v in ('-p', p)
        ]
        assert twister_main(args_load) == 0

        plan = read_testplan(out_path)
        cases = active_testcases(plan)
        # Only the previously saved agnostic cases should be active
        assert len(cases) == 6
        names = {name for _, name, _ in cases}
        assert 'dummy.device.group' not in names

    @pytest.mark.fast
    def test_save_tests_creates_file(self, out_path, tmp_path):
        """--save-tests creates a JSON file with test identifiers."""
        saved = str(tmp_path / 'tests.json')

        args = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '-y', '--save-tests', saved] + [
            v for p in PLATFORMS for v in ('-p', p)
        ]
        assert twister_main(args) == 0
        assert os.path.isfile(saved)

        with open(saved) as fh:
            data = json.load(fh)
        assert len(data) > 0


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestSubset:
    """Tests for -B / --subset option."""

    @pytest.mark.fast
    @pytest.mark.parametrize(
        'subset, expected_names',
        [
            ('1/2', ['dummy.device.group', 'dummy.agnostic.group1.subgroup2']),
            ('2/2', ['dummy.agnostic.group2', 'dummy.agnostic.group1.subgroup1']),
            ('1/3', ['dummy.device.group', 'dummy.agnostic.group1.subgroup2']),
            ('2/3', ['dummy.agnostic.group2']),
            ('3/3', ['dummy.agnostic.group1.subgroup1']),
        ],
        ids=['first_half', 'second_half', 'first_third', 'mid_third', 'last_third'],
    )
    def test_subset_default_seed(self, out_path, subset, expected_names):
        """--subset divides the test plan deterministically with the default seed."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            DUMMY,
            '-y',
            '--shuffle-tests',
            '--shuffle-tests-seed',
            '123',
            '--subset',
            subset,
        ] + [v for p in PLATFORMS for v in ('-p', p)]
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        names = [ts['name'] for ts in plan['testsuites']]
        for expected in expected_names:
            assert expected in names, f'{expected} not in subset {subset}'

    @pytest.mark.fast
    def test_subset_all_parts_cover_full_plan(self, out_path, tmp_path):
        """The union of all subset parts equals the complete test plan."""
        all_names = set()
        for part in ('1/3', '2/3', '3/3'):
            sub_out = str(tmp_path / f'out_{part.replace("/", "_")}')
            args = [
                '-i',
                '--outdir',
                sub_out,
                '-T',
                DUMMY,
                '-y',
                '--shuffle-tests',
                '--shuffle-tests-seed',
                '42',
                '--subset',
                part,
            ] + [v for p in PLATFORMS for v in ('-p', p)]
            assert twister_main(args) == 0
            plan = read_testplan(sub_out)
            for ts in plan['testsuites']:
                all_names.add(ts['name'])

        full_args = ['-i', '--outdir', out_path, '-T', DUMMY, '-y'] + [
            v for p in PLATFORMS for v in ('-p', p)
        ]
        assert twister_main(full_args) == 0
        full_plan = read_testplan(out_path)
        full_names = {ts['name'] for ts in full_plan['testsuites']}
        assert all_names == full_names


HELLO_WORLD = os.path.join(TEST_DATA, 'samples', 'hello_world')


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestSizeCommand:
    """Tests for the --size ELF option (print ROM/RAM size report)."""

    @pytest.mark.build
    def test_size_prints_report(self, capsys, hello_world_elf):
        """--size prints a header line and a Totals line for a pre-built ELF.

        The ELF is built once per session via the ``hello_world_elf`` fixture,
        so subsequent runs of this test (or any other test in the same session
        that needs the same binary) do not incur a second build cost.
        """
        if hello_world_elf is None:
            pytest.skip('hello_world build failed; cannot test --size')

        capsys.readouterr()

        with pytest.raises(SystemExit) as exc_info:
            twister_main(['--size', hello_world_elf])
        assert exc_info.value.code == 0

        out, _ = capsys.readouterr()
        header_re = re.compile(r'SECTION NAME\s+VMA\s+LMA\s+SIZE\s+HEX SZ\s+TYPE')
        footer_re = re.compile(r'Totals:\s+\d+\s+bytes\s+\(ROM\),\s+\d+\s+bytes\s+\(RAM\)')
        assert header_re.search(out), 'No size-report header found in output'
        assert footer_re.search(out), 'No size-report Totals line found in output'
