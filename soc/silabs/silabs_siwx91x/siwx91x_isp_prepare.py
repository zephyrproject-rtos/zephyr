#!/usr/bin/env python3
#
# Copyright (c) 2023 Antmicro
# Copyright (c) 2024 Silicon Laboratories Inc.
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import struct
import sys

import intelhex


# For reference:
#     width=32 poly=0xd95eaae5 init=0 refin=true refout=true xorout=0
def create_table() -> list[int]:
    crc_table = [0] * 256
    for b in range(256):
        register = b
        for _ in range(8):
            lsb = register & 1
            register >>= 1
            if lsb:
                # Reflected polynomial: 0xd95eaae5
                register ^= 0xA7557A9B
        crc_table[b] = register
    return crc_table


def calc_crc32(data: bytes) -> int:
    crc_table = create_table()
    register = 0
    for b in data:
        register = crc_table[(b ^ register) & 0xFF] ^ (register >> 8)
    return register


def calc_checksum(data: bytes | bytearray, size: int, prev_sum: int) -> int:
    # Truncate
    data = data[:size]
    # Zero-pad data to mul of 4 bytes
    nzeros = ((len(data) + 3) // 4 * 4) - len(data)
    data += b"\0" * nzeros
    # Reinterpret data as LE u32
    ints = list(x[0] for x in struct.iter_unpack("<I", data))
    # Sum
    chk = prev_sum + sum(ints)
    # Convert to u32 and account each overflow as 1"s complement addition
    chk = (chk & 0xFFFFFFFF) + (chk >> 32)
    chk = (~chk) & 0xFFFFFFFF
    return chk


def set_bits(x: int, off: int, size: int, field: int) -> int:
    field = int(field)
    mask = ((1 << size) - 1) << off
    x &= ~mask
    x |= (field << off) & mask
    return x


def get_bootload_entry(
    ctrl_len: int = 0,
    ctrl_reserved: int = 0,
    ctrl_spi_32bitmode: bool = False,
    ctrl_release_ta_softreset: bool = False,
    ctrl_start_from_rom_pc: bool = False,
    ctrl_spi_8bitmode: bool = False,
    ctrl_last_entry: bool = True,
    dest_addr: int = 0,
) -> bytes:
    # Format bootload_entry struct
    ctrl = 0
    ctrl = set_bits(ctrl, 0, 24, ctrl_len)
    ctrl = set_bits(ctrl, 24, 3, ctrl_reserved)
    ctrl = set_bits(ctrl, 27, 1, ctrl_spi_32bitmode)
    ctrl = set_bits(ctrl, 28, 1, ctrl_release_ta_softreset)
    ctrl = set_bits(ctrl, 29, 1, ctrl_start_from_rom_pc)
    ctrl = set_bits(ctrl, 30, 1, ctrl_spi_8bitmode)
    ctrl = set_bits(ctrl, 31, 1, ctrl_last_entry)
    return struct.pack("<II", ctrl, dest_addr)


def get_bootload_ds(offset: int, ivt_offset: int, fixed_pattern: int = 0x5AA5) -> bytes:
    ret = b""
    ret += int(fixed_pattern).to_bytes(2, "little")
    ret += int(offset).to_bytes(2, "little")
    ret += int(ivt_offset).to_bytes(4, "little")
    for i in range(7):
        ret += get_bootload_entry(ctrl_last_entry=i == 0)
    return ret


def get_fwupreq(flash_location: int, image_size: int) -> bytes:
    # Field values
    cflags = 1
    sha_type = 0
    magic_no = 0x900D900D
    fw_version = 0
    # Initially CRC value is set to 0, then the CRC is calculated on the
    # whole image (including fwupreq header), and injected here
    crc = 0
    mic = [0, 0, 0, 0]
    counter = 0
    rsvd = [0, 0, 0, 0, magic_no]
    # Format
    ret = b""
    ret += cflags.to_bytes(2, "little")
    ret += sha_type.to_bytes(2, "little")
    ret += magic_no.to_bytes(4, "little")
    ret += image_size.to_bytes(4, "little")
    ret += fw_version.to_bytes(4, "little")
    ret += flash_location.to_bytes(4, "little")
    ret += crc.to_bytes(4, "little")
    for x in mic:
        ret += x.to_bytes(4, "little")
    ret += counter.to_bytes(4, "little")
    for x in rsvd:
        ret += x.to_bytes(4, "little")
    return ret


def main():
    parser = argparse.ArgumentParser(
        description="Converts raw binary output from Zephyr into an ISP binary for Silabs SiWx91x",
        allow_abbrev=False,
    )
    parser.add_argument(
        "ifile",
        metavar="INPUT.BIN",
        help="Raw binary file to read",
        type=argparse.FileType("rb"),
    )
    parser.add_argument(
        "ofile",
        metavar="OUTPUT.BIN",
        help="ISP binary file to write",
        type=argparse.FileType("wb"),
    )
    parser.add_argument(
        "--load-addr",
        metavar="ADDRESS",
        help="Address at which the raw binary image begins in the memory",
        type=lambda x: int(x, 0),
        required=True,
    )
    parser.add_argument(
        "--out-hex",
        metavar="FILE.HEX",
        help="Generate Intel HEX output in addition to binary one",
        type=argparse.FileType("w", encoding="ascii"),
    )
    args = parser.parse_args()

    img = bytearray(args.ifile.read())

    # Calculate and inject checksum
    chk = calc_checksum(img, 236, 1)
    print(f"ROM checksum: 0x{chk:08x}", file=sys.stderr)
    img[236:240] = chk.to_bytes(4, "little")

    # Get bootloader header, pad to 4032 and glue it to the image payload
    bl = bytearray(get_bootload_ds(4032, args.load_addr))
    padding = bytearray(4032 - len(bl))
    img = bl + padding + img

    # Get fwupreq header and glue it to the bootloader payload
    fwupreq = bytearray(get_fwupreq(args.load_addr - 0x8001000, len(img)))
    img = fwupreq + img

    # Calculate and inject CRC
    crc = calc_crc32(img)
    print(f"Image CRC: 0x{crc:08x}", file=sys.stderr)
    img[20:24] = crc.to_bytes(4, "little")

    args.ofile.write(img)

    # If you want to compare this file with the .hex file generated by Zephyr,
    # You have to reformat the Zephyr output:
    #   import intelhex
    #   hx = intelhex.IntelHex()
    #   hx.fromfile("zephyr.hex", "hex")
    #   hx.write_hex_file("zephyr.out.hex", byte_count=32)
    if args.out_hex:
        hx = intelhex.IntelHex()
        # len(bl) + len(padding) + len(fwupreq) == 4096
        hx.frombytes(img, args.load_addr - 4096)
        hx.write_hex_file(args.out_hex, byte_count=32)


if __name__ == "__main__":
    main()
