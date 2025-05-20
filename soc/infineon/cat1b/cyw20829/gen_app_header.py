# Copyright (c) 2024 Cypress Semiconductor Corporation.
# SPDX-License-Identifier: Apache-2.0

import argparse
import ctypes
import sys
from pathlib import Path

from intelhex import bin2hex

# Const
TOC2_SIZE = 16
L1_APP_DESCR_SIZE = 28
L1_APP_DESCR_ADDR = 0x10
DEBUG_CERT_ADDR = 0x0
SERV_APP_DESCR_ADDR = 0x0

DEBUG = False


# Define the structures
class TOC2Data(ctypes.Structure):
    _fields_ = [
        ("toc2_size", ctypes.c_uint32),
        ("l1_app_descr_addr", ctypes.c_uint32),
        ("service_app_descr_addr", ctypes.c_uint32),
        ("debug_cert_addr", ctypes.c_uint32),
    ]


class L1Desc(ctypes.Structure):
    _fields_ = [
        ("l1_app_descr_size", ctypes.c_uint32),
        ("boot_strap_addr", ctypes.c_uint32),
        ("boot_strap_dst_addr", ctypes.c_uint32),
        ("boot_strap_size", ctypes.c_uint32),
        ("smif_crypto_cfg", ctypes.c_uint8 * 12),
        ("reserve", ctypes.c_uint8 * 4),
    ]


class SignHeader(ctypes.Structure):
    _fields_ = [
        ("reserved", ctypes.c_uint8 * 32),  # 32b for sign header
    ]


def generate_platform_headers(
    secure_lcs,
    output_path,
    project_name,
    bootstrap_size,
    bootstrap_dst_addr,
    flash_addr_offset,
    smif_config,
):
    ######################### Generate TOC2 #########################
    toc2_data = TOC2Data(
        toc2_size=TOC2_SIZE,
        l1_app_descr_addr=L1_APP_DESCR_ADDR,
        service_app_descr_addr=SERV_APP_DESCR_ADDR,
        debug_cert_addr=DEBUG_CERT_ADDR,
    )

    ###################### Generate L1_APP_DESCR ####################
    if secure_lcs:
        boot_strap_addr = 0x30  # Fix address for signed image
    else:
        boot_strap_addr = 0x50  # Fix address for un-signed image

    l1_desc = L1Desc(
        l1_app_descr_size=L1_APP_DESCR_SIZE,
        boot_strap_addr=boot_strap_addr,
        boot_strap_dst_addr=int(bootstrap_dst_addr, 16),
        boot_strap_size=int(bootstrap_size, 16),
    )

    if smif_config:
        with open(smif_config, 'rb') as binary_file:
            l1_desc.smif_crypto_cfg[0:] = binary_file.read()

    # Write the structure to a binary file
    with open(Path(output_path) / 'app_header.bin', 'wb') as f:
        f.write(bytearray(toc2_data))
        f.write(bytearray(l1_desc))

        if not secure_lcs:
            f.write(bytearray(SignHeader()))

    # Generate hex from bin
    sys.exit(
        bin2hex(
            Path(output_path) / 'app_header.bin',
            Path(output_path) / 'app_header.hex',
            int(flash_addr_offset, 16),
        )
    )


def main():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument(
        '-m',
        '--secure_lcs',
        required=False,
        type=bool,
        default=False,
        help='Use SECURE Life Cycle stage: True/False',
    )

    parser.add_argument('-p', '--project-path', required=True, help='path to application artifacts')
    parser.add_argument('-n', '--project-name', required=True, help='Application name')
    parser.add_argument('-k', '--keys', required=False, help='Path to keys')

    parser.add_argument('--bootstrap-size', required=False, help='Bootstrap size')
    parser.add_argument(
        '--bootstrap-dst-addr',
        required=False,
        help='Bootstrap destanation address. Should be in RAM (SAHB)',
    )

    parser.add_argument('--flash_addr_offset', required=False, help='Flash offset')

    parser.add_argument('-s', '--smif-config', required=False, help='smif config file')
    args = parser.parse_args()

    generate_platform_headers(
        args.secure_lcs,
        args.project_path,
        args.project_name,
        args.bootstrap_size,
        args.bootstrap_dst_addr,
        args.flash_addr_offset,
        args.smif_config,
    )


if __name__ == '__main__':
    main()
