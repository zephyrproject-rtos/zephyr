"""
Run the TTCN-3 DHCPv4 server conformance suite against a Zephyr instance.

Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: Apache-2.0

"""

from pathlib import Path

import pytest
from ttcn3_runner import build_suite, run_suite
from twister_harness import DeviceAdapter

SUITE = 'dhcpv4_server'


@pytest.fixture(scope='module')
def suite_binary() -> Path:
    return build_suite(SUITE)


def test_dhcpv4_server_conformance(network_lock, dut: DeviceAdapter, suite_binary: Path):
    """What the server offers, what it acknowledges, and what it refuses."""
    dut.readlines_until(regex='DHCPv4 server ready', timeout=30.0)
    run_suite(suite_binary, SUITE)
