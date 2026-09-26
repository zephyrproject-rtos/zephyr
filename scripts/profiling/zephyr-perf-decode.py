#!/usr/bin/env python3
# Copyright (c) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
"""Decode zperf v1 blobs from Zephyr PMU sampling (perf record export).

Accepts either:
  * A raw binary file (magic ``ZPERFV01`` at offset 0), or
  * A text capture of ``shell_hexdump`` output (lines like ``00000000: 5a 50 ... |...|``),
 including extra lines such as ``zperf v1: N bytes`` from the shell.
"""

from __future__ import annotations

import argparse
import re
import struct
import sys

MAGIC = b"ZPERFV01"

HDR_FMT = "<8sIIIIII"  # magic, ver, hdr_bytes, n_sample, stride, lost, rsv
HDR_SIZE = struct.calcsize(HDR_FMT)
EVENT_FMT = "<I32s"
EVENT_SIZE = struct.calcsize(EVENT_FMT)
SAMPLE_FMT = "<QQIHH"  # tstamp, pc, tid, cpu, event
SAMPLE_SIZE = struct.calcsize(SAMPLE_FMT)

# Zephyr shell_hexdump_line: "%08X: " then hex bytes, then "|" ASCII "|"
_HEXDUMP_LINE = re.compile(r"^([0-9A-Fa-f]{8}):\s(.*)$")


def parse_shell_hexdump_text(text: str) -> bytes:
    """Extract binary from Zephyr ``shell_hexdump`` UART output."""
    out = bytearray()
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line:
            continue
        m = _HEXDUMP_LINE.match(line)
        if not m:
            continue
        hexpart = m.group(2)
        if "|" in hexpart:
            hexpart = hexpart.split("|", 1)[0]
        for pair in re.findall(r"[0-9a-fA-F]{2}", hexpart):
            out.append(int(pair, 16))
    return bytes(out)


def load_zperf_blob(path: str) -> tuple[bytes, str]:
    """
    Read file and return (binary_blob, source_description).

    Auto-detects raw zperf vs shell hexdump text.
    """
    with open(path, "rb") as f:
        raw = f.read()

    if len(raw) >= 8 and raw[:8] == MAGIC:
        return raw, "raw binary"

    # UART logs are often UTF-8 text containing hexdump lines.
    text = raw.decode("utf-8", errors="replace")
    parsed = parse_shell_hexdump_text(text)
    if len(parsed) >= 8 and parsed[:8] == MAGIC:
        return parsed, "shell hexdump (parsed)"

    if raw[:8] == b"00000000" or (len(raw) >= 2 and raw[0] in b"0123456789abcdefABCDEF"):
        print(
            "This file looks like a **text hexdump**, not raw binary.\n"
            "  `perf record export` prints `shell_hexdump` lines (e.g. `00000000:5a 50 ...`).\n"
            "  Re-run this script on the same file — it should auto-parse those lines.\n"
            "  If it still fails, ensure the capture includes the `5a 50 45 52 46 56 30 31` "
            "(ZPERFV01) lines and no serial corruption.\n"
            "  To make a raw .bin instead: paste only hex pairs into `xxd -r -p > out.bin` "
            "or use this script which accepts the full UART paste.",
            file=sys.stderr,
        )

    return parsed, "parsed (invalid magic)"


def decode(data: bytes, folded: bool) -> int:
    if len(data) < HDR_SIZE + EVENT_SIZE:
        print("decoded blob too small", file=sys.stderr)
        return 1

    magic, ver, hdr_bytes, n_sample, stride, lost, _rsv = struct.unpack_from(HDR_FMT, data, 0)
    if magic != MAGIC:
        print(
            f"bad magic {magic!r} (expected {MAGIC!r}).\n"
            "  If you saved UART text, include the hexdump lines from `perf record export`.",
            file=sys.stderr,
        )
        return 1
    if ver != 1:
        print(f"unsupported version {ver}", file=sys.stderr)
        return 1
    if stride != SAMPLE_SIZE:
        print(f"unexpected sample stride {stride} (expect {SAMPLE_SIZE})", file=sys.stderr)
        return 1
    if hdr_bytes != HDR_SIZE + EVENT_SIZE:
        print(f"unexpected header size {hdr_bytes}", file=sys.stderr)
        return 1

    evt_id, name_raw = struct.unpack_from(EVENT_FMT, data, HDR_SIZE)
    name = name_raw.split(b"\0", 1)[0].decode("ascii", errors="replace")

    off = hdr_bytes
    need = off + n_sample * SAMPLE_SIZE
    if len(data) < need:
        print(f"truncated blob: need {need} bytes have {len(data)}", file=sys.stderr)
        return 1

    print(f"# zperf v1 event=0x{evt_id:02x} {name!r} samples={n_sample} lost={lost}")

    for i in range(n_sample):
        tstamp, pc, tid, cpu, event = struct.unpack_from(SAMPLE_FMT, data, off)
        off += SAMPLE_SIZE
        if folded:
            print(f"0x{pc:016x} 1")
        else:
            print(f"{i}\t{tstamp}\t0x{pc:016x}\t0x{tid:08x}\t{cpu}\t0x{event:04x}")

    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    ap.add_argument(
        "zperf",
        help="raw zperf binary, or a text file / UART paste of `perf record export` hexdump",
    )
    ap.add_argument(
        "--folded",
        action="store_true",
        help="emit folded stacks (PC only) for flamegraph.pl",
    )
    args = ap.parse_args()

    blob, src = load_zperf_blob(args.zperf)
    if src != "raw binary":
        print(f"# input: {src}, {len(blob)} bytes", file=sys.stderr)

    return decode(blob, args.folded)


if __name__ == "__main__":
    raise SystemExit(main())
