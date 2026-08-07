#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Blackbox tests for twister quarantine options.

Covered options:
  --quarantine-list FILENAME   Mark listed tests as quarantined (skip them)
  --quarantine-verify          Invert quarantine: only run quarantined tests

Quarantine list (test_data/twister-quarantine-list.yml) entries:
  * All platforms: dummy.agnostic.group1.subgroup1
  * intel_adl_crb/alder_lake only: (platform-level quarantine)
  * native_sim/native/64 + dummy.agnostic.group1.subgroup2: (scenario+platform quarantine)
"""

import os
import re
from unittest import mock

import pytest
from conftest import TEST_DATA, TEST_FILENAME_MOCK, active_testcases, read_testplan
from twisterlib.testplan import TestPlan
from twisterlib.twister_main import main as twister_main

DUMMY = os.path.join(TEST_DATA, 'tests', 'dummy')
AGNOSTIC = os.path.join(DUMMY, 'agnostic')
QUARANTINE_FILE = os.path.join(TEST_DATA, 'twister-quarantine-list.yml')
PLATFORMS = ['native_sim', 'native_sim/native/64', 'intel_adl_crb']


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestQuarantineList:
    """Tests for --quarantine-list option."""

    @pytest.mark.fast
    def test_quarantine_list_skips_listed_tests(self, capfd, out_path):
        """Tests in the quarantine list are marked as SKIPPED (Quarantined)."""
        args = [
            '--detailed-test-id',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '--quarantine-list',
            QUARANTINE_FILE,
            '-vv',
            '-ll',
            'DEBUG',
        ] + [v for p in PLATFORMS for v in ('-p', p)]
        assert twister_main(args) == 0

        _, err = capfd.readouterr()
        # Scenario quarantine: subgroup1 on all platforms
        assert re.search(
            r'agnostic/group1/subgroup1/dummy\.agnostic\.group1\.subgroup1 SKIPPED: Quarantined',
            err,
        ), 'Expected subgroup1 to be quarantined on all platforms'

        # Scenario+platform quarantine: subgroup2 only on native_sim/native/64
        assert re.search(
            r'agnostic/group1/subgroup2/dummy\.agnostic\.group1\.subgroup2 SKIPPED: Quarantined',
            err,
        ), 'Expected subgroup2 to be quarantined on native_sim/native/64'

    @pytest.mark.fast
    def test_quarantine_list_leaves_non_quarantined_active(self, out_path):
        """Tests not in the quarantine list are still included."""
        # Use -y (dry-run) so names are short dotted form, not path-prefixed
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '-y',
            '--quarantine-list',
            QUARANTINE_FILE,
        ] + [v for p in PLATFORMS for v in ('-p', p)]
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        # group2 is not quarantined; at least one instance should be active
        active = active_testcases(plan)
        active_names = {name for _, name, _ in active}
        assert 'dummy.agnostic.group2' in active_names

    @pytest.mark.fast
    def test_quarantine_all_platforms_entry(self, out_path):
        """A quarantine entry without a platforms key applies to all platforms."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '-y',
            '--quarantine-list',
            QUARANTINE_FILE,
        ] + [v for p in PLATFORMS for v in ('-p', p)]
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        # subgroup1 is quarantined on every platform; none should be active
        active = active_testcases(plan)
        for _, name, _ in active:
            assert name != 'dummy.agnostic.group1.subgroup1', (
                'subgroup1 should be quarantined on all platforms'
            )

    @pytest.mark.fast
    def test_quarantine_platform_specific_entry(self, out_path):
        """A quarantine entry with a platforms key only applies to those platforms."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            AGNOSTIC,
            '-y',
            '--quarantine-list',
            QUARANTINE_FILE,
        ] + [v for p in PLATFORMS for v in ('-p', p)]
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        # subgroup2 on native_sim/native/64 is quarantined; subgroup2 on native_sim should remain
        active = active_testcases(plan)
        ns32_active = [(p, n) for p, n, _ in active if p in ('native_sim', 'native_sim/native')]
        subgroup2_ns32 = [n for _, n in ns32_active if n == 'dummy.agnostic.group1.subgroup2']
        assert len(subgroup2_ns32) > 0, (
            'subgroup2 should be active on native_sim (only quarantined on native_sim/native/64)'
        )


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestQuarantineVerify:
    """Tests for --quarantine-verify option."""

    @pytest.mark.fast
    def test_quarantine_verify_inverts_selection(self, out_path):
        """--quarantine-verify runs only the tests that are in the quarantine list."""
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            DUMMY,
            '-y',
            '--quarantine-verify',
            '--quarantine-list',
            QUARANTINE_FILE,
        ] + ['-p', 'native_sim', '-p', 'intel_adl_crb']
        assert twister_main(args) == 0

        plan = read_testplan(out_path)
        # In quarantine-verify mode only the quarantined tests should be non-skipped
        non_skipped = [ts for ts in plan['testsuites'] if ts.get('status') != 'skipped']
        assert len(non_skipped) == 2

    @pytest.mark.fast
    def test_quarantine_verify_requires_quarantine_list(self, out_path, capsys):
        """--quarantine-verify without --quarantine-list should exit with an error."""
        args = ['-i', '--outdir', out_path, '-T', AGNOSTIC, '-y', '--quarantine-verify'] + [
            '-p',
            'native_sim',
        ]
        result = twister_main(args)
        captured = capsys.readouterr()
        assert result != 0 or 'quarantine' in captured.err.lower()
