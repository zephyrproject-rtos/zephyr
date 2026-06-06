# Copyright 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

"""
Generate an Intel HEX file containing nRF71 WICR register values
read from a Zephyr edtlib.EDT pickle.
"""

import argparse
import pickle
import struct
import sys
from pathlib import Path

from intelhex import IntelHex

WICR_COMPATIBLE = "nordic,nrf-wicr"


# ── Field table ──────────────────────────────────────────────────────
#
# Each entry describes one WICR register field configurable via DTS.
#
#   property     – DTS property name (must match the binding YAML)
#   offset       – byte offset from WICR base
#   size_offset  – (phandle only) byte offset for the companion SIZE register

WICR_FIELDS = [
    {"property": "firmware-lmacinitpc", "offset": 0x000},
    {"property": "firmware-umacinitpc", "offset": 0x004},
    {"property": "firmware-lmacrompatchaddr", "offset": 0x008},
    {"property": "firmware-umacrompatchaddr", "offset": 0x00C},
    {"property": "ipcconfig-commandmbox", "offset": 0x080, "size_offset": 0x084},
    {"property": "ipcconfig-eventmbox", "offset": 0x088, "size_offset": 0x08C},
    {"property": "ipcconfig-sparembox", "offset": 0x090, "size_offset": 0x094},
]


def setup_devicetree_path(zephyr_base):
    """Add the devicetree package to sys.path so EDT can be unpickled."""

    devicetree_path = Path(zephyr_base) / "scripts/dts/python-devicetree/src"
    if not devicetree_path.is_dir():
        sys.exit(f"Devicetree path does not exist: {devicetree_path}")
    sys.path.insert(0, str(devicetree_path))


def put_word(ih, addr, word):
    """Write a 32-bit little-endian word into the IntelHex."""

    ih.puts(addr, struct.pack("<I", word))


def parse_wicr(args):
    setup_devicetree_path(args.zephyr_base)

    with open(args.edt_pickle, "rb") as f:
        edt = pickle.load(f)

    wicr_nodes = edt.compat2okay.get(WICR_COMPATIBLE, [])
    if not wicr_nodes:
        IntelHex().write_hex_file(args.output)
        return

    wicr = wicr_nodes[0]
    base = wicr.regs[0].addr

    ih = IntelHex()
    wrote_any = False

    for field in WICR_FIELDS:
        prop = wicr.props.get(field["property"])
        if prop is None:
            continue

        target = prop.val
        if not target.regs:
            sys.exit(
                f"Phandle target for '{field['property']}' ({target.path}) has no reg property"
            )

        put_word(ih, base + field["offset"], target.regs[0].addr)

        size_offset = field.get("size_offset")
        if size_offset is not None:
            size_val = target.regs[0].size
            if size_val is None:
                sys.exit(
                    f"Phandle target for '{field['property']}' "
                    f"({target.path}) has no size in its reg property"
                )
            put_word(ih, base + size_offset, size_val)

        wrote_any = True

    if not wrote_any:
        IntelHex().write_hex_file(args.output)
        return

    ih.write_hex_file(args.output)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument(
        "--zephyr-base",
        required=True,
        help="Path to the Zephyr base directory",
    )
    parser.add_argument(
        "--edt-pickle",
        required=True,
        help="Path to the main application's EDT pickle file",
    )
    parser.add_argument(
        "--output",
        required=True,
        help="Path for the output Intel HEX file",
    )
    args = parser.parse_args()

    parse_wicr(args)
