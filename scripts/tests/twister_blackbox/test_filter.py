#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Blackbox tests for twister filtering options.

All tests in this module use --dry-run (-y) so no build is required.
They verify that the correct tests are included or excluded by inspecting
testplan.json.

Test data layout (dummy suite); all agnostic platforms are
native_sim, qemu_x86, qemu_x86_64:
  agnostic/group1/subgroup1  tags: agnostic, subgrouped, odd
  agnostic/group1/subgroup2  tags: agnostic, subgrouped, even
  agnostic/group2            tags: agnostic, even
  device/group               tags: device   platforms: intel_adl_crb
"""

import os
from unittest import mock

import pytest
from conftest import TEST_DATA, TEST_FILENAME_MOCK, active_testcases, read_testplan
from twisterlib.testplan import TestPlan
from twisterlib.twister_main import main as twister_main

DUMMY = os.path.join(TEST_DATA, 'tests', 'dummy')
AGNOSTIC = os.path.join(DUMMY, 'agnostic')
DEVICE = os.path.join(DUMMY, 'device')
ALT_CONFIG_ROOT = os.path.join(TEST_DATA, 'alt-test-configs', 'dummy', 'agnostic')

# Two fast platforms: one that covers agnostic suites, one that covers device
PLATFORMS = ['qemu_x86', 'intel_adl_crb']


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestTagFilter:
    """Tests for --tag (-t) and --exclude-tag (-e) options."""

    @pytest.mark.fast
    @pytest.mark.parametrize(
        'tags, min_count, excluded_names',
        [
            (['agnostic'], 6, []),
            (['device'], 1, []),
            (['agnostic', 'device'], 7, []),
            # subgroup1 has platform_allow: native_sim/qemu_x86/qemu_x86_64;
            # with PLATFORMS=['qemu_x86','intel_adl_crb'], only qemu_x86 matches -> 1 active
            (
                ['odd'],
                1,
                ['dummy.agnostic.group1.subgroup2', 'dummy.agnostic.group2', 'dummy.device.group'],
            ),
            (['even'], 4, ['dummy.agnostic.group1.subgroup1']),
            (['nonexistent'], 0, []),
        ],
        ids=['agnostic', 'device', 'agnostic+device', 'odd', 'even', 'nonexistent'],
    )
    def test_tag_include(self, out_path, tags, min_count, excluded_names):
        """--tag includes only suites that carry the given tag."""
        args = (
            ['-i', '--outdir', out_path, '-T', DUMMY, '-y']
            + [v for t in tags for v in ('--tag', t)]
            + [v for p in PLATFORMS for v in ('-p', p)]
        )
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        cases = active_testcases(plan)
        assert len(cases) >= min_count
        names = {name for _, name, _ in cases}
        for excluded in excluded_names:
            assert excluded not in names, f'{excluded} should be excluded by tag filter'

    @pytest.mark.fast
    @pytest.mark.parametrize(
        'exclude_tags, must_remain, must_be_gone',
        [
            (['device'], ['dummy.agnostic.group1.subgroup1'], ['dummy.device.group']),
            (
                ['agnostic'],
                ['dummy.device.group'],
                ['dummy.agnostic.group1.subgroup1', 'dummy.agnostic.group2'],
            ),
            (
                ['odd'],
                ['dummy.agnostic.group2', 'dummy.agnostic.group1.subgroup2'],
                ['dummy.agnostic.group1.subgroup1'],
            ),
            (
                ['odd', 'device'],
                ['dummy.agnostic.group2'],
                ['dummy.agnostic.group1.subgroup1', 'dummy.device.group'],
            ),
        ],
        ids=['exclude-device', 'exclude-agnostic', 'exclude-odd', 'exclude-odd+device'],
    )
    def test_tag_exclude(self, out_path, exclude_tags, must_remain, must_be_gone):
        """--exclude-tag removes suites that carry the given tag."""
        args = (
            ['-i', '--outdir', out_path, '-T', DUMMY, '-y']
            + [v for t in exclude_tags for v in ('--exclude-tag', t)]
            + [v for p in PLATFORMS for v in ('-p', p)]
        )
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        names = {ts['name'] for ts in plan['testsuites']}
        for name in must_remain:
            assert name in names, f'{name} should still be present'
        for name in must_be_gone:
            assert name not in names, f'{name} should be excluded'

    @pytest.mark.fast
    def test_tag_include_and_exclude_combined(self, out_path):
        """--tag and --exclude-tag may be combined."""
        args = (
            ['-i', '--outdir', out_path, '-T', DUMMY, '-y']
            + ['--tag', 'agnostic', '--exclude-tag', 'odd']
            + [v for p in PLATFORMS for v in ('-p', p)]
        )
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        names = {ts['name'] for ts in plan['testsuites']}
        assert 'dummy.agnostic.group1.subgroup1' not in names
        assert 'dummy.agnostic.group2' in names


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestArchFilter:
    """Tests for --arch (-a) option."""

    @pytest.mark.fast
    @pytest.mark.parametrize(
        'arch, excluded',
        [
            ('x86', ['it8xxx2_evb']),
            ('arm', ['qemu_x86', 'hsdk']),
            ('riscv', ['qemu_x86', 'hsdk']),
        ],
        ids=['x86', 'arm', 'riscv'],
    )
    def test_arch_filter(self, capfd, out_path, arch, excluded):
        """--arch restricts testing to platforms of that architecture."""
        args = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '-y', '--arch', arch]
        assert twister_main(args) == 0

        _, err = capfd.readouterr()
        for platform_fragment in excluded:
            assert platform_fragment not in err or 'FILTERED' in err


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestVendorFilter:
    """Tests for --vendor option."""

    @pytest.mark.fast
    @pytest.mark.parametrize(
        'vendor, should_include, should_exclude',
        [
            ('intel', ['intel_adl_crb'], ['it8xxx2_evb']),
            ('ite', ['it8xxx2_evb'], ['intel_adl_crb', 'qemu_x86']),
            ('nxp', [], ['intel_adl_crb', 'qemu_x86', 'it8xxx2_evb']),
        ],
        ids=['intel', 'ite', 'nxp'],
    )
    def test_vendor_filter(self, capfd, out_path, vendor, should_include, should_exclude):
        """--vendor restricts testing to platforms from the specified vendor."""
        args = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '-y', '--vendor', vendor]
        assert twister_main(args) == 0

        _, err = capfd.readouterr()
        for fragment in should_exclude:
            assert fragment not in err or 'FILTERED' in err


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestSlowFilter:
    """Tests for --enable-slow and --enable-slow-only options."""

    @pytest.mark.fast
    def test_enable_slow_includes_slow_tests(self, out_path):
        """--enable-slow adds alt-config-root suites marked slow: true."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '-y',
            '--enable-slow',
            '--alt-config-root',
            ALT_CONFIG_ROOT,
            '-p',
            'qemu_x86',
            '-p',
            'intel_adl_crb',
        ]
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        names = {ts['name'] for ts in plan['testsuites']}
        # The alt-config-root adds dummy.agnostic.group2.alt which is marked slow
        assert any('alt' in name for name in names), (
            f'Expected a slow (alt) suite in plan; got {names!r}'
        )

    @pytest.mark.fast
    def test_enable_slow_only_limits_to_slow(self, out_path):
        """--enable-slow-only selects ONLY slow tests (excludes normal ones)."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '-y',
            '--enable-slow-only',
            '--alt-config-root',
            ALT_CONFIG_ROOT,
            '-p',
            'qemu_x86',
            '-p',
            'intel_adl_crb',
        ]
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        names = {ts['name'] for ts in plan['testsuites']}
        # Normal (non-slow) suites should NOT appear
        assert not any(
            name
            in (
                'dummy.agnostic.group1.subgroup1',
                'dummy.agnostic.group1.subgroup2',
                'dummy.agnostic.group2',
            )
            for name in names
        ), f'--enable-slow-only should exclude normal tests; got {names!r}'


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestPlatformPatternFilter:
    """Tests for --platform-pattern option.

    NOTE: --platform-pattern and -p are mutually exclusive in twister (elif chain).
    When -p is provided, the platform_pattern is ignored.  These tests use
    --platform-pattern without any explicit -p flag.
    """

    @pytest.mark.fast
    @pytest.mark.parametrize(
        'test_path, pattern, expected_match',
        [
            # Use agnostic/ (platform_allow: native_sim/qemu_x86/qemu_x86_64):
            # qemu_x86$ selects qemu_x86, which IS in platform_allow → only qemu_x86 appears
            (AGNOSTIC, 'qemu_x86$', 'qemu_x86'),
            # Use device/ (platform_allow: intel_adl_crb):
            # intel_adl_crb$ selects intel_adl_crb exactly
            (DEVICE, 'intel_adl_crb$', 'intel_adl_crb'),
        ],
        ids=['qemu_x86_pattern', 'intel_pattern'],
    )
    def test_platform_pattern(self, out_path, test_path, pattern, expected_match):
        """--platform-pattern selects only platforms matching the regex."""
        # No -p: let the pattern drive platform selection from all boards
        args = ['-i', '--outdir', out_path, '-T', test_path, '-y', '--platform-pattern', pattern]
        twister_main(args)

        plan = read_testplan(out_path)
        platforms_seen = {ts['platform'] for ts in plan['testsuites']}
        # All selected platforms must contain the expected match string
        assert all(expected_match in seen for seen in platforms_seen), (
            f'Unexpected platforms selected by {pattern!r}: {platforms_seen!r}'
        )
        assert len(platforms_seen) > 0, f'No platforms selected by pattern {pattern!r}'

    @pytest.mark.fast
    def test_platform_pattern_no_match_falls_back(self, out_path):
        """A --platform-pattern matching no board falls back to platform_allow.

        The pattern selects nothing, so increased_platform_scope kicks in and the
        suites are planned against their own platform_allow list instead.
        """
        bogus = 'nonexistent_xyz_board_12345'
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '-y',
            '--platform-pattern',
            f'^{bogus}$',
        ]
        assert twister_main(args) == 0

        platforms = {ts['platform'] for ts in read_testplan(out_path)['testsuites']}
        assert platforms, 'Expected a fallback to the platform_allow platforms'
        assert not any(bogus in p for p in platforms), (
            f'The non-matching pattern must not select {bogus}; got {platforms!r}'
        )


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestTestPatternFilter:
    """Tests for --test-pattern option."""

    @pytest.mark.fast
    @pytest.mark.parametrize(
        'pattern, expected_present, expected_absent',
        [
            (
                'dummy.agnostic.*',
                ['dummy.agnostic.group1.subgroup1', 'dummy.agnostic.group2'],
                ['dummy.device.group'],
            ),
            ('dummy.device.*', ['dummy.device.group'], ['dummy.agnostic.group1.subgroup1']),
            (
                'dummy.agnostic.group1.*',
                ['dummy.agnostic.group1.subgroup1', 'dummy.agnostic.group1.subgroup2'],
                ['dummy.agnostic.group2'],
            ),
        ],
        ids=['agnostic_pattern', 'device_pattern', 'group1_pattern'],
    )
    def test_test_pattern(self, out_path, pattern, expected_present, expected_absent):
        """--test-pattern selects suites whose name matches the regex."""
        args = ['-i', '--outdir', out_path, '-T', DUMMY, '-y', '--test-pattern', pattern] + [
            v for p in PLATFORMS for v in ('-p', p)
        ]
        twister_main(args)

        plan = read_testplan(out_path)
        names = {ts['name'] for ts in plan['testsuites']}
        for name in expected_present:
            assert name in names, f'Suite {name!r} should be in plan'
        for name in expected_absent:
            assert name not in names, f'Suite {name!r} should not be in plan'


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestExcludePlatform:
    """Tests for --exclude-platform (-P) option."""

    @pytest.mark.fast
    def test_exclude_single_platform(self, out_path):
        """--exclude-platform removes the given platform from the test run."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '-y',
            '--exclude-platform',
            'qemu_x86',
        ] + ['-p', 'qemu_x86', '-p', 'qemu_x86_64', '-p', 'native_sim']
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        platforms = {ts['platform'] for ts in plan['testsuites']}
        assert not any('qemu_x86/' in p or p == 'qemu_x86' for p in platforms)

    @pytest.mark.fast
    def test_exclude_multiple_platforms(self, out_path):
        """Multiple --exclude-platform flags are combined as a logical OR exclusion."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '-y',
            '--exclude-platform',
            'qemu_x86',
            '--exclude-platform',
            'qemu_x86_64',
        ] + ['-p', 'qemu_x86', '-p', 'qemu_x86_64', '-p', 'native_sim']
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        platforms = {ts['platform'] for ts in plan['testsuites']}
        assert all('qemu_x86' not in p for p in platforms)


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestFilterMode:
    """Tests for --filter {buildable,runnable} option."""

    @pytest.mark.fast
    @pytest.mark.parametrize(
        'mode, expected_max',
        [
            ('buildable', 7),
            ('runnable', 5),
        ],
        ids=['buildable', 'runnable'],
    )
    def test_filter_mode(self, out_path, mode, expected_max):
        """--filter controls whether non-runnable instances are included."""
        args = ['-i', '--outdir', out_path, '-T', DUMMY, '-y', '--filter', mode] + [
            v for p in PLATFORMS for v in ('-p', p)
        ]
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        cases = active_testcases(plan)
        assert len(cases) <= expected_max


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestIntegrationFilter:
    """Tests for -G / --integration option (run integration_platforms only)."""

    @pytest.mark.fast
    def test_integration_flag(self, out_path):
        """--integration restricts execution to integration_platforms."""
        # PLATFORMS = ['qemu_x86', 'intel_adl_crb'].
        # device/group has integration_platforms: intel_adl_crb -> 1 active case.
        # agnostic suites have integration_platforms: native_sim, not in PLATFORMS -> filtered.
        args = ['-i', '--outdir', out_path, '-T', DUMMY, '-y', '--integration'] + [
            v for p in PLATFORMS for v in ('-p', p)
        ]
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        cases = active_testcases(plan)
        # Only device/group on intel_adl_crb (its integration_platform) should be active
        assert len(cases) == 1
        assert cases[0][1] == 'dummy.device.group'

    @pytest.mark.fast
    def test_integration_flag_with_integration_platform(self, out_path):
        """--integration with native_sim should include suites whose integration_platforms
        includes native_sim."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '-y',
            '--integration',
            '-p',
            'native_sim',
        ]
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        cases = active_testcases(plan)
        assert len(cases) > 0


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestPlatformKeyFilter:
    """Tests for --ignore-platform-key (-K) option."""

    @pytest.mark.fast
    def test_platform_key_respected_by_default(self, out_path):
        """Without --ignore-platform-key, platform_key deduplicates test instances."""
        path = os.path.join(TEST_DATA, 'tests', 'platform_key', 'dummy')
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            path,
            '-y',
            '-p',
            'qemu_x86',
            '-p',
            'qemu_x86_64',
            '-p',
            'native_sim',
        ]
        twister_main(args)

        plan = read_testplan(out_path)
        active = active_testcases(plan)
        # platform_key deduplication reduces the number of active instances
        assert len(active) < 3

    @pytest.mark.fast
    def test_ignore_platform_key(self, out_path):
        """--ignore-platform-key disables platform_key deduplication."""
        path = os.path.join(TEST_DATA, 'tests', 'platform_key', 'dummy')
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            path,
            '-y',
            '--ignore-platform-key',
            '-p',
            'qemu_x86',
            '-p',
            'qemu_x86_64',
            '-p',
            'native_sim',
        ]
        twister_main(args)

        plan_ignore = read_testplan(out_path)
        active_ignore = active_testcases(plan_ignore)
        # All three platforms should now have active instances
        assert len(active_ignore) >= 1
