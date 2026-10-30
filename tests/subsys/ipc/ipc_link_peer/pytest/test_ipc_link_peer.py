# SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
# SPDX-License-Identifier: Apache-2.0

"""The IPC link backend against a peer written from its documentation page."""

import pytest
from ipc_link_peer import (
    CTRL_UNBIND,
    CTRL_UNBIND_ACK,
    FLAG_BOOT,
    FLAG_HDR_ERR,
    Message,
    Peer,
    cobs_decode,
    cobs_encode,
    crc16_ccitt,
    pack_messages,
)
from twister_harness import DeviceAdapter

# The retransmit-timeout-ms and mtu of the link node in app.overlay
RETRANSMIT_S = 0.1
MTU = 256


def test_vectors():
    """The peer's own building blocks, against known values."""
    assert crc16_ccitt(b'123456789') == 0x6F91
    for data in (b'', b'\x00', b'\x01\x00\x02', bytes(range(256)), bytes(300)):
        encoded = cobs_encode(data)
        assert b'\x00' not in encoded
        assert cobs_decode(encoded) == data
    for truncated in (b'\x02', b'\x03\x01', b'\x02\x01\x03\x02'):
        with pytest.raises(ValueError):
            cobs_decode(truncated)
    assert pack_messages([Message(1, b'abc')]) == b'\x01\x00\x03\x00abc\x00'


def test_handshake_and_bind(dut: DeviceAdapter, peer: Peer):
    """The device announces its boot, the session starts, endpoints bind by name."""
    peer.connect()
    assert peer.frames[0].flags & FLAG_BOOT
    peer.bind('echo')
    dut.readlines_until(regex=r'ipc_link_peer: bound echo', timeout=2)
    peer.bind('sink')
    dut.readlines_until(regex=r'ipc_link_peer: bound sink', timeout=2)


def test_echo(dut: DeviceAdapter, peer: Peer):
    peer.connect()
    peer.bind('echo')
    peer.send('echo', b'hello')
    dut.readlines_until(regex=r'ipc_link_peer: received echo 5', timeout=2)
    assert peer.wait_message('echo') == b'hello'


def test_largest_message(dut: DeviceAdapter, peer: Peer):
    """A message filling the frame: the frame size less 20 bytes."""
    peer.connect()
    peer.bind('echo')
    data = bytes(range(MTU - 20))
    peer.send('echo', data)
    assert peer.wait_message('echo') == data


def test_retransmit(dut: DeviceAdapter, peer: Peer):
    """An unacknowledged payload is sent again after the retransmission timeout."""
    peer.connect()
    peer.bind('echo')
    peer.auto_ack = False
    peer.send('echo', b'again', wait_ack=False)

    def is_echo(frame):
        return any(m.data == b'again' for m in frame.messages)

    first = peer.wait_frame(is_echo)
    second = peer.wait_frame(lambda f: is_echo(f) and f.seq == first.seq, timeout=3 * RETRANSMIT_S)
    assert second is not first

    # Acknowledged, it stops: one more copy may already be on its way
    peer.auto_ack = True
    peer.send_frame()
    peer.run_until(lambda: False, timeout=2 * RETRANSMIT_S)
    start = len(peer.frames)
    peer.run_until(lambda: False, timeout=3 * RETRANSMIT_S)
    assert not any(is_echo(f) for f in peer.frames[start:])


def test_header_error(dut: DeviceAdapter, peer: Peer):
    """A corrupted header is answered with a header carrying the error flag."""
    peer.connect()
    peer.bind('echo')
    peer.corrupt_next_header = True
    peer.send_frame()
    frame = peer.wait_frame(lambda f: f.flags & FLAG_HDR_ERR)
    assert frame.header_ok


def test_credits(dut: DeviceAdapter, peer: Peer):
    """With no credits advertised, the device holds its payloads."""
    peer.connect()
    peer.bind('echo')
    peer.credits = 0
    peer.send_frame()
    start = len(peer.frames)
    peer.send('echo', b'held')
    dut.readlines_until(regex=r'ipc_link_peer: received echo 4', timeout=2)
    peer.run_until(lambda: False, timeout=5 * RETRANSMIT_S)
    assert not any(f.length > 0 for f in peer.frames[start:]), 'sent without credits'

    peer.credits = 2
    peer.send_frame()
    assert peer.wait_message('echo') == b'held'


def test_peer_reboot(dut: DeviceAdapter, peer: Peer):
    """The peer boots again: the device unbinds its endpoints and binds them again."""
    peer.connect()
    peer.bind('echo')
    dut.readlines_until(regex=r'ipc_link_peer: bound echo', timeout=2)
    peer.reset()
    peer.connect()
    dut.readlines_until(regex=r'ipc_link_peer: unbound echo', timeout=2)
    peer.bind('echo')
    dut.readlines_until(regex=r'ipc_link_peer: bound echo', timeout=2)
    peer.send('echo', b'back')
    assert peer.wait_message('echo') == b'back'


def test_unbind(dut: DeviceAdapter, peer: Peer):
    """An unbind is acknowledged once the device unbound the endpoint."""
    peer.connect()
    peer.bind('echo')
    dut.readlines_until(regex=r'ipc_link_peer: bound echo', timeout=2)
    peer.send_ctrl(CTRL_UNBIND, peer.endpoints['echo'])
    dut.readlines_until(regex=r'ipc_link_peer: unbound echo', timeout=2)
    frame = peer.wait_frame(
        lambda f: any(
            m.addr == 0 and m.data[0] == CTRL_UNBIND_ACK and m.data[1] == peer.endpoints['echo']
            for m in f.messages
        )
    )
    assert frame.payload_ok


def test_frame_size_mismatch(dut: DeviceAdapter, peer: Peer):
    """A peer with another frame size is reported, and never connects."""
    peer.connect()
    peer.bind('echo')
    peer.send_frame(mtu=512)
    dut.readlines_until(regex=r'another protocol version or frame size', timeout=2)
    dut.readlines_until(regex=r'ipc_link_peer: unbound echo', timeout=2)

    # The device announces its boot again; with the right size, it connects again
    peer.wait_frame(lambda f: f.flags & FLAG_BOOT, timeout=3 * RETRANSMIT_S)
    peer.bind('echo')
    dut.readlines_until(regex=r'ipc_link_peer: bound echo', timeout=2)
