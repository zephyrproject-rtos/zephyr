"""
Run the ETSI derived TTCN-3 CoAP suite against a Zephyr instance.

Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: Apache-2.0

"""

from pathlib import Path

import pytest
from ttcn3_runner import build_suite, run_suite
from twister_harness import DeviceAdapter

SUITE = 'coap'


@pytest.fixture(scope='module')
def suite_binary() -> Path:
    return build_suite(SUITE)


def test_coap_conformance(network_lock, dut: DeviceAdapter, suite_binary: Path):
    """The server answers the ETSI CoAP core test cases as they require."""
    dut.readlines_until(regex='CoAP server ready', timeout=30.0)
    run_suite(suite_binary, SUITE)
