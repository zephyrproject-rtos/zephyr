#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Blackbox tests for twister sanitizer options.

Covered options:
  --enable-asan    AddressSanitizer
  --enable-ubsan   UndefinedBehaviorSanitizer
  --enable-lsan    LeakSanitizer (Linux only, native_sim)
  --enable-valgrind  Valgrind memory checker

Each sanitizer test builds and runs a small native_sim program that
is crafted to trigger (or not trigger) the relevant error.
"""

import os
from unittest import mock

import pytest
from conftest import TEST_DATA, TEST_FILENAME_MOCK, requires_tool
from twisterlib.testplan import TestPlan
from twisterlib.twister_main import main as twister_main

SAN_BASE = os.path.join(TEST_DATA, 'tests', 'san')


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestUBSan:
    """Tests for --enable-ubsan option."""

    @pytest.mark.run
    @pytest.mark.parametrize(
        'flags, expected_exit',
        [
            ([], '0'),
            (['--enable-ubsan'], '1'),
        ],
        ids=['no-sanitiser', 'ubsan'],
    )
    def test_ubsan(self, out_path, flags, expected_exit):
        """--enable-ubsan causes UB-triggering code to exit with a failure."""
        path = os.path.join(SAN_BASE, 'ubsan')
        args = ['-i', '--outdir', out_path, '-T', path] + flags + ['-p', 'native_sim']
        result = twister_main(args)
        assert str(result) == expected_exit


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestASan:
    """Tests for --enable-asan option."""

    @pytest.mark.run
    @pytest.mark.parametrize(
        'flags, expected_exit',
        [
            ([], '0'),
            (['--enable-asan'], '1'),
        ],
        ids=['no-sanitiser', 'asan'],
    )
    def test_asan(self, out_path, flags, expected_exit):
        """--enable-asan causes memory errors to exit with a failure."""
        path = os.path.join(SAN_BASE, 'asan')
        args = ['-i', '--outdir', out_path, '-T', path] + flags + ['-p', 'native_sim']
        result = twister_main(args)
        assert str(result) == expected_exit


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestLSan:
    """Tests for --enable-lsan option."""

    @pytest.mark.run
    @pytest.mark.parametrize(
        'flags, expected_exit',
        [
            ([], '0'),
            (['--enable-lsan'], '0'),
        ],
        ids=['no-sanitiser', 'lsan'],
    )
    def test_lsan(self, out_path, flags, expected_exit):
        """--enable-lsan detects memory leaks in native_sim."""
        path = os.path.join(SAN_BASE, 'lsan')
        args = ['-i', '--outdir', out_path, '-T', path] + flags + ['-p', 'native_sim']
        result = twister_main(args)
        assert str(result) == expected_exit


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestValgrind:
    """Tests for --enable-valgrind option."""

    @pytest.mark.run
    @pytest.mark.parametrize(
        'flags, expected_exit',
        [
            ([], '0'),
            pytest.param(['--enable-valgrind'], '1', marks=requires_tool('valgrind')),
        ],
        ids=['no-sanitiser', 'valgrind'],
    )
    def test_valgrind(self, out_path, flags, expected_exit):
        """--enable-valgrind detects memory errors via Valgrind."""
        path = os.path.join(SAN_BASE, 'val')
        args = ['-i', '--outdir', out_path, '-T', path] + flags + ['-p', 'native_sim']
        result = twister_main(args)
        assert str(result) == expected_exit


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestSanitizerMutualExclusion:
    """Tests that mutually exclusive sanitizer flags are rejected."""

    @pytest.mark.fast
    def test_asan_and_valgrind_are_exclusive(self, out_path):
        """--enable-asan and --enable-valgrind cannot be used together."""
        path = os.path.join(SAN_BASE, 'asan')
        args = [
            '-i',
            '--outdir',
            out_path,
            '-T',
            path,
            '--enable-asan',
            '--enable-valgrind',
            '-p',
            'native_sim',
            '-y',
        ]
        # argparse mutually exclusive group raises SystemExit(2)
        with pytest.raises(SystemExit) as exc_info:
            twister_main(args)
        assert exc_info.value.code != 0
