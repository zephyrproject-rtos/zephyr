"""
Run the TTCN-3 mDNS conformance suite against a Zephyr instance.

Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: Apache-2.0

"""

from pathlib import Path

import pytest
from ttcn3_runner import build_suite, run_suite
from twister_harness import DeviceAdapter

SUITE = 'mdns'


@pytest.fixture(scope='module')
def suite_binary() -> Path:
    return build_suite(SUITE)


def test_mdns_conformance(network_lock, dut: DeviceAdapter, suite_binary: Path):
    """The responder answers the suite the way the tests expect."""
    dut.readlines_until(regex='mDNS responder ready', timeout=30.0)
    run_suite(suite_binary, SUITE)
