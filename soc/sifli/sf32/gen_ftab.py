#!/usr/bin/env python3
#
# Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
# SPDX-License-Identifier: Apache-2.0

"""
Generate the SF32LB52 platform ftab image from devicetree information.
For the specific ftab format, please refer to <https://docs.sifli.com/projects/sdk/latest/en/sf32lb52x/bootloader.html>
"""

from __future__ import annotations

import argparse
import enum
import os
import pickle
import struct
import sys
from pathlib import Path

from intelhex import IntelHex


def _zephyr_base() -> Path:
    try:
        return Path(os.environ["ZEPHYR_BASE"]).resolve()
    except KeyError as exc:  # pragma: no cover - build-time helper
        raise SystemExit("ZEPHYR_BASE environment variable is not set") from exc


def _load_edt(pickle_path: Path):
    zephyr_base = _zephyr_base()
    sys.path.insert(0, str(zephyr_base / "scripts/dts/python-devicetree/src"))

    with pickle_path.open("rb") as edt_pickle:
        return pickle.load(edt_pickle)


def _partition_bounds(node) -> tuple[int, int]:
    if not node or not node.regs:
        raise SystemExit(f"Devicetree node '{getattr(node, 'path', '?')}' has no 'reg' entries")

    reg = node.regs[0]
    return reg.addr, reg.size


def _flash_base_address(flash_node) -> int:
    controller = flash_node.parent
    if controller is None:
        raise SystemExit(f"Flash node '{flash_node.path}' has no parent controller")

    reg_names = controller.props.get("reg-names")
    region_index = None
    if reg_names:
        try:
            region_index = list(reg_names.val).index("nor")
        except ValueError:
            region_index = None

    if region_index is None:
        if len(controller.regs) == 1:
            region_index = 0
        else:
            raise SystemExit(
                f"Unable to determine flash base for '{controller.path}'. "
                "Ensure a 'nor' region exists in reg-names."
            )

    return controller.regs[region_index].addr


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate the SF32LB ftab image", allow_abbrev=False
    )
    parser.add_argument("--edt-pickle", required=True, help="Path to the pickled edtlib.EDT object")
    parser.add_argument(
        "--output", required=True, help="Path to write the generated ftab Intel HEX"
    )
    return parser.parse_args()


class PartitionIndex(enum.IntEnum):
    FLASH_PARTITION_TABLE = 0
    CALIBRATION_TABLE = 1
    LCPU_IMAGE_PING = 2
    SECONDARY_BOOTLOADER = 3


class ImageFlag(enum.IntEnum):
    DFU_FLAG_ENC = 1
    DFU_FLAG_AUTO = 2
    DFU_FLAG_SINGLE = 4
    DFU_IMGHDR_KEY_OFFSET = 8
    DFU_FLAG_COMPRESS = 16


PARTITION_ENTRY_COUNT = 16
IMAGE_DESCRIPTION_COUNT = 14
IMAGE_DESCRIPTION_SIZE = 512
PUBKEY_SIZE = 294
RESERVED_SIZE = 3542
IMAGE_INDEX_COUNT = 4
PARTITION_INFO_STRUCT = struct.Struct("<IIII")
FLASH_TABLE_SIZE = 0x8000
FTAB_TOTAL_SIZE = 11280


def _build_partition_table(flash_base: int, code_start: int, code_size: int) -> bytes:
    entries = [(0, 0, 0, 0)] * PARTITION_ENTRY_COUNT
    entries[PartitionIndex.FLASH_PARTITION_TABLE.value] = (
        flash_base,
        FLASH_TABLE_SIZE,
        0,
        0,
    )
    sec_idx = PartitionIndex.SECONDARY_BOOTLOADER.value
    entries[sec_idx] = (code_start, code_size, code_start, 0)

    out = bytearray()
    for start, size, xip, flag in entries:
        out.extend(PARTITION_INFO_STRUCT.pack(start, size, xip, flag))
    return bytes(out)


def _pack_image_description(length: int, used: bool) -> bytes:
    block_size = 512 if used else 0
    flags = ImageFlag.DFU_FLAG_AUTO if used else 0
    header = struct.pack("<IHH", length, block_size, flags)
    payload_len = IMAGE_DESCRIPTION_SIZE - len(header)
    return header + bytes(payload_len)


def _build_image_descriptions(sec_index: int, code_size: int) -> bytes:
    desc = bytearray()
    for idx in range(IMAGE_DESCRIPTION_COUNT):
        used = idx == sec_index
        length = code_size if used else 0xFFFFFFFF
        desc.extend(_pack_image_description(length, used))
    return bytes(desc)


def _build_image_index_table(flash_base: int, sec_index: int) -> bytes:
    base_offset = (
        flash_base
        + 4
        + PARTITION_ENTRY_COUNT * PARTITION_INFO_STRUCT.size
        + PUBKEY_SIZE
        + RESERVED_SIZE
    )
    entries = [0xFFFFFFFF] * IMAGE_INDEX_COUNT
    if 0 <= sec_index < IMAGE_INDEX_COUNT:
        entries[sec_index] = base_offset + sec_index * IMAGE_DESCRIPTION_SIZE
    return struct.pack("<" + "I" * IMAGE_INDEX_COUNT, *entries)


def main() -> None:
    args = _parse_args()
    edt = _load_edt(Path(args.edt_pickle))

    flash_node = edt.chosen_node("zephyr,flash")
    if flash_node is None:
        raise SystemExit("Unable to locate 'zephyr,flash' in devicetree")
    flash_base = _flash_base_address(flash_node)

    code_node = edt.chosen_node("zephyr,code-partition")
    if code_node is None:
        raise SystemExit("Unable to locate 'zephyr,code-partition' in devicetree")

    try:
        ptable_node = edt.label2node["ptable"]
    except KeyError as exc:
        raise SystemExit("Unable to locate 'ptable' labeled partition in devicetree") from exc

    code_offset, code_size = _partition_bounds(code_node)
    code_start = flash_base + code_offset
    ptable_offset, _ = _partition_bounds(ptable_node)
    ptable_addr = flash_base + ptable_offset
    sec_image_desc_index = (
        PartitionIndex.SECONDARY_BOOTLOADER.value - PartitionIndex.LCPU_IMAGE_PING.value
    )

    blob = bytearray()
    blob.extend(struct.pack("<I", 0x53454346))
    blob.extend(_build_partition_table(flash_base, code_start, code_size))
    blob.extend(bytes(PUBKEY_SIZE))
    blob.extend(bytes(RESERVED_SIZE))
    blob.extend(_build_image_descriptions(sec_image_desc_index, code_size))
    blob.extend(_build_image_index_table(flash_base, sec_image_desc_index))

    if len(blob) != FTAB_TOTAL_SIZE:
        raise SystemExit(
            f"Generated blob size {len(blob)} does not match expected {FTAB_TOTAL_SIZE}"
        )

    out_path = Path(args.output)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    hex_image = IntelHex()
    hex_image.frombytes(blob, offset=ptable_addr)
    hex_image.write_hex_file(str(out_path))


if __name__ == "__main__":
    main()
