"""
Run the TTCN-3 DNS resolver conformance suite against a Zephyr instance.

Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: Apache-2.0

"""

from pathlib import Path

import pytest
from ttcn3_runner import build_suite, run_suite
from twister_harness import DeviceAdapter

SUITE = 'dns'


@pytest.fixture(scope='module')
def suite_binary() -> Path:
    return build_suite(SUITE)


def test_dns_conformance(network_lock, dut: DeviceAdapter, suite_binary: Path):
    """The queries the resolver sends, and what it does with the answers."""
    dut.readlines_until(regex='DNS resolver ready', timeout=30.0)
    run_suite(suite_binary, SUITE)
