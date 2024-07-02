#
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
# Author: Dylan Hsieh <dylan.hsieh@realtek.com>

import click
import struct
import os

IMAGE_MAGIC = 0x524C544B    # ASCII 'RLTK'
IMAGE_HDR_SIZE = 32
RAM_LOAD = 0x0000020

class BasedIntParamType(click.ParamType):
    name = 'integer'

    def convert(self, value, param, ctx):
        try:
            return int(value, 0)
        except ValueError:
            self.fail('%s is not a valid integer. Please use code literals '
                      'prefixed with 0b/0B, 0o/0O, or 0x/0X as necessary.'
                      % value, param, ctx)

@click.command()
@click.option('-L', '--load-addr', type=BasedIntParamType(), required=False,
              help='Load address for image when it should run from RAM.')
@click.option('--spi-freq', type=click.Choice(['0', '1', '2', '3']),
              required=False, default='3',
              help='Specify the frequency of SPI I/F.')
@click.option('--spi-mode', type=click.Choice(['0', '1', '2', '3']),
              required=False, default='0',
              help='Specify the mode of SPI I/F.')
@click.option('--spi-rdcmd', type=click.Choice(['0x03', '0x0B', '0x3B', '0x6B']),
              required=False, default='0x03', 
              help='Specify the command for flash read.')
@click.argument('infile')
@click.argument('outfile')

def img_gen(load_addr, spi_freq, spi_mode, spi_rdcmd, outfile, infile):
    img_size = os.path.getsize(infile)
    payload = bytearray(0)
    img_size = os.path.getsize(infile)

    print(f'+-------------------------------+');

    print(f'|    RTS5912 Image Generator    |');
    print(f'+-------------------------------+');
 
    print(f'Input = {infile}, size = {img_size}')
    print(f'Output = {outfile}')
    print(f'+-------------------------------+\r\n');
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
        0 + (int(spi_freq) << 2)+ (int(spi_mode)),
        int(spi_rdcmd, 0),
        0)
    
    payload[:len(header)] = header

    file = open(outfile, "wb")
    file.write(payload)
    file.flush()
    file.close()

    with open(outfile, "ab") as outFile, open(infile, "rb") as inFile:
        outFile.write(inFile.read())
        outFile.flush()
        outFile.close()
        inFile.close()

if __name__ == '__main__':
    img_gen()
