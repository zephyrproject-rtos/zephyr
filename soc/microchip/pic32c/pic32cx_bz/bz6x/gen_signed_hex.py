#!/usr/bin/env python3
#
# Copyright 2025 Microchip
#
# SPDX-License-Identifier: Apache-2.0
#

import os
import struct
import sys

from intelhex import IntelHex

METADATA_ADDR = 0x1000000
METADATA_LENGTH = 512
fw_start_addr = 0x01000200
image_length_bytes = 0


def build_metadata(seq_num, auth_mthd, img_rev):
    metadata = bytearray([0x00] * METADATA_LENGTH)

    manu_id = b'PHCM'
    metadata[24:28] = manu_id

    # seq num: 4 bytes (Little Endian)
    seq_bytes = seq_num.to_bytes(4, 'little')
    metadata[60:64] = seq_bytes
    metadata[64] = 0x03
    metadata[65] = 0x01

    # auth method: 1 byte
    metadata[66] = auth_mthd & 0xFF

    metadata[70] = 0x74

    # fw img revision: 4 bytes (Little Endian)
    img_bytes = img_rev.to_bytes(4, 'little')
    metadata[72:76] = img_bytes

    metadata[76:80] = fw_start_addr.to_bytes(4, 'little')
    metadata[80:84] = fw_start_addr.to_bytes(4, 'little')
    metadata[84:88] = image_length_bytes

    return metadata


def inject_metadata(input_hex, output_hex, seq_num, auth_mthd, img_rev):
    ih = IntelHex(input_hex)
    metadata = build_metadata(seq_num, auth_mthd, img_rev)

    for offset, byte in enumerate(metadata):
        ih[METADATA_ADDR + offset] = byte

    ih.write_hex_file(output_hex)


if __name__ == "__main__":
    if len(sys.argv) != 7:
        print("Incorrect input format!!")
    else:
        input_hex = sys.argv[1]
        input_bin = sys.argv[2]
        output_hex = sys.argv[3]

        seq_num = int(sys.argv[4], 16)
        auth_mthd = int(sys.argv[5])
        img_rev = int(sys.argv[6], 16)

        image_length = os.path.getsize(input_bin)
        image_length_bytes = struct.pack('<I', image_length)

        inject_metadata(input_hex, output_hex, seq_num, auth_mthd, img_rev)
