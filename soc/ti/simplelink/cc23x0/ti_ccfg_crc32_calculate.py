#!/usr/bin/env python3
#
# Copyright (c) 2025 Fabian Pflug
#
# SPDX-License-Identifier: Apache-2.0

import sys
from zlib import crc32

from intelhex import IntelHex

CRC_LEN = 4
TI_CCFG_SECTION_START = 0x4E020000


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} source_file target_file")
        sys.exit(1)

    handle = IntelHex()
    handle.loadhex(sys.argv[1])

    # data from https://www.ti.com/lit/ug/swcu193a/swcu193a.pdf table 9-2 CRC Locations
    sections = [
        (0x0, 0xC),
        (0x10, 0x74C),
        (0x750, 0x7CC),
        (0x7D0, 0x7FC),
    ]

    for start, end in sections:
        data = handle.gets(TI_CCFG_SECTION_START + start, end - start)
        crc = (crc32(data) & 0xFFFFFFFF).to_bytes(CRC_LEN, "little")
        handle.puts(TI_CCFG_SECTION_START + end, crc)

    handle.write_hex_file(sys.argv[2])


if __name__ == "__main__":
    main()
