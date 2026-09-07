"""
Run the TTCN-3 MQTT client conformance suite against a Zephyr instance.

Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: Apache-2.0

"""

from pathlib import Path

import pytest
from ttcn3_runner import build_suite, run_suite
from twister_harness import DeviceAdapter

SUITE = 'mqtt'


@pytest.fixture(scope='module')
def suite_binary() -> Path:
    return build_suite(SUITE)


def test_mqtt_conformance(network_lock, dut: DeviceAdapter, suite_binary: Path):
    """What the client sends, and which answers it acts on."""
    dut.readlines_until(regex='MQTT client ready', timeout=30.0)
    run_suite(suite_binary, SUITE)
