#!/usr/bin/env python3
#
# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

# This script will add specific headers and FW load message to tell
# intel_s1000 ROM bootloader to load the firmware
# Usage python3 ${ZEPHYR_BASE}/scripts/create_board_img.py
#                    -i in_file.bin -o out_file.bin -l zephyr_prebuilt.elf
#                    [-m|--no_sram] [-c|--no_l1cache] [-t|--no_tlb]
#                    [-x|--no_exec] [-s|--no_sha] [-k|--clk_sel]

import argparse
import os
import sys
import struct
from elftools.elf.elffile import ELFFile

help_text="""
The flash memory needs to have specific headers and fw load message to tell
the ROM bootloader to load firmware.

To run script use command:

create_board_img.py -i in_file.bin -o out_file.bin -l zephyr_prebuilt.elf
                    [-m|--no_sram] [-c|--no_l1cache] [-t|--no_tlb]
                    [-x|--no_exec] [-s|--no_sha] [-k|--clk_sel]
"""

FLASH_PART_TABLE_OFFSET	= (0x1000)
FLASH_SECTOR_SIZE	= (4 * 1024)
FLASH_MAGIC_WORD	= (0x30504353)

SRAM_SIZE		= (1024*1024*4)
MAX_FLASH_FILE_SIZE	= (1024*1024*2)
SIZEOF_IPC64		= (64)
ROM_CONTROL_MEMWRITE	= (0x11)
ROM_CONTROL_LOADFW	= (0x2)
ROM_CONTROL_EXEC	= (0x13)

FW_LOAD_NO_L1_CACHE	= (1 << 29)
FW_LOAD_NO_SRAM_FLAG	= (1 << 28)
FW_LOAD_NO_TLB_FLAG	= (1 << 27)
FW_LOAD_NO_EXEC_FLAG	= (1 << 26)
FW_LOAD_NO_SHA_FLAG	= (1 << 25)
FW_LOAD_CLK_SEL		= (1 << 21)

# File write buffer
flash_content = []
write_buf     = []

def debug(text):
    sys.stdout.write(os.path.basename(sys.argv[0]) + ": " + text + "\n")

def error(text):
    sys.exit(os.path.basename(sys.argv[0]) + ": " + text)

def set_magic_number(value):
    flash_content.append(value)

def set_partition_table_pointer(value):
    flash_content.append(value)

def ipc_load_fw(fw_size, fw_offset):
    dword_count = 3
    load_flags = 0
    clock_sel = 0

    dword_count = 0x3ff & dword_count

    debug("Creating flash image with following options:")

    if args.no_sram:
        load_flags |= FW_LOAD_NO_SRAM_FLAG
        debug("-m    no_sram")

    if args.no_l1cache:
        load_flags |= FW_LOAD_NO_L1_CACHE
        debug("-c    no_l1cache")

    if args.no_tlb:
        load_flags |= FW_LOAD_NO_TLB_FLAG
        debug("-t    no_tlb")

    if args.no_exec:
        load_flags |= FW_LOAD_NO_EXEC_FLAG
        debug("-x    no_exec")

    if args.no_sha:
        load_flags |= FW_LOAD_NO_SHA_FLAG
        debug("-s    no_sha")

    if args.clk_sel:
        clock_sel = FW_LOAD_CLK_SEL
        debug("-k    clk_sel")

    with open(args.kernel, "rb") as fp_kernel:
        kernel = ELFFile(fp_kernel)
        load_address = kernel.header['e_entry']
        debug("load address = 0x%X" % load_address)

    msg_header = 0x81000000 | ROM_CONTROL_LOADFW
    ext_header = 0x80000000 | dword_count | load_flags | clock_sel

    flash_content.append(msg_header)
    flash_content.append(ext_header)
    flash_content.append(load_address)
    flash_content.append(fw_offset)
    flash_content.append(fw_size)

def ipc_dbg_exec(address):
    dword_count = 1
    dword_count = 0x3ff & dword_count

    msg_header = 0x81000000 | ROM_CONTROL_EXEC
    ext_header = 0x0 | dword_count

    flash_content.append(msg_header)
    flash_content.append(ext_header)
    flash_content.append(address)

def ipc_dbg_memwrite(address, value):
    dword_count = 2
    dword_count = 0x3ff & dword_count

    msg_header = 0x81000000 | ROM_CONTROL_MEMWRITE
    ext_header = 0x0 | dword_count

    flash_content.append(msg_header)
    flash_content.append(ext_header)
    flash_content.append(address)
    flash_content.append(value)

def parse_args():
    global args
    parser = argparse.ArgumentParser(
        description=help_text,
        formatter_class=argparse.RawTextHelpFormatter)

    parser.add_argument("-i", "--in_file", required=True,
					help="Input FW Bin File")
    parser.add_argument("-o", "--out_file", required=True,
					help="Output Flash Bin File")
    parser.add_argument("-l", "--kernel", required=True,
                                        help="Zephyr kernel image")
    parser.add_argument("-m", "--no_sram", action="store_true",
					help="No SRAM")
    parser.add_argument("-c", "--no_l1cache", action="store_true",
					help="No L1 Cache")
    parser.add_argument("-t", "--no_tlb", action="store_true",
					help="No TLB")
    parser.add_argument("-x", "--no_exec", action="store_true",
					help="No Exec")
    parser.add_argument("-s", "--no_sha", action="store_true",
					help="No SHA")
    parser.add_argument("-k", "--clk_sel", action="store_true",
					help="Clock Select")

    args = parser.parse_args()

def main():
    global flash_content, write_buf
    parse_args()

    in_file_size = os.path.getsize(args.in_file)
    if in_file_size == 0:
        error("%s file has no content\n" % args.in_file)

    out_file_size = FLASH_PART_TABLE_OFFSET + in_file_size

    # round up the flash size to sector boundary
    zeropad_size = FLASH_SECTOR_SIZE - (out_file_size % FLASH_SECTOR_SIZE)
    out_file_size += zeropad_size
    if out_file_size > MAX_FLASH_FILE_SIZE:
        error("%s exceeds %d bytes\n" % (args.out_file, MAX_FLASH_FILE_SIZE))

    # pre-boot initialization commands
    set_magic_number(FLASH_MAGIC_WORD)
    set_partition_table_pointer(FLASH_PART_TABLE_OFFSET)
    ipc_dbg_memwrite(0x71d14, 0x0)
    ipc_dbg_memwrite(0x71d24, 0x0)
    ipc_dbg_memwrite(0x304628, 0xd)
    ipc_dbg_memwrite(0x71fd0, 0x3)

    # load the image at address FLASH_PART_TABLE_OFFSET in flash to SRAM
    # at load_address (determined by the entrypoint in the supplied elf)
    ipc_load_fw(SRAM_SIZE, FLASH_PART_TABLE_OFFSET)

    # pad zeros until FLASH_PART_TABLE_OFFSET
    num_zero_pad = FLASH_PART_TABLE_OFFSET // 4 - len(flash_content)
    flash_content += num_zero_pad * [0]

    # read contents of firmware input file and change the endianness
    with open(args.in_file, "rb") as in_fp:
        read_buf = in_fp.read()
        in_fp.close()
        for itr in range(int(in_file_size/4)):
            write_buf.append(read_buf[itr*4 + 3])
            write_buf.append(read_buf[itr*4 + 2])
            write_buf.append(read_buf[itr*4 + 1])
            write_buf.append(read_buf[itr*4 + 0])

        # pad zeros until the sector boundary
        write_buf += zeropad_size*[0]

    # Generate the file which should be downloaded to Flash
    with open(args.out_file, "wb") as out_fp:
        # write as a uint (4 bytes) with byte order swapped (big-endian)
        out_fp.write(struct.pack(">{}I".format(len(flash_content)),
                                                        *flash_content))

        # write as a byte
        out_fp.write(struct.pack("{}B".format(len(write_buf)),
                                                        *write_buf))

        out_fp.close()

    debug("Input %s = %ld bytes" % (os.path.basename(args.in_file), in_file_size))
    debug("Output %s = %ld bytes" % (os.path.basename(args.out_file), out_file_size))

if __name__ == "__main__":
    main()
