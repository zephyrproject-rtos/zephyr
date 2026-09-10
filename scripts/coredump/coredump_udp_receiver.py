#!/usr/bin/env python3
#
# Copyright (c) 2026 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0

"""Receive Zephyr coredump UDP frames and write a raw .bin for coredump_gdbserver.

For use with the UDP coredump logging backend (Kconfig
DEBUG_COREDUMP_BACKEND_LOGGING_UDP). The output file is the same binary stream
produced by coredump_serial_log_parser.py from UART #CD: hex lines, and is
consumed by coredump_gdbserver.py.

Protocol (all little-endian after 4-byte magic b'ZCDU'):
  uint32_t offset  — byte offset in the reconstructed stream
  uint16_t len     — payload length following this header (0 allowed)
  uint16_t flags   — bit0 = END (session complete); total size == offset in END datagram
  uint32_t seq     — sender sequence (for debugging)

Examples:
  IPv4 collector bound on all interfaces:
    python3 scripts/coredump/coredump_udp_receiver.py --listen 0.0.0.0 --port 17777 -o coredump.bin
  Dual-stack (listen IPv6 ``::``, IPv4-mapped enabled where supported):
    python3 scripts/coredump/coredump_udp_receiver.py --listen :: --port 17777 -o coredump.bin
"""

from __future__ import annotations

import argparse
import select
import socket
import struct
import sys
from pathlib import Path

HDR_FMT = "<4sIHHI"
HDR_SIZE = struct.calcsize(HDR_FMT)
FLAG_END = 0x0001


def udp_listen_socket(listen_addr: str, port: int) -> socket.socket:
    """AF_INET for dotted-quad listen addresses; AF_INET6 when ``listen_addr`` contains ':'"""
    if ":" in listen_addr:
        sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
        if hasattr(socket, "IPPROTO_IPV6") and hasattr(socket, "IPV6_V6ONLY"):
            sock.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, 0)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind((listen_addr, port))
        return sock

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((listen_addr, port))
    return sock


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )
    ap.add_argument("--listen", default="0.0.0.0")
    ap.add_argument("--port", type=int, default=17777)
    ap.add_argument("--output", "-o", type=Path, required=True)
    ap.add_argument(
        "--idle-timeout",
        type=float,
        default=30.0,
        help="Seconds without a packet before abort",
    )
    args = ap.parse_args()

    sock = udp_listen_socket(args.listen, args.port)
    sock.setblocking(False)

    chunks: dict[int, bytes] = {}
    total_expected: int | None = None
    packets = 0

    while total_expected is None:
        r, _, _ = select.select([sock], [], [], args.idle_timeout)
        if not r:
            print("ERROR: idle timeout waiting for coredump UDP", file=sys.stderr)
            return 1
        data, _addr = sock.recvfrom(65536)
        packets += 1
        if len(data) < HDR_SIZE:
            print(f"WARNING: short datagram len={len(data)}", file=sys.stderr)
            continue
        magic, off, plen, flags, _seq = struct.unpack(HDR_FMT, data[:HDR_SIZE])
        if magic != b"ZCDU":
            print(f"WARNING: bad magic {magic!r}", file=sys.stderr)
            continue
        if plen + HDR_SIZE > len(data):
            print("ERROR: length field exceeds datagram", file=sys.stderr)
            return 1
        if flags & FLAG_END:
            total_expected = off
            if plen != 0:
                print("WARNING: END flag but non-zero payload length", file=sys.stderr)
            continue
        payload = data[HDR_SIZE : HDR_SIZE + plen] if plen else b""
        if off in chunks and chunks[off] != payload:
            print(f"ERROR: conflicting chunk at offset {off}", file=sys.stderr)
            return 1
        chunks[off] = payload

    sock.close()

    if total_expected is None:
        return 1

    if not chunks and total_expected != 0:
        print("ERROR: no data chunks before END", file=sys.stderr)
        return 1

    ordered = sorted(chunks.items(), key=lambda x: x[0])
    out = bytearray()
    pos = 0
    for off, blob in ordered:
        if off != pos:
            print(f"ERROR: gap or overlap: expected offset {pos} got {off}", file=sys.stderr)
            return 1
        out.extend(blob)
        pos += len(blob)

    if len(out) != total_expected:
        print(
            f"ERROR: size mismatch: reassembled {len(out)} vs END offset {total_expected}",
            file=sys.stderr,
        )
        return 1

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(out)
    print(f"Wrote {args.output} ({len(out)} bytes, {packets} datagrams)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
