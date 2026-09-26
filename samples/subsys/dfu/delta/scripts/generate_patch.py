#!/usr/bin/env python3

# Copyright (c) 2026 Clovis Corde
# SPDX-License-Identifier: Apache-2.0

"""Generate a delta patch using bsdiff with heatshrink compression."""

import argparse
import logging
import os
import sys
import tempfile
from pathlib import Path

import bsdiffhs

_log_file = os.path.join(tempfile.gettempdir(), "generate_patch.log")
logging.basicConfig(
    filename=_log_file,
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s",
)

_stderr_handler = logging.StreamHandler(sys.stderr)
_stderr_handler.setLevel(logging.INFO)
_stderr_handler.setFormatter(logging.Formatter("%(levelname)s: %(message)s"))
logging.getLogger().addHandler(_stderr_handler)


def validated_path(arg, must_exist):
    """Resolve a CLI path argument and check it points at a usable location."""
    path = Path(arg).resolve()
    if must_exist and not path.is_file():
        logging.error("Not a regular file: %s", path)
        sys.exit(1)
    if not must_exist and not path.parent.is_dir():
        logging.error("Directory does not exist: %s", path.parent)
        sys.exit(1)
    return str(path)


def create_patch(src, dst, patch, window_sz2, lookahead_sz2, max_patch_size):
    """Generate a patch between the source and target firmware."""
    try:
        bsdiffhs.file_diff(src, dst, patch, window_sz2, lookahead_sz2)
    except Exception:
        logging.exception("Patch generation failed")
        sys.exit(1)

    with open(patch, "rb") as patch_file:
        original = patch_file.read()

    if original[:8] != b"BSDIFFHS":
        logging.error(
            "Unexpected magic in generated patch: %r (expected b'BSDIFFHS')", original[:8]
        )
        sys.exit(1)

    # bsdiffhs produces a 16-byte header: 8-byte magic + 8-byte LE target size.
    # We insert 2 backend-specific bytes (window_sz2, lookahead_sz2) immediately
    # after those 16 bytes to form the 18-byte BSDIFFHS extended header consumed
    # by lib/delta/backends/bsdiffhs/bsdiffhs.c.
    header_16 = original[:16]
    rest = original[16:]

    extended = header_16 + bytes([window_sz2]) + bytes([lookahead_sz2]) + rest

    with open(patch, "wb") as patch_file:
        patch_file.write(extended)

    patch_size = len(extended)
    if patch_size > max_patch_size:
        logging.error("Patch size %d exceeds maximum %d bytes", patch_size, max_patch_size)
        sys.exit(1)

    logging.info("Patch generated: %d bytes", patch_size)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Generate a delta patch using bsdiff with heatshrink compression.",
        allow_abbrev=False,
    )
    parser.add_argument("fw_source", type=str, help="Source firmware file.")
    parser.add_argument("fw_target", type=str, help="Target firmware file.")
    parser.add_argument("patch_file", type=str, help="Output patch file.")
    parser.add_argument(
        "window_sz2",
        type=int,
        help="Heatshrink window size (log2). Typical value: 10.",
    )
    parser.add_argument(
        "lookahead_sz2",
        type=int,
        help="Heatshrink lookahead size (log2). Typical value: 4.",
    )
    parser.add_argument(
        "max_size_patch",
        type=int,
        help="Maximum allowed patch size in bytes.",
    )

    args = parser.parse_args()

    if not 4 <= args.window_sz2 <= 15:
        logging.error("window_sz2 must be in range [4, 15], got %d", args.window_sz2)
        sys.exit(1)
    if not 3 <= args.lookahead_sz2 <= 14:
        logging.error("lookahead_sz2 must be in range [3, 14], got %d", args.lookahead_sz2)
        sys.exit(1)
    if args.lookahead_sz2 >= args.window_sz2:
        logging.error(
            "lookahead_sz2 (%d) must be less than window_sz2 (%d)",
            args.lookahead_sz2,
            args.window_sz2,
        )
        sys.exit(1)

    logging.info(
        "Script executed: Source FW: '%s', Target FW: '%s'",
        args.fw_source,
        args.fw_target,
    )

    create_patch(
        validated_path(args.fw_source, must_exist=True),
        validated_path(args.fw_target, must_exist=True),
        validated_path(args.patch_file, must_exist=False),
        args.window_sz2,
        args.lookahead_sz2,
        args.max_size_patch,
    )
