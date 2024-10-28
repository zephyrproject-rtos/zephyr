#!/usr/bin/env python
"""
Utility script to assist in the migration of a board from hardware model v1
(HWMv1) to hardware model v2 (HWMv2).

.. warning::
    This script is not a complete migration tool. It is meant to assist in the
    migration process, but it does not handle all cases.

This script requires the following arguments:

- ``-b|--board``: The name of the board to migrate.
- ``-g|--group``: The group the board belongs to. This is used to group a set of
  boards in the same folder. In HWMv2, the boards are no longer organized by
  architecture.
- ``-v|--vendor``: The vendor name.
- ``-s|--soc``: The SoC name.

In some cases, the new board name will differ from the old board name. For
example, the old board name may have the SoC name as a suffix, while in HWMv2,
this is no longer needed. In such cases, ``-n|--new-board`` needs to be
provided.

For boards with variants, ``--variants`` needs to be provided.

For out-of-tree boards, provide ``--board-root`` pointing to the custom board
root.

Copyright (c) 2023 Nordic Semiconductor ASA
SPDX-License-Identifier: Apache-2.0
"""

import argparse
from pathlib import Path
import re
import sys

import ruamel.yaml


ZEPHYR_BASE = Path(__file__).parents[2]


def board_v1_to_v2(board_root, board, new_board, group, vendor, soc, variants):
    try:
        board_path = next(board_root.glob(f"boards/*/{board}"))
    except StopIteration:
        sys.exit(f"Board not found: {board}")

    new_board_path = board_root / "boards" / group / new_board
    if new_board_path.exists():
        print("New board already exists, updating board with additional SoC")
        if not soc:
            sys.exit("No SoC provided")

    new_board_path.mkdir(parents=True, exist_ok=True)

    print("Moving files to the new board folder...")
    for f in board_path.iterdir():
        f_new = new_board_path / f.name
        if f_new.exists():
            print(f"Skipping existing file: {f_new}")
            continue
        f.rename(f_new)

    print("Creating or updating board.yaml...")
    board_settings_file = new_board_path / "board.yml"
    if not board_settings_file.exists():
        board_settings = {
            "board": {
                "name": new_board,
                "vendor": vendor,
                "socs": []
            }
        }
    else:
        with open(board_settings_file, encoding='utf-8') as f:
            yaml = ruamel.yaml.YAML(typ='safe', pure=True)
            board_settings = yaml.load(f) # pylint: disable=assignment-from-no-return

    soc = {"name": soc}
    if variants:
        soc["variants"] = [{"name": variant} for variant in variants]

    board_settings["board"]["socs"].append(soc)

    yaml = ruamel.yaml.YAML()
    yaml.indent(sequence=4, offset=2)
    with open(board_settings_file, "w") as f:
        yaml.dump(board_settings, f)

    print(f"Updating {board}_defconfig...")
    board_defconfig_file = new_board_path / f"{board}_defconfig"
    with open(board_defconfig_file) as f:
        board_soc_settings = []
        board_defconfig = ""

        dropped_line = False
        for line in f.readlines():
            m = re.match(r"^CONFIG_BOARD_.*$", line)
            if m:
                dropped_line = True
                continue

            m = re.match(r"^CONFIG_(SOC_[A-Z0-9_]+).*$", line)
            if m:
                dropped_line = True
                if not re.match(r"^CONFIG_SOC_SERIES_.*$", line):
                    board_soc_settings.append(m.group(1))
                continue

            if dropped_line and re.match(r"^$", line):
                continue

            dropped_line = False
            board_defconfig += line

    with open(board_defconfig_file, "w") as f:
        f.write(board_defconfig)

    print("Updating Kconfig.defconfig...")
    board_kconfig_defconfig_file = new_board_path / "Kconfig.defconfig"
    with open(board_kconfig_defconfig_file) as f:
        board_kconfig_defconfig = ""
        has_kconfig_defconfig_entries = False

        in_board = False
        for line in f.readlines():
            # drop "config BOARD" entry from Kconfig.defconfig
            m = re.match(r"^config BOARD$", line)
            if m:
                in_board = True
                continue

            if in_board and re.match(r"^\s+.*$", line):
                continue

            in_board = False

            m = re.match(r"^config .*$", line)
            if m:
                has_kconfig_defconfig_entries = True

            m = re.match(rf"^(.*)BOARD_{board.upper()}(.*)$", line)
            if m:
                board_kconfig_defconfig += (
                    m.group(1) + "BOARD_" + new_board.upper() + m.group(2) + "\n"
                )
                continue

            board_kconfig_defconfig += line

    if has_kconfig_defconfig_entries:
        with open(board_kconfig_defconfig_file, "w") as f:
            f.write(board_kconfig_defconfig)
    else:
        print("Removing empty Kconfig.defconfig after update...")
        board_kconfig_defconfig_file.unlink()

    print(f"Creating or updating Kconfig.{new_board}...")
    board_kconfig_file = new_board_path / "Kconfig.board"
    copyright = None
    with open(board_kconfig_file) as f:
        for line in f.readlines():
            if "Copyright" in line:
                copyright = line
    new_board_kconfig_file = new_board_path / f"Kconfig.{new_board}"
    header = "# SPDX-License-Identifier: Apache-2.0\n"
    if copyright is not None:
        header = copyright + header
    selects = "\n\t" + "\n\t".join(["select " + setting for setting in board_soc_settings]) + "\n"
    if not new_board_kconfig_file.exists():
        with open(new_board_kconfig_file, "w") as f:
            f.write(
                header +
                f"\nconfig BOARD_{new_board.upper()}{selects}"
            )
    else:
        with open(new_board_kconfig_file, "a") as f:
            f.write(selects)

    print("Removing old Kconfig.board...")
    board_kconfig_file.unlink()

    print("Conversion done!")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(allow_abbrev=False)

    parser.add_argument(
        "--board-root",
        type=Path,
        default=ZEPHYR_BASE,
        help="Board root",
    )

    parser.add_argument("-b", "--board", type=str, required=True, help="Board name")
    parser.add_argument("-n", "--new-board", type=str, help="New board name")
    parser.add_argument("-g", "--group", type=str, required=True, help="Board group")
    parser.add_argument("-v", "--vendor", type=str, required=True, help="Vendor name")
    parser.add_argument("-s", "--soc", type=str, required=True, help="Board SoC")
    parser.add_argument("--variants", nargs="+", default=[], help="Board variants")

    args = parser.parse_args()

    board_v1_to_v2(
        args.board_root,
        args.board,
        args.new_board or args.board,
        args.group,
        args.vendor,
        args.soc,
        args.variants,
    )
