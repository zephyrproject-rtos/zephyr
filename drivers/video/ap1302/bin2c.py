#!/usr/bin/env python3
#
# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0

"""Convert a binary blob to a C source file defining a const uint8_t array.

This is intended for build-time embedding of firmware blobs without depending
on objcopy or target-specific object formats.

Usage:
  bin2c.py --input <file.bin> --output <file.c> --symbol <name>
"""

from __future__ import annotations

import argparse
from pathlib import Path


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--input", required=True)
    ap.add_argument("--output", required=True)
    ap.add_argument("--symbol", required=True)
    args = ap.parse_args()

    inp = Path(args.input)
    out = Path(args.output)
    sym = args.symbol

    data = inp.read_bytes()

    # Keep line length reasonable; 12 bytes per line is readable.
    per_line = 12

    lines: list[str] = []
    lines.append("/* SPDX-License-Identifier: Apache-2.0 */\n")
    lines.append("/* Auto-generated from: %s */\n" % inp.as_posix())
    lines.append("#include <stddef.h>\n")
    lines.append("#include <stdint.h>\n\n")

    lines.append("const uint8_t %s[] = {\n" % sym)
    for i in range(0, len(data), per_line):
        chunk = data[i : i + per_line]
        lines.append("\t" + ", ".join(f"0x{b:02x}" for b in chunk) + ",\n")
    lines.append("};\n\n")
    lines.append("const size_t %s_len = %dU;\n" % (sym, len(data)))

    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("".join(lines), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
