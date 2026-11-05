# Copyright 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

"""
Generate an Intel HEX file containing nRF71 UICR register values
read from a Zephyr edtlib.EDT pickle.
"""

import argparse
import pickle
import struct
import sys
from pathlib import Path

from intelhex import IntelHex

UICR_COMPATIBLE = "nordic,nrf71-uicr"

# ── Field table ──────────────────────────────────────────────────────
#
# Each entry describes one UICR register field configurable via DTS.
#
#   property – DTS property name (must match the binding YAML)
#   offset   – byte offset from UICR base
#   mask     – bit mask of the field within the 32-bit register
#   encoding – optional mapping from DTS string values to integer field values
#   signed   – optional flag indicating that integer values are signed
#   reset    – 32-bit reset value
#
# The full register word is computed as:
#   word = (reset & ~mask) | encoded_value
#

UICR_FIELDS = [
    {
        "property": "supply-config1v8",
        "offset": 0x400,
        "mask": 0x00000003,
        "encoding": {
            "normal": 0,
            "external": 1,
            "high-load": 2,
        },
        "reset": 0xFFFFFFFF,
    },
    {
        "property": "xtalcomp-offset",
        "offset": 0x440,
        "mask": 0xFFFFFFFF,
        "signed": True,
        "reset": 0xFFFFFFFF,
    },
]


def setup_devicetree_path(zephyr_base):
    """Add the devicetree package to sys.path so EDT can be unpickled."""

    devicetree_path = Path(zephyr_base) / "scripts/dts/python-devicetree/src"
    if not devicetree_path.is_dir():
        sys.exit(f"Devicetree path does not exist: {devicetree_path}")
    sys.path.insert(0, str(devicetree_path))


def encode_field_value(field, value):
    """Validate and encode a field value for insertion into its register."""

    mask = field["mask"]
    if field.get("signed", False):
        width = mask.bit_length()
        if mask == 0 or mask != (1 << width) - 1:
            sys.exit(f"Signed mask 0x{mask:x} for '{field['property']}' must start at bit 0")

        value_min = -(1 << (width - 1))
        value_max = (1 << (width - 1)) - 1
        if not value_min <= value <= value_max:
            sys.exit(
                f"Value {value} for '{field['property']}' is outside signed "
                f"{width}-bit range [{value_min}, {value_max}]"
            )

        # Convert a negative value to its unsigned two's-complement bit pattern.
        if value < 0:
            return (1 << width) + value

    if value != (value & mask):
        sys.exit(f"Value 0x{value:x} for '{field['property']}' exceeds mask 0x{mask:x}")

    return value


def parse_uicr(args):
    setup_devicetree_path(args.zephyr_base)

    with open(args.edt_pickle, "rb") as f:
        edt = pickle.load(f)

    # Find the UICR node.
    uicr_nodes = edt.compat2okay.get(UICR_COMPATIBLE, [])
    if not uicr_nodes:
        IntelHex().write_hex_file(args.output)
        return

    uicr = uicr_nodes[0]
    base = uicr.regs[0].addr

    ih = IntelHex()

    for field in UICR_FIELDS:
        prop = uicr.props.get(field["property"])
        if prop is None:
            continue

        value = prop.val
        encoding = field.get("encoding")
        mask = field["mask"]
        if encoding is not None:
            if value not in encoding:
                valid = ", ".join(f'"{k}"' for k in encoding)
                sys.exit(f"Unknown value '{value}' for '{field['property']}'. Expected: {valid}")
            value = encoding[value]

        encoded_value = encode_field_value(field, value)

        word = (field["reset"] & ~mask) | encoded_value
        addr = base + field["offset"]
        ih.puts(addr, struct.pack("<I", word))

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

    parse_uicr(args)
