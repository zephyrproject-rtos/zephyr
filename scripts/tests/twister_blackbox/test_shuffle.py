#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Blackbox tests for twister shuffle and subset options.

Covered options:
  --shuffle-tests                  Randomise test order
  --shuffle-tests-seed SEED        Fix the shuffle seed
  --subset N/M                     Run only the Nth of M equal parts
  --seed SEED                      Native-sim random seed (passed to the binary)
"""

import os
from unittest import mock

import pytest
from conftest import TEST_DATA, TEST_FILENAME_MOCK, read_testplan
from twisterlib.testplan import TestPlan
from twisterlib.twister_main import main as twister_main

DUMMY = os.path.join(TEST_DATA, 'tests', 'dummy')
PLATFORMS = ['native_sim', 'intel_adl_crb']


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestShuffleTests:
    """Tests for --shuffle-tests and --shuffle-tests-seed options."""

    @pytest.mark.fast
    def test_shuffle_changes_order(self, out_path, tmp_path):
        """--shuffle-tests with different seeds produces different orderings."""

        # --shuffle-tests requires --subset; use 1/1 to get all tests
        def _order(seed, path):
            args = [
                '-i',
                '--outdir',
                path,
                '-T',
                DUMMY,
                '-y',
                '--shuffle-tests',
                '--shuffle-tests-seed',
                str(seed),
                '--subset',
                '1/1',
            ] + [v for p in PLATFORMS for v in ('-p', p)]
            twister_main(args)
            plan = read_testplan(path)
            return [ts['name'] for ts in plan['testsuites']]

        order_123 = _order(123, str(tmp_path / 'seed123'))
        order_321 = _order(321, str(tmp_path / 'seed321'))
        # Different seeds should usually produce different orderings
        assert order_123 != order_321

    @pytest.mark.fast
    def test_shuffle_same_seed_reproducible(self, out_path, tmp_path):
        """The same seed always produces the same ordering."""

        # --shuffle-tests requires --subset; use 1/1 to get all tests
        def _order(path):
            args = [
                '-i',
                '--outdir',
                path,
                '-T',
                DUMMY,
                '-y',
                '--shuffle-tests',
                '--shuffle-tests-seed',
                '42',
                '--subset',
                '1/1',
            ] + [v for p in PLATFORMS for v in ('-p', p)]
            twister_main(args)
            return [ts['name'] for ts in read_testplan(path)['testsuites']]

        order_a = _order(str(tmp_path / 'run_a'))
        order_b = _order(str(tmp_path / 'run_b'))
        assert order_a == order_b

    @pytest.mark.fast
    @pytest.mark.parametrize(
        'seed, subset, expected_names',
        [
            ('123', '1/2', ['dummy.agnostic.group1.subgroup2', 'dummy.device.group']),
            ('123', '2/2', ['dummy.agnostic.group2', 'dummy.agnostic.group1.subgroup1']),
            ('321', '1/2', ['dummy.agnostic.group1.subgroup2', 'dummy.agnostic.group1.subgroup1']),
            ('321', '2/2', ['dummy.agnostic_cpp.group2', 'dummy.device.group']),
            ('123', '1/3', ['dummy.agnostic.group1.subgroup2', 'dummy.device.group']),
            ('123', '2/3', ['dummy.agnostic.group2']),
            ('123', '3/3', ['dummy.agnostic.group1.subgroup1']),
            ('321', '1/3', ['dummy.agnostic.group1.subgroup1', 'dummy.agnostic.group1.subgroup2']),
            ('321', '2/3', ['dummy.device.group']),
            ('321', '3/3', ['dummy.agnostic_cpp.group2']),
        ],
        ids=[
            'seed123-half1',
            'seed123-half2',
            'seed321-half1',
            'seed321-half2',
            'seed123-third1',
            'seed123-third2',
            'seed123-third3',
            'seed321-third1',
            'seed321-third2',
            'seed321-third3',
        ],
    )
    def test_subset_with_seed(self, out_path, seed, subset, expected_names):
        """--shuffle-tests + --shuffle-tests-seed + --subset yields deterministic subsets."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            DUMMY,
            '-y',
            '--shuffle-tests',
            '--shuffle-tests-seed',
            seed,
            '--subset',
            subset,
        ] + [v for p in PLATFORMS for v in ('-p', p)]
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        names = [ts['name'] for ts in plan['testsuites']]
        for name in expected_names:
            assert name in names, f'{name!r} not in subset {subset} (seed={seed})'

    @pytest.mark.fast
    def test_shuffle_preserves_all_suites(self, out_path, tmp_path):
        """Shuffling does not add or remove test suites from the plan."""
        args_plain = ['-i', '--outdir', str(tmp_path / 'plain'), '-T', DUMMY, '-y'] + [
            v for p in PLATFORMS for v in ('-p', p)
        ]
        twister_main(args_plain)
        plain_names = {ts['name'] for ts in read_testplan(str(tmp_path / 'plain'))['testsuites']}

        # --shuffle-tests requires --subset; use 1/1 to get all tests
        args_shuffled = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            DUMMY,
            '-y',
            '--shuffle-tests',
            '--shuffle-tests-seed',
            '999',
            '--subset',
            '1/1',
        ] + [v for p in PLATFORMS for v in ('-p', p)]
        twister_main(args_shuffled)
        shuffled_names = {ts['name'] for ts in read_testplan(out_path)['testsuites']}

        assert plain_names == shuffled_names


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestSubsetCoverage:
    """Tests that subsets together cover the full test plan."""

    @pytest.mark.fast
    @pytest.mark.parametrize('parts', [2, 3, 4], ids=['halves', 'thirds', 'quarters'])
    def test_all_parts_cover_full_plan(self, out_path, tmp_path, parts):
        """The union of all N parts equals the unrestricted test plan."""
        all_names: set = set()
        for i in range(1, parts + 1):
            subset = f'{i}/{parts}'
            part_path = str(tmp_path / f'part_{i}')
            args = [
                '-i',
                '--outdir',
                part_path,
                '-T',
                DUMMY,
                '-y',
                '--shuffle-tests',
                '--shuffle-tests-seed',
                '7',
                '--subset',
                subset,
            ] + [v for p in PLATFORMS for v in ('-p', p)]
            assert twister_main(args) == 0
            plan = read_testplan(part_path)
            all_names.update(ts['name'] for ts in plan['testsuites'])

        # Full plan
        full_args = ['-i', '--outdir', out_path, '-T', DUMMY, '-y'] + [
            v for p in PLATFORMS for v in ('-p', p)
        ]
        twister_main(full_args)
        full_names = {ts['name'] for ts in read_testplan(out_path)['testsuites']}
        assert all_names == full_names
