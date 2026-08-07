#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Blackbox tests for twister platform management options.

Covered options:
  -p / --platform           Restrict test run to named platforms
  -P / --exclude-platform   Remove platforms from the run
  --board-root (-A)         Add an extra board root directory
  --platform-pattern        Regex-based platform selection
  Default platform selection (when no -p is given)
"""

import os
from unittest import mock

import pytest
from conftest import TEST_DATA, TEST_FILENAME_MOCK, active_testcases, read_testplan
from twisterlib.testplan import TestPlan
from twisterlib.twister_main import main as twister_main

DUMMY = os.path.join(TEST_DATA, 'tests', 'dummy')
AGNOSTIC = os.path.join(DUMMY, 'agnostic')
PLATFORMS = ['qemu_x86', 'intel_adl_crb']


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestPlatformSelection:
    """Tests for -p / --platform option."""

    @pytest.mark.fast
    def test_single_platform(self, out_path):
        """-p limits the run to the specified platform."""
        args = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '-y', '-p', 'qemu_x86']
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        platforms = {ts['platform'] for ts in plan['testsuites']}
        assert all(p.startswith('qemu_x86') for p in platforms)

    @pytest.mark.fast
    def test_multiple_platforms(self, out_path):
        """-p may be repeated to add multiple platforms."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            DUMMY,
            '-y',
            '-p',
            'qemu_x86',
            '-p',
            'intel_adl_crb',
        ]
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        platforms = {ts['platform'] for ts in plan['testsuites']}
        assert any('qemu_x86' in p for p in platforms)
        assert any('intel_adl_crb' in p for p in platforms)

    @pytest.mark.fast
    def test_platform_with_variant(self, out_path):
        """A platform may be specified with its variant (board/soc) qualifier."""
        args = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '-y', '-p', 'qemu_x86/atom']
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        platforms = {ts['platform'] for ts in plan['testsuites']}
        assert all('qemu_x86' in p for p in platforms)

    @pytest.mark.fast
    def test_platform_allow_respected(self, out_path):
        """platform_allow in testcase.yaml restricts which platforms are used."""
        # device/group has platform_allow: intel_adl_crb; qemu_x86 must not appear
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            DUMMY + '/device',
            '-y',
            '-p',
            'qemu_x86',
            '-p',
            'intel_adl_crb',
        ]
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        active = active_testcases(plan)
        # Only intel_adl_crb should have active cases for the device suite
        active_platforms = {p for p, _, _ in active}
        assert not any('qemu_x86' in p for p in active_platforms)
        assert any('intel_adl_crb' in p for p in active_platforms)


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestExcludePlatform:
    """Tests for -P / --exclude-platform option."""

    @pytest.mark.fast
    def test_exclude_single(self, out_path):
        """--exclude-platform removes the named platform from the run."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '-y',
            '-p',
            'qemu_x86',
            '-p',
            'qemu_x86_64',
            '-p',
            'native_sim',
            '--exclude-platform',
            'qemu_x86',
        ]
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        platforms = {ts['platform'] for ts in plan['testsuites']}
        assert not any(
            'qemu_x86/' in p or p == 'qemu_x86/atom' for p in platforms if 'qemu_x86_64' not in p
        )

    @pytest.mark.fast
    def test_exclude_all_leaves_empty(self, out_path):
        """Excluding all selected platforms results in an empty plan."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '-y',
            '-p',
            'qemu_x86',
            '--exclude-platform',
            'qemu_x86',
        ]
        twister_main(args)

        plan = read_testplan(out_path)
        active = active_testcases(plan)
        assert len(active) == 0


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestBoardRoot:
    """Tests for --board-root (-A) option."""

    @pytest.mark.fast
    def test_board_root_enables_custom_board(self, out_path):
        """--board-root makes a custom board visible to the test plan."""
        board_root = os.path.join(TEST_DATA, 'boards')
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            DUMMY,
            '-y',
            '--board-root',
            board_root,
            '-p',
            'qemu_x86',
            '-p',
            'dummy/unit_testing',
        ]
        assert twister_main(args) == 0

    @pytest.mark.fast
    def test_missing_board_without_root_errors(self, out_path, capsys):
        """Referencing a custom board without --board-root returns exit code 2."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            DUMMY,
            '-y',
            '-p',
            'qemu_x86',
            '-p',
            'dummy/unit_testing',
        ]
        result = twister_main(args)
        assert result == 2


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestAnyPlatform:
    """Tests for --all / -l (ignore platform constraints)."""

    @pytest.mark.fast
    @pytest.mark.parametrize('flag', ['--all', '-l'], ids=['long', 'short'])
    def test_any_platform_flag_accepted(self, out_path, flag):
        """--all / -l causes twister to ignore platform_allow constraints."""
        args = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '-y', flag] + [
            v for p in PLATFORMS for v in ('-p', p)
        ]
        assert twister_main(args) == 0

    @pytest.mark.fast
    def test_all_overrides_platform_allow(self, out_path):
        """With --all, even device/ suites appear on non-allowed platforms."""
        args = ['-i', '--outdir', out_path, '-T', DUMMY, '-y', '--all', '-p', 'qemu_x86']
        assert twister_main(args) == 0
        plan = read_testplan(out_path)
        # --all: device/group should appear on qemu_x86 even though its
        # platform_allow is intel_adl_crb
        names = {ts['name'] for ts in plan['testsuites']}
        assert 'dummy.device.group' in names


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestEmulationOnly:
    """Tests for --emulation-only flag."""

    @pytest.mark.fast
    def test_emulation_only_selects_only_emulated_platforms(self, out_path):
        """--emulation-only restricts runs to platforms with a simulator."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '-y',
            '--emulation-only',
            '-p',
            'qemu_x86',
            '-p',
            'intel_adl_crb',
        ]
        assert twister_main(args) == 0
        plan = read_testplan(out_path)
        # Only qemu_x86 (emulated) should appear; intel_adl_crb (HW) should not
        platforms = {ts['platform'] for ts in plan['testsuites']}
        assert any('qemu_x86' in p for p in platforms)
        assert not any('intel_adl_crb' in p for p in platforms), (
            f'Expected intel_adl_crb to be filtered out; got {platforms!r}'
        )


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestForcePlatform:
    """Tests for --force-platform flag."""

    @pytest.mark.fast
    def test_force_platform_ignores_platform_allow(self, out_path):
        """--force-platform runs tests on all listed platforms regardless of platform_allow."""
        args = ['-i', '--outdir', out_path, '-T', DUMMY, '-y', '--force-platform'] + [
            v for p in PLATFORMS for v in ('-p', p)
        ]
        assert twister_main(args) == 0
        plan = read_testplan(out_path)
        cases = active_testcases(plan)
        # Both qemu_x86 and intel_adl_crb should appear for all suites
        platforms_used = {plat for plat, _, _ in cases}
        assert any('qemu_x86' in p for p in platforms_used)
        assert any('intel_adl_crb' in p for p in platforms_used)


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestDefaultPlatforms:
    """Tests for default platform selection behaviour."""

    @pytest.mark.fast
    def test_default_platforms_selected(self, out_path):
        """When no -p is given, platforms marked default in their YAML are used."""
        args = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '-y']
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        # Some platforms must have been selected
        assert len(plan['testsuites']) > 0


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestPlatformStats:
    """Tests for per-platform statistics in testplan.json."""

    @pytest.mark.fast
    @pytest.mark.parametrize(
        'test_path, platforms, expected_suites, expected_active_cases',
        [
            (
                os.path.join(TEST_DATA, 'tests', 'dummy', 'agnostic'),
                ['qemu_x86', 'qemu_x86_64', 'intel_adl_crb'],
                # platform_allow in each suite limits them to at most 2 of the 3 platforms:
                # subgroup1/subgroup2/group2 each allow native_sim|qemu_x86|qemu_x86_64 => 2 hit
                6,  # 3 suites * 2 matching platforms (intel_adl_crb excluded by platform_allow)
                12,  # 6 testsuites * 2 test cases each
            ),
            (
                os.path.join(TEST_DATA, 'tests', 'dummy', 'device'),
                ['qemu_x86', 'qemu_x86_64', 'intel_adl_crb'],
                # platform_allow: intel_adl_crb => only intel_adl_crb matches
                1,  # 1 suite * 1 matching platform
                1,  # intel_adl_crb combo is active
            ),
        ],
        ids=['agnostic', 'device'],
    )
    def test_plan_stats(
        self, out_path, test_path, platforms, expected_suites, expected_active_cases
    ):
        """testplan.json testsuite and active case counts match expectations."""
        args = ['-i', '--outdir', out_path, '-T', test_path, '-y'] + [
            v for p in platforms for v in ('-p', p)
        ]
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        assert len(plan['testsuites']) == expected_suites, (
            f'Expected {expected_suites} testsuites in plan'
        )
        cases = active_testcases(plan)
        assert len(cases) == expected_active_cases, (
            f'Expected {expected_active_cases} active test cases'
        )
