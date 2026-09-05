"""
Run the TTCN-3 address resolution conformance suite against a Zephyr instance.

Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: Apache-2.0

"""

from pathlib import Path

import pytest
from ttcn3_runner import build_suite, run_suite
from twister_harness import DeviceAdapter

SUITE = 'arp'


@pytest.fixture(scope='module')
def suite_binary() -> Path:
    return build_suite(SUITE)


def test_arp_conformance(network_lock, dut: DeviceAdapter, suite_binary: Path):
    """How it answers for its own address, and how it asks for another."""
    dut.readlines_until(regex='ARP responder ready', timeout=30.0)
    run_suite(suite_binary, SUITE)
