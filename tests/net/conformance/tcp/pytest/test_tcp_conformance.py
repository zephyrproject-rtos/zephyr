"""
Run the TTCN-3 TCP conformance suite against a Zephyr instance.

Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: Apache-2.0

"""

from pathlib import Path

import pytest
from ttcn3_runner import build_suite, run_suite
from twister_harness import DeviceAdapter

SUITE = 'tcp'


@pytest.fixture(scope='module')
def suite_binary() -> Path:
    return build_suite(SUITE)


def test_tcp_conformance(network_lock, dut: DeviceAdapter, suite_binary: Path):
    """The handshake, data, close, and what it does with a malformed segment."""
    dut.readlines_until(regex='TCP echo server ready', timeout=30.0)
    run_suite(suite_binary, SUITE)
