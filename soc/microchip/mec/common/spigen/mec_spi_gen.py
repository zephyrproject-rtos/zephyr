#!/usr/bin/env python3

# Copyright (c) 2022 Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

import sys
import argparse
import hashlib

verbose_mode = False

# Header parameters
HDR_SIZE = 0x140
HDR_VER_MEC172X = 0x03
HDR_VER_MEC152X = 0x02
HDR_SPI_CLK_12MHZ = 0x3
HDR_SPI_CLK_16MHZ = 0x2
HDR_SPI_CLK_24MHZ = 0x1
HDR_SPI_CLK_48MHZ = 0
HDR_SPI_DRV_STR_1X = 0
HDR_SPI_DRV_STR_2X = 0x4
HDR_SPI_DRV_STR_4X = 0x8
HDR_SPI_DRV_STR_6X = 0xc
HDR_SPI_SLEW_SLOW = 0
HDR_SPI_SLEW_FAST = 0x10
HDR_SPI_CPOL_LO = 0
HDR_SPI_CPOL_HI = 0x20
HDR_SPI_CHPHA_MOSI_EDGE_2 = 0
HDR_SPI_CHPHA_MOSI_EDGE_1 = 0x40
HDR_SPI_CHPHA_MISO_EDGE_1 = 0
HDR_SPI_CHPHA_MISO_EDGE_2 = 0x80

# User defined constants HDR_SPI_RD_ (0, 1, 2, 3) as per boot rom spec.
# 1st digit - number of I/O pins used to transmit the opcode
# 2nd digit - number of I/O pins used to transmit the SPI address
# 3rd digit - number of pins used to read data from flash
# 4th digit (if present) - dummy clocks between address and data phase
HDR_SPI_RD_111 = 0
HDR_SPI_RD_1118 = 1
HDR_SPI_RD_1128 = 2
HDR_SPI_RD_1148 = 3

# Payload parameters
PLD_LOAD_ADDR = 0xc0000
PLD_LOAD_ADDR_MEC172X = 0xc0000
PLD_LOAD_ADDR_MEC152X = 0xe0000
PLD_ENTRY_ADDR = 0
PLD_GRANULARITY = 128
PLD_PAD_SIZE = 128
PLD_PAD_BYTE = b'\xff'

MCHP_CHAR_P = 0x50
MCHP_CHAR_H = 0x48
MCHP_CHAR_C = 0x43
MCHP_CHAR_M = 0x4D

EC_INFO_BLOCK_SIZE = 128
ENCR_KEY_HDR_SIZE = 128
COSIG_SIZE = 96
TRAILER_SIZE = 160
TRAILER_PAD_BYTE = b'\xff'

TAG_SPI_LOC = 0
HDR_SPI_LOC = 0x100
PLD_SPI_LOC = 0x1000

CRC_TABLE = [0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15,
             0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d]

CHIP_DICT = {
    'mec15xx': { 'sram_base': 0xe0000, 'sram_size': 0x40000, 'header_ver': 2 },
    'mec172x': { 'sram_base': 0xc0000, 'sram_size': 0x68000, 'header_ver': 3 },
}

CHIP_DEFAULT = 'mec172x'
SPI_READ_MODE_DEFAULT = 'fast'
SPI_FREQ_MHZ_DEFAULT = 12
SPI_MODE_DEFAULT = 0
SPI_MODE_MIN = 0
SPI_MODE_MAX = 7
SPI_DRIVE_STRENGTH_MULT_DEFAULT = "1x"
SPI_SLEW_RATE_DEFAULT = "slow"

def print_bytes(title, b):
    """Print bytes or bytearray as hex values"""
    print("{0} = {{ ".format(title), end='')
    count = 1
    for v in b:
        print("0x{0:02x}, ".format(v), end='')
        if (count % 8) == 0:
            print("")
        count = count + 1

    print("}")

def crc8(crc, data):
    """Update CRC8 value.

    CRC8-ITU calculation
    """
    for v in data:
        crc = ((crc << 4) & 0xff) ^ (CRC_TABLE[(crc >> 4) ^ (v >> 4)])
        crc = ((crc << 4) & 0xff) ^ (CRC_TABLE[(crc >> 4) ^ (v & 0xf)])
    return crc ^ 0x55

def build_tag(hdr_spi_loc):
    """Build MEC172x Boot-ROM TAG

    MEC172x Boot-ROM TAG is 4 bytes
    bits[23:0] = bits[31:8] of the Header SPI address
    Header location must be a mutliple of 256 bytes
    bits[31:24] = CRC8-ITU of bits[23:0]
    return immutable bytes type
    """
    tag = bytearray([(hdr_spi_loc >> 8) & 0xff,
                   (hdr_spi_loc >> 16) & 0xff,
                   (hdr_spi_loc >> 24) & 0xff])
    tag.append(crc8(0, tag))

    return bytes(tag)

def build_header(chip, spi_config, hdr_spi_loc, pld_spi_loc, pld_entry_addr, pld_len):
    """Build MEC152x/MEC172x Boot-ROM SPI image header

    Args:
        chip: mec15xx or mec172x
        spi_config: spi configuration
        hdr_spi_loc: Header location in SPI Image
        pld_spi_loc: Payload(FW binary) location in SPI Image
        pld_entry_addr: Payload load address in MEC172x SPI SRAM
            Payload entry point address: index 0 instructs Boot-ROM to assume
            ARM vector table at beginning of payload and reset handler
            address is at offset 4 of payload.
        pld_len: Payload length, must be multiple of PLD_GRANULARITY

    return: immutable bytes type for built header
    """
    hdr = bytearray(HDR_SIZE)

    hdr[0] = MCHP_CHAR_P
    hdr[1] = MCHP_CHAR_H
    hdr[2] = MCHP_CHAR_C
    hdr[3] = MCHP_CHAR_M

    hdr[4] = CHIP_DICT[chip]['header_ver'] & 0xff

    if spi_config['spi_freq_mhz'] == 48:
        hdr[5] = HDR_SPI_CLK_48MHZ
    elif spi_config['spi_freq_mhz'] == 24:
        hdr[5] = HDR_SPI_CLK_24MHZ
    elif spi_config['spi_freq_mhz'] == 16:
        hdr[5] = HDR_SPI_CLK_16MHZ
    else:
        hdr[5] = HDR_SPI_CLK_12MHZ

    if spi_config['spi_mode'] & 0x01:
        hdr[5] |= HDR_SPI_CPOL_HI
    if spi_config['spi_mode'] & 0x02:
        hdr[5] |= HDR_SPI_CHPHA_MOSI_EDGE_1
    if spi_config['spi_mode'] & 0x04:
        hdr[5] |= HDR_SPI_CHPHA_MISO_EDGE_2

    # translate 1x, 2x, 4x, 6x to 0, 1, 2, 3
    if spi_config['spi_drive_str'] == "6x":
        hdr[5] |= HDR_SPI_DRV_STR_6X
    elif spi_config['spi_drive_str'] == "4x":
        hdr[5] |= HDR_SPI_DRV_STR_4X
    elif spi_config['spi_drive_str'] == "2x":
        hdr[5] |= HDR_SPI_DRV_STR_2X
    else:
        hdr[5] |= HDR_SPI_DRV_STR_1X

    # translate "slow", "fast" to 0, 1
    if spi_config['spi_slew_rate'] == "fast":
        hdr[5] |= HDR_SPI_SLEW_FAST

    # MEC172x b[0]=0 do not allow 96MHz SPI clock
    hdr[6] = 0 # not using authentication or encryption

    if spi_config['spi_read_mode'] == 'quad':
        hdr[7] = HDR_SPI_RD_1148
    elif spi_config['spi_read_mode'] == 'dual':
        hdr[7] = HDR_SPI_RD_1128
    elif spi_config['spi_read_mode'] == 'normal':
        hdr[7] = HDR_SPI_RD_111
    else:
        hdr[7] = HDR_SPI_RD_1118

    # payload load address in SRAM
    pld_load_addr = CHIP_DICT[chip]['sram_base']
    hdr[8] = pld_load_addr & 0xff
    hdr[9] = (pld_load_addr >> 8) & 0xff
    hdr[0xA] = (pld_load_addr >> 16) & 0xff
    hdr[0xB] = (pld_load_addr >> 24) & 0xff

    # payload entry point address in SRAM
    hdr[0xC] = pld_entry_addr & 0xff
    hdr[0xD] = (pld_entry_addr >> 8) & 0xff
    hdr[0xE] = (pld_entry_addr >> 16) & 0xff
    hdr[0xF] = (pld_entry_addr >> 24) & 0xff

    # payload size (16-bit) in granularity units
    pld_units = pld_len // PLD_GRANULARITY
    hdr[0x10] = pld_units & 0xff
    hdr[0x11] = (pld_units >> 8) & 0xff
    # hdr[0x12:0x13] = 0 reserved

    # Unsigned offset from start of Header to start of FW Binary
    # FW binary(payload) must always be located after header
    pld_offset = pld_spi_loc - hdr_spi_loc
    hdr[0x14] = pld_offset & 0xff
    hdr[0x15] = (pld_offset >> 8) & 0xff
    hdr[0x16] = (pld_offset >> 16) & 0xff
    hdr[0x17] = (pld_offset >> 24) & 0xff

    # hdr[0x18] = 0 not using authentication
    # hdr[0x19] = 0 not adjusting SPI flash device drive strength
    # hdr[0x1A through 0x1F] = 0 reserved
    # hdr[0x20 through 0x27] = 0 not adjust SPI flash device drive strength
    # hdr[0x28 through 0x47] = 0 reserved
    # hdr[0x48 through 0x4F] = 0 reserved
    # hdr[0x50 through 0x7F] = ECDSA P-384 Public key x-component
    # hdr[0x80 through 0xAF] = ECDSA P-384 Public key y-component
    # hdr[0xB0 through 0xDF] = SHA-384 digest of hdr[0 through 0xAF] Always required
    # hdr[0xE0 through 0x10F] = ECDSA signature R-component of hdr[0 through 0xDF]
    # hdr[0x110 through 0x13F] = ECDSA signature S-component of hdr[0 through 0xDF]

    h = hashlib.sha384()
    h.update(hdr[0:0xB0])
    hdr_digest = h.digest()

    if verbose_mode:
        print_bytes("hdr_sha384_digest", hdr_digest)

    hdr[0xB0:0xE0] = hdr_digest

    return bytes(hdr)

def parse_args():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    # Use a lambda to handle base 10 or base 16 (hex) input
    parser.add_argument("-c",
                        type=str,
                        dest="chip",
                        choices = ["mec15xx", "mec172x"],
                        default="mec172x",
                        help="Chip name: mec172x(default) or mec15xx")
    parser.add_argument("-i",
                        type=str,
                        dest="infilename",
                        default="zephyr.bin",
                        help="Input firmware binary file path/name (default: %(default)s)")
    parser.add_argument("-o",
                        type=str,
                        dest="outfilename",
                        default="zephyr.mchp.bin",
                        help="Output SPI image file path/name (default: %(default)s)")
    parser.add_argument("-s",
                        type=int,
                        dest="spi_size_kb",
                        default=256,
                        help="SPI image size in kilobytes (default: %(default)s)")
    parser.add_argument("-e",
                        type=int,
                        dest="entry_point",
                        default=0,
                        help="FW entry point address Lookup in image (default: %(default)s)")
    parser.add_argument("-f",
                        type=int,
                        dest="spi_freq_mhz",
                        choices = [12, 16, 24, 48],
                        default=12,
                        help="SPI frequency: 12, 16, 24, or 48 MHz")
    parser.add_argument("-r",
                        type=str,
                        dest="spi_read_mode",
                        choices = ["normal", "fast", "dual", "quad"],
                        default="fast",
                        help="SPI read mode: normal, fast, dual or quad")
    parser.add_argument("-m",
                        type=int,
                        dest="spi_mode",
                        choices = [0, 1, 2, 3, 4, 5, 6, 7],
                        default=0,
                        help="SPI signalling mode 3-bit field: 0-7")
    parser.add_argument("--drvstr",
                        type=str,
                        dest="spi_drive_strength",
                        choices = ["1x", "2x", "4x", "6x"],
                        default="1x",
                        help="SPI pin driver strength multiplier encoded")
    parser.add_argument("--slewrate",
                        type=str,
                        dest="spi_slew_rate",
                        choices = ["slow", "fast"],
                        default="slow",
                        help="SPI pins slew rate")
    parser.add_argument("--fill",
                        dest="fill",
                        action='store_true',
                        help="Fill with 0xFF to flash size")
    parser.add_argument("-v",
                        dest="verbose",
                        action='store_true',
                        help="Enable messages to console")

    ret_args = parser.parse_args()

    return ret_args

def main():
    """MEC SPI Gen"""
    args = parse_args()

    verbose_mode = args.verbose

    if verbose_mode:
        print("Command line arguments/defaults")
        print("  chip = {0}".format(args.chip))
        print("  infilename = {0}".format(args.infilename))
        print("  outfilename = {0}".format(args.outfilename))
        print("  SPI size (kilobytes) = {0}".format(args.spi_size_kb))
        print("  Entry point address = {0}".format(args.entry_point))
        print("  SPI frequency MHz = {0}".format(args.spi_freq_mhz))
        print("  SPI Read Mode = {0}".format(args.spi_read_mode))
        print("  SPI Signalling Mode = {0}".format(args.spi_mode))
        print("  SPI drive strength = {0}".format(args.spi_drive_strength))
        print("  SPI slew rate fast = {0}".format(args.spi_slew_rate))
        print("  Verbose = {0}".format(args.verbose))

    if args.infilename is None:
        print("ERROR: Specify input binary file name with -i")
        sys.exit(-1)

    if args.outfilename is None:
        print("ERROR: Specify output binary file name with -o")
        sys.exit(-1)

    chip = args.chip
    spi_read_mode = args.spi_read_mode
    spi_freq_mhz = args.spi_freq_mhz
    spi_mode = args.spi_mode
    spi_drive_str_mult = args.spi_drive_strength
    spi_slew = args.spi_slew_rate

    spi_size = args.spi_size_kb * 1024

    indata = None
    with open(args.infilename, "rb") as fin:
        indata = fin.read()

    indata_len = len(indata)
    if verbose_mode:
        print("Read input FW binary: length = {0}".format(indata_len))

    # if necessary pad input data to PLD_GRANULARITY required by Boot-ROM loader
    pad_len = 0
    if (indata_len % PLD_GRANULARITY) != 0:
        pad_len = PLD_GRANULARITY - (indata_len % PLD_GRANULARITY)
        # NOTE: MCHP Production SPI Image Gen. pads with 0
        padding = PLD_PAD_BYTE * pad_len
        indata = indata + padding

    indata_len += pad_len

    if verbose_mode:
        print("Padded FW binary: length = {0}".format(indata_len))

    # Do we have enough space for 4KB block containing TAG and Header, padded FW binary,
    # EC Info Block, Co-Sig Block, and Trailer?
    mec_add_info_size = PLD_SPI_LOC + EC_INFO_BLOCK_SIZE + COSIG_SIZE + TRAILER_SIZE
    if indata_len > (spi_size - mec_add_info_size):
        print("ERROR: FW binary exceeds flash size! indata_len = {0} spi_size = {1}".format(indata_len, spi_size))
        sys.exit(-1)

    entry_point = args.entry_point
    if args.entry_point == 0:
        # Look up entry point in image
        # Assumes Cortex-M4 vector table
        # at beginning of image and second
        # word in table is address of reset handler
        entry_point = int.from_bytes(indata[4:8], byteorder="little")

    tag = build_tag(HDR_SPI_LOC)

    if verbose_mode:
        print_bytes("TAG", tag)
        print("Build Header at {0}: Load Address = 0x{1:0x} Entry Point Address = 0x{2:0x}".format(
            HDR_SPI_LOC, PLD_LOAD_ADDR, entry_point))

    spi_config_info = {
            "spi_freq_mhz": spi_freq_mhz,
            "spi_mode": spi_mode,
            "spi_read_mode": spi_read_mode,
            "spi_drive_str": spi_drive_str_mult,
            "spi_slew_rate": spi_slew,
    }

    header = build_header(chip, spi_config_info, HDR_SPI_LOC, PLD_SPI_LOC, entry_point, indata_len)

    if verbose_mode:
        print_bytes("HEADER", header)
        print("")

    # appended to end of padded payload
    ec_info_block = bytearray(EC_INFO_BLOCK_SIZE)
    ec_info_loc = PLD_SPI_LOC + len(indata)

    # appended to end of (padded payload + ec_info_block)
    cosig = bytearray(b'\xff' * COSIG_SIZE)
    cosig_loc = ec_info_loc + EC_INFO_BLOCK_SIZE

    # appended to end of (padded payload + ec_info_block + cosig)
    # trailer[0:0x30] = SHA384(indata || ec_info_block || cosig)
    # trailer[0x30:] = 0xFF
    trailer = bytearray(b'\xff' * TRAILER_SIZE)
    trailer_loc = cosig_loc + COSIG_SIZE

    h = hashlib.sha384()
    h.update(indata)
    h.update(ec_info_block)
    h.update(cosig)
    image_digest = h.digest()
    trailer[0:len(image_digest)] = image_digest

    if verbose_mode:
        print("SHA-384 digest (paddedFW || ec_info_block || cosig)")
        print_bytes("digest", image_digest)

    spi_bufs = []
    spi_bufs.append(("TAG", TAG_SPI_LOC, tag))
    spi_bufs.append(("HEADER", HDR_SPI_LOC, header))
    spi_bufs.append(("PAYLOAD", PLD_SPI_LOC, indata))
    spi_bufs.append(("EC_INFO", ec_info_loc, ec_info_block))
    spi_bufs.append(("COSIG", cosig_loc, cosig))
    spi_bufs.append(("TRAILER", trailer_loc, trailer))

    spi_bufs.sort(key=lambda x: x[1])

    if verbose_mode:
        i = 0
        for sb in spi_bufs:
            print("buf[{0}]: {1} location=0x{2:0x} length=0x{3:0x}".format(i, sb[0], sb[1], len(sb[2])))
        print("")

    fill = bytes(b'\xff' * 256)
    if verbose_mode:
        print("len(fill) = {0}".format(len(fill)))

    loc = 0
    with open(args.outfilename, "wb") as fout:
        for sb in spi_bufs:
            if verbose_mode:
                print("sb: {0} location=0x{1:0x} len=0x{2:0x}".format(sb[0], sb[1], len(sb[2])))
            if loc < sb[1]:
                fill_len = sb[1] - loc
                if verbose_mode:
                    print("loc = 0x{0:0x}: Fill with 0xFF len=0x{1:0x}".format(loc, fill_len))
                nfill = fill_len // 256
                rem = fill_len % 256
                for _ in range(nfill):
                    fout.write(fill)
                if rem > 0:
                    fout.write(fill[0:rem])
                loc = loc + fill_len
            if verbose_mode:
                print("loc = 0x{0:0x}: write {1} len=0x{2:0x}".format(loc, sb[0], len(sb[2])))
            fout.write(sb[2])
            loc = loc + len(sb[2])
        if args.fill and (loc < spi_size):
            fill_len = spi_size - loc
            nfill = fill_len // 256
            rem = fill_len % 256
            for _ in range(nfill):
                fout.write(fill)
            if rem > 0:
                fout.write(fill[0:rem])
            loc = loc + fill_len
        if verbose_mode:
            print("Final loc = 0x{0:0x}".format(loc))

    if verbose_mode:
        print("MEC SPI Gen done")

if __name__ == '__main__':
    main()
