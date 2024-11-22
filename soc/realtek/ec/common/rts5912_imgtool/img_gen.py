# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
# Author: Dylan Hsieh <dylan.hsieh@realtek.com>

"""
The RTS5912 specific image header shows the bootROM how to
load the image from flash to internal SRAM, this script obtains
the header to original BIN and output a new BIN file.
"""

import argparse
import os
import struct

IMAGE_MAGIC = 0x524C544B  # ASCII 'RLTK'
IMAGE_HDR_SIZE = 32
RAM_LOAD = 0x0000020
FREQ_50M = 0
FREQ_25M = 1
FREQ_12P5M = 2
FREQ_6P25M = 3
NORMAL_read = "0x03"
DUAL_read = "0x3B"
QUAD_read = "0x6B"


def parse_args():
    """
    Parsing the arguments
    """
    parser = argparse.ArgumentParser(allow_abbrev=False)

    parser.add_argument(
        "-L",
        "--load-addr",
        type=int,
        dest="load_addr",
        help="Load address for image when it should run from RAM.",
    )

    parser.add_argument(
        "-I",
        "--input",
        type=str,
        dest="original_bin",
        default="zephyr.bin",
        help="Input bin file path",
    )

    parser.add_argument(
        "-O",
        "--output",
        type=str,
        dest="signed_bin",
        default="zephyr.rts5912.bin",
        help="Output bin file path",
    )

    parser.add_argument(
        "-F",
        "--spi-freq",
        type=int,
        dest="spi_freq",
        choices=[FREQ_50M, FREQ_25M, FREQ_12P5M, FREQ_6P25M],
        default=FREQ_6P25M,
        help="Specify the frequency of SPI I/F.",
    )

    parser.add_argument(
        "-R",
        "--spi-rdcmd",
        type=str,
        dest="spi_rdcmd",
        choices=[NORMAL_read, DUAL_read, QUAD_read],
        default=NORMAL_read,
        help="Specify the command for flash read.",
    )

    parser.add_argument("-V", "--verbose", action="count", default=0, help="Verbose Output")

    ret_args = parser.parse_args()
    return ret_args


def img_gen(load_addr, spi_freq, spi_rdcmd, original_bin, signed_bin):
    """
    To obtain the RTS5912 image header and output a new BIN file
    """
    img_size = os.path.getsize(original_bin)
    payload = bytearray(0)
    img_size = os.path.getsize(original_bin)

    fmt = (
        "<"
        +
        # type ImageHdr struct {
        "I"  # Magic    uint32
        + "I"  # LoadAddr uint32
        + "H"  # HdrSz    uint16
        + "H"  # reserved uint16
        + "I"  # ImgSz    uint32
        + "I"  # Flags    uint32
        + "I"  # reserved uint32
        + "I"  # reserved uint32
        + "B"  # SpiFmt   uint8
        + "B"  # SpiRdCmd uint8
        + "H"  # reserved uint16
    )  # }

    header = struct.pack(
        fmt,
        IMAGE_MAGIC,
        load_addr,
        IMAGE_HDR_SIZE,
        0,
        img_size,
        RAM_LOAD,
        0,
        0,
        0 + (int(spi_freq) << 2),
        int(spi_rdcmd, 0),
        0,
    )

    payload[: len(header)] = header

    with open(signed_bin, "wb") as signed:
        signed.write(payload)
        signed.flush()
        signed.close()

    with open(signed_bin, "ab") as signed, open(original_bin, "rb") as original:
        signed.write(original.read())
        signed.flush()
        signed.close()
        original.close()


def main():
    """
    Image generateor tool entry point
    """
    args = parse_args()
    if args.verbose:
        print(f"  Input = {args.original_bin}")
        print(f"  Output = {args.signed_bin}")
        print(f"  Load Address = {hex(args.load_addr)}")
        print(f"  SPI Frequency = {args.spi_freq}")
        print(f"  SPI Read Command = {args.spi_rdcmd}")
    img_gen(
        args.load_addr,
        args.spi_freq,
        args.spi_rdcmd,
        args.original_bin,
        args.signed_bin,
    )


if __name__ == "__main__":
    main()
