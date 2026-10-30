"""
Run the TTCN-3 MLD listener conformance suite against a Zephyr instance.

SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
SPDX-License-Identifier: Apache-2.0
"""

from pathlib import Path

import pytest
from ttcn3_runner import build_suite, run_suite
from twister_harness import DeviceAdapter

SUITE = 'mld'


@pytest.fixture(scope='module')
def suite_binary() -> Path:
    return build_suite(SUITE)


def test_mld_conformance(network_lock, dut: DeviceAdapter, suite_binary: Path):
    """How it reports the addresses it listens to, and how it answers a querier."""
    dut.readlines_until(regex='MLD listener ready', timeout=30.0)
    run_suite(suite_binary, SUITE)
