#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Blackbox tests for twister configuration-file options.

Covered options:
  --alt-config-root       Override testcase.yaml with alternative configs
  --test-config           Specify a platform / level configuration file
  --level                 Select a named test level from test-config
  --disable-suite-name-check    Skip verification of suite names at runtime
  --disable-warnings-as-errors  Treat compiler warnings as non-fatal
"""

import os
import re
from unittest import mock

import pytest
from conftest import TEST_DATA, TEST_FILENAME_MOCK, active_testcases, read_testplan
from twisterlib.testplan import TestPlan
from twisterlib.twister_main import main as twister_main

DUMMY = os.path.join(TEST_DATA, 'tests', 'dummy')
PLATFORMS = ['native_sim', 'intel_adl_crb']
TEST_CONFIG = os.path.join(TEST_DATA, 'test_config.yaml')


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestAltConfigRoot:
    """Tests for --alt-config-root option."""

    @pytest.mark.fast
    def test_alt_config_overrides_tag(self, out_path):
        """--alt-config-root provides alternative testcase definitions.

        The alt configs in test_data/alt-test-configs/dummy add the tag
        'alternate-config-root'; filtering on that tag should produce results
        only when the alt-config-root is active.
        """
        alt_root = os.path.join(TEST_DATA, 'alt-test-configs', 'dummy')

        # Without alt-config-root, the tag is unknown -> no results
        args_without = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            DUMMY,
            '-y',
            '--tag',
            'alternate-config-root',
        ] + [v for p in PLATFORMS for v in ('-p', p)]
        assert twister_main(args_without) == 0
        plan_without = read_testplan(out_path)
        assert len(active_testcases(plan_without)) == 0

        # With alt-config-root, the overridden YAML carries the tag
        args_with = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            DUMMY,
            '-y',
            '--alt-config-root',
            alt_root,
            '--tag',
            'alternate-config-root',
        ] + [v for p in PLATFORMS for v in ('-p', p)]
        assert twister_main(args_with) == 0
        plan_with = read_testplan(out_path)
        assert len(active_testcases(plan_with)) == 4

    @pytest.mark.fast
    def test_alt_config_does_not_affect_other_suites(self, out_path):
        """--alt-config-root only overrides matching suites, not all suites."""
        alt_root = os.path.join(TEST_DATA, 'alt-test-configs', 'dummy')
        args = ['-i', '--outdir', out_path, '-T', DUMMY, '-y', '--alt-config-root', alt_root] + [
            v for p in PLATFORMS for v in ('-p', p)
        ]
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        names = {ts['name'] for ts in plan['testsuites']}
        # Standard suites that were NOT overridden should still be present
        assert 'dummy.agnostic.group1.subgroup1' in names

    @pytest.mark.fast
    @pytest.mark.parametrize('reverse_alt_roots', [False, True], ids=['in_order', 'reversed'])
    def test_alt_config_root_matched_per_suite_root(self, out_path, reverse_alt_roots):
        """Each --alt-config-root applies only to the -T suite root with the same basename."""
        test_root_1 = os.path.join(TEST_DATA, 'tests', 'alt_config_root', 'group1')
        test_root_2 = os.path.join(TEST_DATA, 'tests', 'alt_config_root', 'group2')
        alt_root_1 = os.path.join(TEST_DATA, 'alt-test-configs', 'alt_config_root', 'group1')
        alt_root_2 = os.path.join(TEST_DATA, 'alt-test-configs', 'alt_config_root', 'group2')
        alt_roots = [alt_root_2, alt_root_1] if reverse_alt_roots else [alt_root_1, alt_root_2]
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            test_root_1,
            '-T',
            test_root_2,
            '-y',
        ]
        args += [v for root in alt_roots for v in ('--alt-config-root', root)]
        args += [v for p in PLATFORMS for v in ('-p', p)]
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        paths = {}
        for ts in plan['testsuites']:
            paths.setdefault(ts['name'], set()).add(os.path.normpath(ts['path']))

        # Each alt config is loaded for the suite root with the same basename only
        for group in ('group1', 'group2'):
            name = f'alt_config_root.{group}.alt'
            assert name in paths
            assert len(paths[name]) == 1
            assert next(iter(paths[name])).endswith(os.path.join('alt_config_root', group))
            # The standard config of that root was replaced, not loaded in addition
            assert f'alt_config_root.{group}' not in paths


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestLevel:
    """Tests for --level and --test-config options."""

    @pytest.mark.fast
    @pytest.mark.parametrize(
        'level, expected_count',
        [
            ('smoke', 6),
            ('acceptance', 7),
        ],
        ids=['smoke', 'acceptance'],
    )
    def test_level_selects_correct_suites(self, out_path, level, expected_count):
        """--level combined with --test-config selects the correct test cases."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            DUMMY,
            '-y',
            '--level',
            level,
            '--test-config',
            TEST_CONFIG,
        ] + [v for p in PLATFORMS for v in ('-p', p)]
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        cases = active_testcases(plan)
        assert len(cases) == expected_count, (
            f'Level {level!r} expected {expected_count} cases, got {len(cases)}'
        )

    @pytest.mark.fast
    def test_smoke_subset_of_acceptance(self, out_path, tmp_path):
        """The smoke level is a subset of the acceptance level."""

        def _cases_for_level(level, path):
            args = [
                '-i',
                '--outdir',
                path,
                '-T',
                DUMMY,
                '-y',
                '--level',
                level,
                '--test-config',
                TEST_CONFIG,
            ] + [v for p in PLATFORMS for v in ('-p', p)]
            twister_main(args)
            plan = read_testplan(path)
            return {tc for _, _, tc in active_testcases(plan)}

        smoke_path = str(tmp_path / 'smoke')
        accept_path = str(tmp_path / 'accept')
        smoke_cases = _cases_for_level('smoke', smoke_path)
        accept_cases = _cases_for_level('acceptance', accept_path)
        assert smoke_cases.issubset(accept_cases)


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestDisableSuiteNameCheck:
    """Tests for --disable-suite-name-check option."""

    @pytest.mark.build
    def test_disable_suite_name_check_suppresses_warning(self, capfd, out_path):
        """--disable-suite-name-check suppresses suite-name mismatch output."""
        path = os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic')
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            path,
            '--disable-suite-name-check',
            '-vv',
            '-ll',
            'DEBUG',
        ] + ['-p', 'native_sim']
        assert twister_main(args) == 0

        _, err = capfd.readouterr()
        # The warning about mismatched suite names should not appear
        assert re.search(r'Expected suite names', err) is None
        assert re.search(r'Detected suite names', err) is None

    @pytest.mark.build
    def test_suite_name_check_reports_mismatch(self, capfd, out_path):
        """Without --disable-suite-name-check, suite-name mismatches are reported."""
        path = os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic')
        args = ['-i', '--outdir', out_path, '-T', path, '-vv', '-ll', 'DEBUG'] + [
            '-p',
            'native_sim',
        ]
        assert twister_main(args) == 0

        _, err = capfd.readouterr()
        # The expected/detected output should now be present
        assert re.search(r'Expected suite names', err) is not None


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestDisableWarningsAsErrors:
    """Tests for --disable-warnings-as-errors option."""

    @pytest.mark.build
    @pytest.mark.parametrize(
        'flag, expected_exit',
        [
            ('--disable-warnings-as-errors', 0),
            ('-v', 1),
        ],
        ids=['disabled', 'default_warnings_as_errors'],
    )
    def test_disable_warnings_as_errors(self, capfd, out_path, flag, expected_exit):
        """--disable-warnings-as-errors prevents a warning from causing failure."""
        path = os.path.join(TEST_DATA, 'tests', 'always_warning')
        args = ['-i', '--outdir', out_path, '-T', path, flag, '-vv', '--build-only'] + [
            '-p',
            'native_sim',
        ]
        result = twister_main(args)
        assert result == expected_exit
