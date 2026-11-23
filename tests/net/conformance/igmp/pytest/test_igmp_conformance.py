"""
Run the TTCN-3 IGMP host conformance suite against a Zephyr instance.

SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
SPDX-License-Identifier: Apache-2.0
"""

from pathlib import Path

import pytest
from ttcn3_runner import build_suite, run_suite
from twister_harness import DeviceAdapter

SUITE = 'igmp'


@pytest.fixture(scope='module')
def suite_binary() -> Path:
    return build_suite(SUITE)


def test_igmp_conformance(network_lock, dut: DeviceAdapter, suite_binary: Path):
    """How it reports its groups, and how it answers a querier."""
    dut.readlines_until(regex='IGMP host ready', timeout=30.0)
    run_suite(suite_binary, SUITE)
