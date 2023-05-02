# Copyright (c) 2023 Chen Xingyu <hi@xingrz.me>
# SPDX-License-Identifier: Apache-2.0

import argparse
import struct
import hashlib


def align_up(x, align):
    return (x + align - 1) & ~(align - 1)


def main():
    parser = argparse.ArgumentParser(
        description='Add header and footer to a binary file')
    parser.add_argument('--binary', help='binary file to be signed')
    args = parser.parse_args()

    with open(args.binary, 'rb') as f:
        data = f.read()

    # Pad data to 16-byte boundary
    data += bytearray([0] * (align_up(len(data), 16) - len(data)))

    # Add 64-byte header
    header = bytearray()
    header += struct.pack('>I', 0xAA55AA55)
    header += struct.pack('<I', 56)
    header += bytearray([0] * 24)
    header += struct.pack('<I', 64)
    header += struct.pack('<I', len(data))
    header += struct.pack('>I', 0x00003F00)
    header += struct.pack('<I', len(data) + 64)
    header += struct.pack('<I', 32)
    header += struct.pack('>I', 0xCC33CC33)
    header += bytearray([0] * 8)

    # Add 48-byte footer
    footer = bytearray()
    footer += hashlib.sha256(header + data).digest()
    footer += bytearray([0] * 8)
    footer += struct.pack('<I', len(data) + 64 + 48)
    footer += struct.pack('>I', 0xAA55AA55)

    with open(args.binary, 'wb') as f:
        f.write(header + data + footer)


if __name__ == '__main__':
    main()
