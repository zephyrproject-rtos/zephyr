#!/usr/bin/env python3

# Copyright 2019 The Chromium OS Authors. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Script to pack EC binary with manifest header.

Package ecos main FW binary (kernel) and AON task binary into final EC binary
image with a manifest header, ISH shim loader will parse this header and load
each binaries into right memory location.
"""

import argparse
import os
import struct

MANIFEST_ENTRY_SIZE = 0x80
HEADER_SIZE = 0x1000
HEADER_VER = 0x00010001
PAGE_SIZE = 0x1000
MAIN_FW_ADDR = 0xFF200000
SRAM_BASE_ADDR = MAIN_FW_ADDR
SRAM_BANK_SIZE = 0x00008000
SRAM_BIT_WIDTH = 64
SRAM_SIZE = 0x000A0000
AON_RF_ADDR = 0xFF800000
AON_RF_SIZE = 0x2000
KERN_LOAD_ADDR = MAIN_FW_ADDR
AON_LOAD_ADDR = AON_RF_ADDR


def parseargs():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument(
        "-k",
        "--kernel",
        help="EC kernel binary to pack, \
                        usually ec.RW.bin or ec.RW.flat.",
        required=True,
    )
    parser.add_argument(
        "-a",
        "--aon",
        help="EC aontask binary to pack, \
                        usually ish_aontask.bin.",
        required=False,
    )
    parser.add_argument("-o", "--output", help="Output flash binary file")

    parser.add_argument(
        "-v", "--version", help="Specify the version of the EC firmware", required=True
    )

    return parser.parse_args()


def gen_global_manifest():
    """Returns a binary blob that represents a manifest entry"""
    m = bytearray(MANIFEST_ENTRY_SIZE)

    args = parseargs()
    version_parts = args.version.split('.')
    if len(version_parts) != 4:
        raise ValueError("Version must be in format major.minor.patch.build")
    major, minor, patch, build = map(int, version_parts)

    # 4 bytes of ASCII encode ID (little endian)
    struct.pack_into('<4s', m, 0, b'ISHG')
    # 4 bytes of extension size, fixed 128 (little endian)
    struct.pack_into('<I', m, 4, MANIFEST_ENTRY_SIZE)
    # 4 bytes of header version, fixed (little endian)
    struct.pack_into('<I', m, 8, HEADER_VER)
    # 4 bytes of flags (little endian)
    struct.pack_into('<I', m, 12, 0)

    # 8 bytes of ISH build version (little endian)
    struct.pack_into('<I', m, 16, major)
    struct.pack_into('<I', m, 18, minor)
    struct.pack_into('<I', m, 20, patch)
    struct.pack_into('<I', m, 22, build)
    # 4 bytes of entry point
    struct.pack_into('<I', m, 24, MAIN_FW_ADDR)

    # 4 bytes of AON start physical address
    struct.pack_into('<I', m, 28, AON_RF_ADDR)
    # 4 bytes of AON RF limit
    struct.pack_into('<I', m, 32, AON_RF_SIZE)
    # 4 bytes of SRAM bank size
    struct.pack_into('<I', m, 36, SRAM_BANK_SIZE)
    # 4 bytes of SRAM bit width
    struct.pack_into('<I', m, 40, SRAM_BIT_WIDTH)
    # 4 bytes of SRAM base
    struct.pack_into('<I', m, 44, SRAM_BASE_ADDR)
    # 4 bytes of SRAM limit
    struct.pack_into('<I', m, 48, SRAM_SIZE)

    return m


def gen_manifest_end():
    """Returns a binary blob that represents a manifest entry"""
    m = bytearray(MANIFEST_ENTRY_SIZE)

    # 4 bytes of ASCII encode ID (little endian)
    struct.pack_into('<4s', m, 0, b'ISHE')

    return m


def gen_module_manifest(ext_id, comp_app_name, code_offset, module_size, load_addr):
    """Returns a binary blob that represents a manifest entry"""
    m = bytearray(MANIFEST_ENTRY_SIZE)

    # 4 bytes of ASCII encode ID (little endian)
    struct.pack_into('<4s', m, 0, ext_id)
    # 4 bytes of extension size, fixed 128 (little endian)
    struct.pack_into('<I', m, 4, MANIFEST_ENTRY_SIZE)
    # 8 bytes of ASCII module name (little endian)
    struct.pack_into('<8s', m, 8, comp_app_name)

    # 4 bytes of module offset (little endian)
    struct.pack_into('<I', m, 16, code_offset)
    # 4 bytes of module size (little endian)
    struct.pack_into('<I', m, 20, module_size)
    # 4 bytes of load offset (little endian)
    struct.pack_into('<I', m, 24, 0)
    # 4 bytes of load size (little endian)
    struct.pack_into('<I', m, 28, module_size)

    # 4 bytes of load address (little endian)
    struct.pack_into('<I', m, 32, load_addr)

    return m


def roundup_page(size):
    """Returns roundup-ed page size from size of bytes"""
    return int(size / PAGE_SIZE) + (size % PAGE_SIZE > 0)


def main():
    args = parseargs()
    print("    Packing EC image file for ISH")
    print("args.version", args.version)
    with open(args.output, 'wb') as f:
        kernel_size = os.path.getsize(args.kernel)

        if args.aon is not None:
            aon_size = os.path.getsize(args.aon)

        print("      kernel binary size:", kernel_size)
        kern_rdup_pg_size = roundup_page(kernel_size)

        # Global SOC configuration
        f.write(gen_global_manifest())

        # Add manifest for main ISH binary
        f.write(gen_module_manifest(b'ISHM', b'ISH_KERN', HEADER_SIZE, kernel_size, KERN_LOAD_ADDR))

        if args.aon is not None:
            print("      AON binary size:   ", aon_size)
            aon_rdup_pg_size = roundup_page(aon_size)
            # Add manifest for aontask binary
            f.write(
                gen_module_manifest(
                    b'ISHM',
                    b'AON_TASK',
                    (HEADER_SIZE + kern_rdup_pg_size * PAGE_SIZE),
                    aon_size,
                    AON_LOAD_ADDR,
                )
            )
        else:
            aon_rdup_pg_size = 0

        # ICCM DCCM placeholder
        f.write(
            gen_module_manifest(
                b'ISHM',
                b'ICCM_IMG',
                (HEADER_SIZE + (kern_rdup_pg_size + aon_rdup_pg_size) * PAGE_SIZE),
                0,
                0,
            )
        )
        f.write(
            gen_module_manifest(
                b'ISHM',
                b'DCCM_IMG',
                (HEADER_SIZE + (kern_rdup_pg_size + aon_rdup_pg_size) * PAGE_SIZE),
                0,
                0,
            )
        )

        # Add manifest that signals end of manifests
        # f.write(gen_manifest(b'ISHE', b'', 0, 0))
        f.write(gen_manifest_end())

        # Pad the remaining HEADER with 0s
        if args.aon is not None:
            f.write(b'\x00' * (HEADER_SIZE - (MANIFEST_ENTRY_SIZE * 6)))
        else:
            f.write(b'\x00' * (HEADER_SIZE - (MANIFEST_ENTRY_SIZE * 5)))

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
