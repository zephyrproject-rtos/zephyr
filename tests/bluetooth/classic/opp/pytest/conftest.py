# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=duplicate-code

import logging
import re

import pytest
from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)

BD_ADDR_REGEX = r'Identity: *(?P<bd_addr>([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2}) *\((.*?)\))'


def _init_one(dut: DeviceAdapter, shell: Shell) -> str:
    """Initialize Bluetooth on a single DUT and return its BD address."""
    shell.exec_command("bt init")
    lines = dut.readlines_until(regex="Bluetooth initialized")

    bd_addr = None
    for line in lines:
        logger.info(f"Shell log {line}")
        m = re.search(BD_ADDR_REGEX, line)
        if m:
            bd_addr = m.group('bd_addr')

    if bd_addr is None:
        logger.error('Fail to get IUT BD address')
        raise AssertionError

    return bd_addr.split(" ")[0]


@pytest.fixture(name='initialize', scope='session')
def fixture_initialize(duts: list[DeviceAdapter], shells: list[Shell]):
    """Initialize both DUTs; DUT0 is the OPP client, DUT1 is the OPP server."""
    assert len(duts) > 1, "This board-to-board test requires two DUTs."
    assert len(shells) > 1, "This board-to-board test requires two DUTs."

    client_addr = _init_one(duts[0], shells[0])
    server_addr = _init_one(duts[1], shells[1])

    logger.info(f'initialized: client {client_addr}, server {server_addr}')
    return client_addr, server_addr
