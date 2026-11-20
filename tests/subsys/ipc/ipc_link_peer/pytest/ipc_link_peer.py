# SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
# SPDX-License-Identifier: Apache-2.0

"""A peer of the IPC link backend, in Python.

It speaks the wire protocol over a serial port: COBS-framed frames with a 12 byte header
and its CRC-16, a payload of messages and its own CRC-16, the boot handshake,
acknowledgments, credits and the control channel. Everything is explicit, so a test can withhold an
acknowledgment, corrupt a header or starve the peer of credits on purpose.
"""

import struct
import time
from dataclasses import dataclass, field

import serial

VERSION = 1
HDR_SIZE = 12
PCRC_SIZE = 2
MSG_HDR_SIZE = 4
MSG_ALIGN = 4

FLAG_BOOT = 1 << 0
FLAG_BOOT_ACK = 1 << 1
FLAG_HDR_ERR = 1 << 2
FLAG_POLL = 1 << 3

CTRL_ADDR = 0
CTRL_BIND = 1
CTRL_BIND_ACK = 2
CTRL_UNBIND = 3
CTRL_UNBIND_ACK = 4

HDR_FORMAT = '<BBBBBBHHH'


def crc16_ccitt(data: bytes, seed: int = 0xFFFF) -> int:
    """CRC-16/MCRF4XX: polynomial 0x1021 reflected, no final XOR, check value 0x6F91.

    Covers the header and, separately, the payload.
    """
    crc = seed
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ 0x8408 if crc & 1 else crc >> 1
    return crc


def cobs_encode(data: bytes) -> bytes:
    out = bytearray()
    block = bytearray()
    for byte in data:
        if byte == 0:
            out.append(len(block) + 1)
            out += block
            block = bytearray()
        else:
            block.append(byte)
            if len(block) == 254:
                out.append(255)
                out += block
                block = bytearray()
    out.append(len(block) + 1)
    out += block
    return bytes(out)


def cobs_decode(data: bytes) -> bytes:
    out = bytearray()
    i = 0
    while i < len(data):
        code = data[i]
        if code == 0 or i + code > len(data):
            raise ValueError('bad COBS block')
        out += data[i + 1 : i + code]
        i += code
        if code < 255 and i < len(data):
            out.append(0)
    return bytes(out)


@dataclass
class Message:
    addr: int
    data: bytes


@dataclass
class Frame:
    version: int = VERSION
    flags: int = 0
    seq: int = 0
    ack: int = 0
    credits: int = 0
    length: int = 0
    mtu: int = 0
    header_ok: bool = True
    payload_ok: bool = True
    messages: list = field(default_factory=list)

    def encode(self, payload: bytes = b'') -> bytes:
        header = struct.pack(
            HDR_FORMAT,
            self.version,
            self.flags,
            self.seq,
            self.ack,
            self.credits,
            0,
            len(payload),
            self.mtu,
            0,
        )[:-2]
        header += struct.pack('<H', crc16_ccitt(header))
        if payload:
            payload += struct.pack('<H', crc16_ccitt(payload))
        return header + payload


def pack_messages(messages: list) -> bytes:
    payload = bytearray()
    for msg in messages:
        payload += struct.pack('<BBH', msg.addr, 0, len(msg.data)) + msg.data
        payload += bytes(-len(payload) % MSG_ALIGN)
    return bytes(payload)


def unpack_messages(payload: bytes) -> list:
    messages = []
    offset = 0
    while offset + MSG_HDR_SIZE <= len(payload):
        addr, _, length = struct.unpack_from('<BBH', payload, offset)
        start = offset + MSG_HDR_SIZE
        if start + length > len(payload):
            raise ValueError('message past the payload')
        messages.append(Message(addr, payload[start : start + length]))
        offset = start + length
        offset += -offset % MSG_ALIGN
    if offset != len(payload):
        raise ValueError('messages do not fill the payload')
    return messages


class Peer:
    """One side of a link, driven explicitly by the test.

    The peer keeps the protocol state, the endpoints it registered and what it received.
    service() reads and processes one frame and answers what the frame asks for; the
    helpers around it run service() until something happened.
    """

    def __init__(self, port: str, mtu: int = 256, credits: int = 2):
        self.serial = serial.Serial(port, timeout=0.05)
        self.mtu = mtu
        self.credits = credits
        self.auto_ack = True
        self.corrupt_next_header = False
        self.reset()
        self.endpoints = {}
        self.remote = {}
        self.announced = {}
        self.next_addr = 1
        self.received = []
        self.frames = []
        self.rx_buffer = bytearray()
        self.tx_queue = []

    def reset(self):
        """Start a session: announce the boot until the peer acknowledges it."""
        self.booting = True
        self.boot_ack_owed = False
        self.peer_booting = False
        self.connected = False
        self.seq = 0
        self.expected = 0
        self.peer_ack = 0
        self.peer_credits = 0
        self.ack_owed = False
        self.hdr_err_owed = False
        self.remote = {}
        self.announced = {}
        self.tx_queue = []

    def close(self):
        self.serial.close()

    # Sending

    def send_raw(self, frame: bytes):
        self.serial.write(cobs_encode(frame) + b'\x00')
        self.serial.flush()

    def send_frame(
        self,
        flags: int = 0,
        messages: list | None = None,
        seq: int | None = None,
        mtu: int | None = None,
        ack: int | None = None,
        credits: int | None = None,
    ):
        """Send one frame: a header, and a payload when there are messages."""
        if self.booting:
            flags |= FLAG_BOOT
        if self.boot_ack_owed:
            flags |= FLAG_BOOT_ACK
            self.boot_ack_owed = False
        if self.hdr_err_owed:
            flags |= FLAG_HDR_ERR
            self.hdr_err_owed = False
        self.ack_owed = False
        payload = pack_messages(messages) if messages else b''
        frame = Frame(
            flags=flags,
            seq=seq if seq is not None else 0,
            ack=self.expected if ack is None else ack,
            credits=self.credits if credits is None else credits,
            mtu=self.mtu if mtu is None else mtu,
        )
        raw = frame.encode(payload)
        if self.corrupt_next_header:
            raw = bytes([raw[0]]) + bytes([raw[1] ^ 0x80]) + raw[2:]
            self.corrupt_next_header = False
        self.send_raw(raw)

    def in_flight(self) -> int:
        return (self.seq - self.peer_ack) & 0xFF

    def flush(self):
        """Send the queued payloads the peer has credits for, one message per payload."""
        while self.tx_queue and self.in_flight() < self.peer_credits:
            seq = self.seq
            self.seq = (self.seq + 1) & 0xFF
            self.send_frame(messages=[self.tx_queue.pop(0)], seq=seq)

    def send_message(self, addr: int, data: bytes):
        """Queue one payload with one message, and send it once the peer has credits for it.

        Returns the sequence number it goes out with.
        """
        seq = (self.seq + len(self.tx_queue)) & 0xFF
        self.tx_queue.append(Message(addr, data))
        self.flush()
        return seq

    def send_ctrl(self, op: int, addr: int, name: str = ''):
        return self.send_message(CTRL_ADDR, bytes([op, addr]) + name.encode())

    def send_bind_ack(self, addr: int, peer_addr: int):
        """Answer the bind of the device's endpoint at peer_addr with the endpoint at addr."""
        return self.send_message(CTRL_ADDR, bytes([CTRL_BIND_ACK, addr, peer_addr]))

    # Receiving

    def read_frame(self, timeout: float) -> Frame | None:
        """Read the next COBS block and parse it, without acting on it.

        A block already buffered is returned at once: the port is only read while none is.
        """
        deadline = time.monotonic() + timeout
        while True:
            if b'\x00' in self.rx_buffer:
                block, _, self.rx_buffer = self.rx_buffer.partition(b'\x00')
                if not block:
                    continue
                return self.parse(block)
            if time.monotonic() >= deadline:
                return None
            self.rx_buffer += self.serial.read(self.serial.in_waiting or 1)

    def parse(self, block: bytes) -> Frame:
        frame = Frame(header_ok=False)
        try:
            raw = cobs_decode(bytes(block))
        except ValueError:
            return frame
        if len(raw) < HDR_SIZE:
            return frame
        fields = struct.unpack_from(HDR_FORMAT, raw)
        if crc16_ccitt(raw[: HDR_SIZE - 2]) != fields[8]:
            return frame
        frame = Frame(
            version=fields[0],
            flags=fields[1],
            seq=fields[2],
            ack=fields[3],
            credits=fields[4],
            length=fields[6],
            mtu=fields[7],
        )
        if frame.length:
            payload = raw[HDR_SIZE : HDR_SIZE + frame.length]
            crc = raw[HDR_SIZE + frame.length : HDR_SIZE + frame.length + PCRC_SIZE]
            if (
                len(payload) != frame.length
                or len(crc) != PCRC_SIZE
                or struct.unpack('<H', crc)[0] != crc16_ccitt(payload)
            ):
                frame.payload_ok = False
            else:
                try:
                    frame.messages = unpack_messages(payload)
                except ValueError:
                    frame.payload_ok = False
        return frame

    def process(self, frame: Frame):
        """Update the state from a frame, as the protocol says."""
        self.frames.append(frame)
        if not frame.header_ok:
            self.hdr_err_owed = True
            return
        if frame.version != VERSION or frame.mtu != self.mtu:
            return
        peer_boot = bool(frame.flags & FLAG_BOOT)
        if peer_boot and not self.peer_booting:
            # The peer started a session: forget the previous one
            self.seq = 0
            self.expected = 0
            self.peer_ack = 0
            self.remote = {}
            self.announced = {}
            self.tx_queue = []
            self.connected = True
        self.peer_booting = peer_boot
        if peer_boot:
            self.boot_ack_owed = True
        if self.booting:
            if not frame.flags & FLAG_BOOT_ACK:
                return
            self.booting = False
            self.connected = True
        if frame.flags & (FLAG_HDR_ERR | FLAG_POLL):
            self.ack_owed = True
        self.peer_ack = frame.ack
        self.peer_credits = frame.credits
        if frame.length and frame.payload_ok:
            self.ack_owed = True
            if frame.seq == self.expected:
                self.expected = (self.expected + 1) & 0xFF
                for msg in frame.messages:
                    self.deliver(msg)

    def deliver(self, msg: Message):
        if msg.addr != CTRL_ADDR:
            self.received.append(msg)
            return
        op, addr = msg.data[0], msg.data[1]
        if op == CTRL_BIND:
            name = msg.data[2:].decode()
            self.announced[name] = addr
            if name in self.endpoints:
                self.remote[name] = addr
                self.send_bind_ack(self.endpoints[name], addr)
        elif op == CTRL_BIND_ACK:
            # Names the endpoint it answers, which must still be the one that asked
            for name, own in self.endpoints.items():
                if own == msg.data[2]:
                    self.remote[name] = addr
        elif op == CTRL_UNBIND:
            for name, remote in list(self.remote.items()):
                if remote == addr:
                    del self.remote[name]
            self.send_ctrl(CTRL_UNBIND_ACK, addr)
        elif op == CTRL_UNBIND_ACK:
            pass

    def service(self, timeout: float = 0.5) -> Frame | None:
        """Read one frame, process it and answer what it asks for."""
        frame = self.read_frame(timeout)
        if frame is not None:
            self.process(frame)
            self.flush()
            if self.auto_ack and (
                self.ack_owed or self.boot_ack_owed or self.hdr_err_owed or self.booting
            ):
                self.send_frame()
        return frame

    def run_until(self, condition, timeout: float = 2.0, probe: float = 0.2) -> bool:
        """Service the link until the condition holds, announcing the boot while booting."""
        deadline = time.monotonic() + timeout
        last_probe = 0.0
        while time.monotonic() < deadline:
            if condition():
                return True
            if self.booting and time.monotonic() - last_probe > probe:
                self.send_frame()
                last_probe = time.monotonic()
            self.service(timeout=0.05)
        return condition()

    # Helpers for the tests

    def connect(self, timeout: float = 3.0):
        assert self.run_until(lambda: self.connected and not self.booting, timeout), (
            'no session with the device'
        )

    def bind(self, name: str, timeout: float = 2.0) -> int:
        """Register an endpoint and bind it to the device's endpoint of that name.

        Returns once the device acknowledged everything sent, so that it is bound there too.
        """
        if name not in self.endpoints:
            self.endpoints[name] = self.next_addr
            self.next_addr += 1
        if name in self.announced:
            self.remote[name] = self.announced[name]
            self.send_bind_ack(self.endpoints[name], self.announced[name])
        else:
            self.send_ctrl(CTRL_BIND, self.endpoints[name], name)
        assert self.run_until(
            lambda: name in self.remote and not self.tx_queue and self.in_flight() == 0, timeout
        ), f'{name} not bound'
        return self.remote[name]

    def send(self, name: str, data: bytes, wait_ack: bool = True) -> int:
        seq = self.send_message(self.remote[name], data)
        if wait_ack:
            acked = (seq + 1) & 0xFF
            assert self.run_until(lambda: self.peer_ack == acked), f'seq {seq} not acknowledged'
        return seq

    def wait_message(self, name: str, timeout: float = 2.0) -> bytes:
        addr = self.endpoints[name]

        def arrived():
            return any(m.addr == addr for m in self.received)

        assert self.run_until(arrived, timeout), f'nothing received for {name}'
        for i, msg in enumerate(self.received):
            if msg.addr == addr:
                return self.received.pop(i).data
        raise AssertionError

    def wait_frame(self, predicate, timeout: float = 2.0) -> Frame:
        start = len(self.frames)

        def seen():
            return any(predicate(f) for f in self.frames[start:])

        assert self.run_until(seen, timeout), 'expected frame not received'
        return next(f for f in self.frames[start:] if predicate(f))
