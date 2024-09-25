#!/usr/bin/env python3
# -*- coding: utf-8 -*-"

# Copyright 2019 The Chromium OS Authors. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Script to pack EC binary with manifest header.

Package ecos main FW binary (kernel) and AON task binary into final EC binary
image with a manifest header, ISH shim loader will parse this header and load
each binaries into right memory location.
"""

import argparse
import struct
import os

MANIFEST_ENTRY_SIZE = 0x80
HEADER_SIZE = 0x1000
PAGE_SIZE = 0x1000

def parseargs():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("-k", "--kernel",
                        help="EC kernel binary to pack, \
                        usually ec.RW.bin or ec.RW.flat.",
                        required=True)
    parser.add_argument("-a", "--aon",
                        help="EC aontask binary to pack, \
                        usually ish_aontask.bin.",
                        required=False)
    parser.add_argument("-o", "--output",
                        help="Output flash binary file")

    return parser.parse_args()

def gen_manifest(ext_id, comp_app_name, code_offset, module_size):
    """Returns a binary blob that represents a manifest entry"""
    m = bytearray(MANIFEST_ENTRY_SIZE)

    # 4 bytes of ASCII encode ID (little endian)
    struct.pack_into('<4s', m, 0, ext_id)
    # 8 bytes of ASCII encode ID (little endian)
    struct.pack_into('<8s', m, 32, comp_app_name)
    # 4 bytes of code offset (little endian)
    struct.pack_into('<I', m, 96, code_offset)
    # 2 bytes of module in page size increments (little endian)
    struct.pack_into('<H', m, 100, module_size)

    return m

def roundup_page(size):
    """Returns roundup-ed page size from size of bytes"""
    return int(size / PAGE_SIZE) + (size % PAGE_SIZE > 0)

def main():
    args = parseargs()
    print("    Packing EC image file for ISH")

    with open(args.output, 'wb') as f:
        kernel_size = os.path.getsize(args.kernel)

        if args.aon is not None:
            aon_size = os.path.getsize(args.aon)

        print("      kernel binary size:", kernel_size)
        kern_rdup_pg_size = roundup_page(kernel_size)
        # Add manifest for main ISH binary
        f.write(gen_manifest(b'ISHM', b'ISH_KERN', HEADER_SIZE, kern_rdup_pg_size))

        if args.aon is not None:
            print("      AON binary size:   ", aon_size)
            aon_rdup_pg_size = roundup_page(aon_size)
            # Add manifest for aontask binary
            f.write(gen_manifest(b'ISHM', b'AON_TASK',
                                 (HEADER_SIZE + kern_rdup_pg_size * PAGE_SIZE -
                                  MANIFEST_ENTRY_SIZE), aon_rdup_pg_size))

        # Add manifest that signals end of manifests
        f.write(gen_manifest(b'ISHE', b'', 0, 0))

        # Pad the remaining HEADER with 0s
        if args.aon is not None:
            f.write(b'\x00' * (HEADER_SIZE - (MANIFEST_ENTRY_SIZE * 3)))
        else:
            f.write(b'\x00' * (HEADER_SIZE - (MANIFEST_ENTRY_SIZE * 2)))

        # Append original kernel image
        with open(args.kernel, 'rb') as in_file:
            f.write(in_file.read())
            # Filling padings due to size round up as pages
            f.write(b'\x00' * (kern_rdup_pg_size * PAGE_SIZE - kernel_size))

            if args.aon is not None:
                # Append original aon image
                with open(args.aon, 'rb') as in_file:
                    f.write(in_file.read())
                # Filling padings due to size round up as pages
                f.write(b'\x00' * (aon_rdup_pg_size * PAGE_SIZE - aon_size))

if __name__ == '__main__':
    main()
