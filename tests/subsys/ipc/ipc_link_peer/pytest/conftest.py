# SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
# SPDX-License-Identifier: Apache-2.0

import re

import pytest
from ipc_link_peer import Peer
from twister_harness import DeviceAdapter

PTY_LINE = re.compile(r'uart_1 connected to pseudotty: (/dev/\S+)')


@pytest.fixture
def peer(dut: DeviceAdapter):
    """The Python side of the link, on the device's second PTY UART."""
    lines = dut.readlines_until(regex=PTY_LINE.pattern, timeout=10)
    matches = [m.group(1) for line in lines if (m := PTY_LINE.search(line))]
    assert matches, 'the link UART printed no pseudo terminal'
    port = matches[0]
    dut.readlines_until(regex=r'ipc_link_peer: ready', timeout=10)
    peer = Peer(port)
    yield peer
    peer.close()
