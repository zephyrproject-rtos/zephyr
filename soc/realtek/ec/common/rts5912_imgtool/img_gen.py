# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
# Author: Dylan Hsieh <dylan.hsieh@realtek.com>

"""
The RTS5912 specific image header shows the bootROM how to
load the image from flash to internal SRAM, this script obtains
the header to original BIN and output a new BIN file.
"""

import struct
import os
import click

IMAGE_MAGIC = 0x524C544B    # ASCII 'RLTK'
IMAGE_HDR_SIZE = 32
RAM_LOAD = 0x0000020

class BasedIntParamType(click.ParamType):
    """
    Class converting the string to integer with literals prefixed
    with bin/oct/hex format
    """
    name = 'integer'
    def convert(self, value, param, ctx):
        try:
            return int(value, 0)
        except ValueError:
            self.fail('Please use code literals prefixed with 0b/0B, 0o/0O, or 0x/0X as necessary.')
        return None

@click.command()
@click.option('-L', '--load-addr', type=BasedIntParamType(), required=False,
              help='Load address for image when it should run from RAM.')
@click.option('--spi-freq', type=click.Choice(['0', '1', '2', '3']),
              required=False, default='3',
              help='Specify the frequency of SPI I/F.')
@click.option('--spi-rdcmd', type=click.Choice(['0x03', '0x0B', '0x3B', '0x6B']),
              required=False, default='0x03',
              help='Specify the command for flash read.')
@click.argument('original_bin')
@click.argument('signed_bin')

def img_gen(load_addr, spi_freq, spi_rdcmd, signed_bin, original_bin):
    """
    To obtain the RTS5912 image header and output a new BIN file
    """
    img_size = os.path.getsize(original_bin)
    payload = bytearray(0)
    img_size = os.path.getsize(original_bin)

    print(f'Load Address = {hex(load_addr)}')
    print(f'SPI Frequency = {int(spi_freq)}')
    print(f'Input = {original_bin}, size = {img_size}')
    print(f'Output = {signed_bin}')

    fmt = ('<' +
        # type ImageHdr struct {
        'I' +     # Magic    uint32
        'I' +     # LoadAddr uint32
        'H' +     # HdrSz    uint16
        'H' +     # reserved uint16
        'I' +     # ImgSz    uint32
        'I' +     # Flags    uint32
        'I' +     # reserved uint32
        'I' +     # reserved uint32
        'B' +     # SpiFmt   uint8
        'B' +     # SpiRdCmd uint8
        'H'       # reserved uint16
           )      # }

    header = struct.pack(fmt,
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
        0)

    payload[:len(header)] = header

    with open(signed_bin, "wb") as signed:
        signed.write(payload)
        signed.flush()
        signed.close()

    with open(signed_bin, "ab") as signed, open(original_bin, "rb") as original:
        signed.write(original.read())
        signed.flush()
        signed.close()
        original.close()

if __name__ == '__main__':
    # @click.command edits the function arguments but pylint doesn't know it,
    # disable pylint in next line to avoid warning.

    # pylint: disable-next=no-value-for-parameter
    img_gen()
