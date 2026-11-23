#!/usr/bin/env python3
#
# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
#
# SPDX-License-Identifier: Apache-2.0

"""Copy an llext file, moving a .rela.text entry past the end of .text."""

import argparse
import struct
import sys

from elftools.elf.elffile import ELFFile


def main():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument("input", help="input llext file")
    parser.add_argument("output", help="output llext file")
    args = parser.parse_args()

    with open(args.input, "rb") as f:
        data = bytearray(f.read())
        f.seek(0)
        elf = ELFFile(f)
        rela = elf.get_section_by_name(".rela.text")
        if rela is None:
            sys.exit(f"{args.input}: no .rela.text section")
        index = next(
            (i for i, r in enumerate(rela.iter_relocations()) if r["r_info_type"] != 0), None
        )
        if index is None:
            sys.exit(f"{args.input}: no .rela.text entry to modify")
        tgt_size = elf.get_section(rela["sh_info"])["sh_size"]
        entry = rela["sh_offset"] + index * rela["sh_entsize"]
        fmt = ("<" if elf.little_endian else ">") + ("I" if elf.elfclass == 32 else "Q")

    struct.pack_into(fmt, data, entry, tgt_size)

    with open(args.output, "wb") as f:
        f.write(data)


if __name__ == "__main__":
    main()
