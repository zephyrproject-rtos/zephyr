# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Drive a Zephyr CAN controller from the host end of the bus.

The twister ``can`` sidecar brings up a per-instance SocketCAN interface and
reports its name through the ``sidecar_params`` fixture, so this test opens a
raw CAN socket on that very interface and talks to the guest directly. Only the
standard library is needed; a raw CAN socket does not receive the frames it
sends, so the two directions cannot be confused.
"""

import socket
import struct
from pathlib import Path

import pytest
from twister_harness import DeviceAdapter

# Linux struct can_frame: id (u32), dlc (u8), 3 bytes of padding, data[8].
CAN_FRAME_FMT = '<IB3x8s'
CAN_FRAME_SIZE = 16
CAN_STD_ID_MASK = 0x7FF

GUEST_TX_ID = 0x123
GUEST_TX_DATA = bytes((0xAB, 0xCD))
GUEST_RX_ID = 0x321
HOST_TX_DATA = bytes((0x01, 0x02, 0x03))


@pytest.fixture()
def can_socket(sidecar_params: dict[str, str]):
    """A raw CAN socket bound to the interface the sidecar created."""
    sock = socket.socket(socket.PF_CAN, socket.SOCK_RAW, socket.CAN_RAW)
    sock.bind((sidecar_params['iface'],))
    sock.settimeout(10)
    try:
        yield sock
    finally:
        sock.close()


def test_sidecar_reports_the_bus(sidecar_params: dict[str, str]):
    """The sidecar hands the test the interface it brought up for this run."""
    iface = sidecar_params['iface']
    assert iface, 'the can sidecar reported no interface'
    assert Path('/sys/class/net', iface).exists(), f'{iface} does not exist on the host'


def test_can_frames_cross_both_ways(dut: DeviceAdapter, can_socket: socket.socket):
    dut.readlines_until(regex='CAN host test ready', timeout=30.0)

    # guest -> host: the guest sends continuously, so read until its frame shows
    # up (anything else on the bus is ignored).
    for _ in range(50):
        can_id, dlc, data = struct.unpack(CAN_FRAME_FMT, can_socket.recv(CAN_FRAME_SIZE))
        if can_id & CAN_STD_ID_MASK == GUEST_TX_ID:
            break
    else:
        pytest.fail(f'no frame with id {GUEST_TX_ID:#x} received from the guest')

    assert dlc == len(GUEST_TX_DATA)
    assert data[:dlc] == GUEST_TX_DATA

    # host -> guest: send a burst so a frame lands while the guest is in its
    # receive window, then check it reported the frame back over the console.
    frame = struct.pack(CAN_FRAME_FMT, GUEST_RX_ID, len(HOST_TX_DATA), HOST_TX_DATA)
    for _ in range(20):
        can_socket.send(frame)

    expected = f'rx id={GUEST_RX_ID:#x} dlc={len(HOST_TX_DATA)} data={HOST_TX_DATA.hex()}'
    dut.readlines_until(regex=expected, timeout=30.0)
