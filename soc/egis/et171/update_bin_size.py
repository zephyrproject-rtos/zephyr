#!/usr/bin/env python3

# Copyright 2025 Egis Technology Inc.
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import struct


def pad_to_multiple_of_32(size):
    return ((size + 31) // 32) * 32


def update_file_size_at_offset(file_path):
    original_size = os.path.getsize(file_path)
    padded_size = pad_to_multiple_of_32(original_size)

    with open(file_path, 'r+b') as f:
        if original_size < padded_size:
            f.seek(original_size)
            f.write(b'\x00' * (padded_size - original_size))
        f.seek(0x10)
        f.write(struct.pack('<I', padded_size))

    print(f"Original size: {original_size} bytes")
    print(f"Extended & written size to offset 0x10: {padded_size} bytes")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Pad file size to multiple of 32 and write to offset 0x0c.", allow_abbrev=False
    )
    parser.add_argument("file", help="Target binary file path")
    args = parser.parse_args()

    update_file_size_at_offset(args.file)
