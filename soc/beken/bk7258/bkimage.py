#!/usr/bin/env python3
#
# Copyright (c) 2026 Beken Corporation
#
# SPDX-License-Identifier: Apache-2.0

'''Prepare BK7258 flash images.

Flash holds an interleaved stream: every 32 bytes of image data are followed
by the CRC-16 of those bytes, most significant byte first, and a partial last
block is padded with 0xff before its CRC is computed. Nothing on the chip
undoes that when it fetches instructions, so an image has to be converted
before it can be written -- including the first stage, which the BootROM reads
through the same window.

    crc     interleave one flat binary
    merge   build the complete flash image from the first stage and the
            application, which are linked independently and live at different
            flash offsets

The result of "crc" is written at the flash offset the binary was linked for.
The result of "merge" can be written at offset 0 in one go. 4 KB is both the
flash sector size and the BootROM protocol's sector size, so merge pads to it.

Examples:

    bkimage.py crc zephyr.bin zephyr.crc.bin
    bkimage.py merge --first-stage first.crc.bin --app zephyr.crc.bin \\
        --out zephyr_full.bin
'''

import argparse
import sys

BLOCK = 32
SECTOR = 4096

# Flash offset of the application slot, matching flash0 in the board dts. The
# boot address in the first stage is the XIP alias of this offset.
APP_OFFSET = 0x165000


def crc16(data):
    '''CRC-16 with polynomial 0x8005 and initial value 0xffff, MSB first.'''
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x8005) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def interleave(image):
    '''Append the CRC of every 32-byte block, padding the last block with 0xff.'''
    out = bytearray()
    for offset in range(0, len(image), BLOCK):
        block = image[offset : offset + BLOCK]
        block += b'\xff' * (BLOCK - len(block))
        out += block
        out += crc16(block).to_bytes(2, 'big')
    return bytes(out)


def merge(first_stage, app, app_offset=APP_OFFSET):
    '''Place both images at their flash offsets, pad to a whole sector.'''
    if app_offset % BLOCK:
        raise ValueError(f'application offset {app_offset:#x} is not a multiple of {BLOCK}')
    if len(first_stage) > app_offset:
        raise ValueError(
            f'first stage is {len(first_stage)} bytes, it does not fit below {app_offset:#x}'
        )
    if not app:
        raise ValueError('application image is empty')
    image = bytearray(b'\xff' * app_offset)
    image[0 : len(first_stage)] = first_stage
    image[app_offset : app_offset + len(app)] = app
    image += b'\xff' * ((SECTOR - len(image) % SECTOR) % SECTOR)
    return bytes(image)


def read(path):
    with open(path, 'rb') as f:
        return f.read()


def write(path, data):
    with open(path, 'wb') as f:
        f.write(data)


def parse_offset(text):
    '''A flash offset from the command line, decimal or 0x hex.'''
    offset = int(text, 0)
    if offset < 0:
        raise argparse.ArgumentTypeError(f'negative offset {text}')
    return offset


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0], allow_abbrev=False)
    commands = parser.add_subparsers(dest='command', required=True)

    crc = commands.add_parser('crc', allow_abbrev=False, help='interleave one flat binary')
    crc.add_argument('input', help='flat binary, normally zephyr.bin')
    crc.add_argument('output', help='interleaved image to write at the same offset')

    mg = commands.add_parser(
        'merge', allow_abbrev=False, help='build the complete flash image, written at offset 0'
    )
    mg.add_argument('--first-stage', required=True, help='interleaved first stage')
    mg.add_argument('--app', required=True, help='interleaved application image')
    mg.add_argument(
        '--app-offset',
        type=parse_offset,
        default=APP_OFFSET,
        help=f'flash offset of the application slot, default {APP_OFFSET:#x}',
    )
    mg.add_argument('--out', required=True, help='complete image to write at offset 0')

    args = parser.parse_args(argv)

    try:
        if args.command == 'crc':
            write(args.output, interleave(read(args.input)))
        else:
            write(
                args.out,
                merge(read(args.first_stage), read(args.app), args.app_offset),
            )
    except (OSError, ValueError) as err:
        return str(err)
    return 0


if __name__ == '__main__':
    sys.exit(main())
