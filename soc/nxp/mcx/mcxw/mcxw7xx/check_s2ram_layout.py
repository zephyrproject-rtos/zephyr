# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

import argparse
from pathlib import Path

from elftools.elf.constants import SH_FLAGS
from elftools.elf.elffile import ELFFile


def main():
    parser = argparse.ArgumentParser(
        description="Check MCXW7x S2RAM low-STCM exclusion", allow_abbrev=False
    )
    parser.add_argument("elf", type=Path)
    parser.add_argument(
        "--region",
        nargs=2,
        action="append",
        type=lambda value: int(value, 0),
        required=True,
        metavar=("ADDRESS", "SIZE"),
        help="excluded DT range; repeat for each address alias",
    )
    args = parser.parse_args()
    for base, size in args.region:
        if base < 0 or size <= 0 or base + size > 1 << 32:
            parser.error("each excluded region must be a nonempty 32-bit address range")
    conflicts = []

    with args.elf.open("rb") as stream:
        for section in ELFFile(stream).iter_sections():
            start = section["sh_addr"]
            size = section["sh_size"]
            if not (section["sh_flags"] & SH_FLAGS.SHF_ALLOC) or size == 0:
                continue
            if any(start < base + length and base < start + size for base, length in args.region):
                conflicts.append(f"  {section.name}: [{start:#010x}, {start + size:#010x})")

    if conflicts:
        parser.exit(
            1,
            f"{args.elf}: allocated sections overlap the MCXW7x S2RAM low-STCM exclusion:\n"
            + "\n".join(conflicts)
            + "\n",
        )
    print("MCXW7x S2RAM ROM-safe layout PASS")


if __name__ == "__main__":
    main()
