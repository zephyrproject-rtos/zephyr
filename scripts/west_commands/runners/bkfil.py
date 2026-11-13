# Copyright (c) 2026 Beken Corporation
# SPDX-License-Identifier: Apache-2.0

'''Flash a Beken BK7258 through the BootROM download protocol.

The protocol itself lives in soc/beken/bk7258/bkfil.py, which needs pyserial
but nothing else; this runner only passes the board's port and the built image
to it.
'''

import os
import sys

from runners.core import RunnerCaps, ZephyrBinaryRunner

DEFAULT_BAUD = 1000000


class BkfilBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for the BK7258 BootROM download protocol.'''

    def __init__(self, cfg, port, baud, offset, script):
        super().__init__(cfg)
        self.port = port
        self.baud = baud
        self.offset = offset
        self.script = script

    @classmethod
    def name(cls):
        return 'bkfil'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument(
            '--port',
            help='serial port of the board, e.g. /dev/ttyUSB0 or COM17',
        )
        parser.add_argument(
            '--baud',
            type=int,
            default=DEFAULT_BAUD,
            help=f'download speed, default {DEFAULT_BAUD}',
        )
        parser.add_argument(
            '--offset',
            type=lambda text: int(text, 0),
            default=0,
            help='flash offset of the image, default 0 for a complete image',
        )
        parser.add_argument(
            '--bkfil-script',
            help='path to bkfil.py, by default the one in this repository',
        )

    @classmethod
    def do_create(cls, cfg, args):
        return BkfilBinaryRunner(cfg, args.port, args.baud, args.offset, args.bkfil_script)

    def do_run(self, command, **kwargs):
        if command != 'flash':
            raise ValueError(f'only flash is supported, not {command}')
        if not self.port:
            raise ValueError('--port is required for the bkfil runner')

        script = self.script or os.path.join(
            os.path.dirname(os.path.realpath(__file__)),
            '..',
            '..',
            '..',
            'soc',
            'beken',
            'bk7258',
            'bkfil.py',
        )
        self.check_call(
            [
                sys.executable,
                script,
                '--port',
                self.port,
                'write',
                '--baud',
                str(self.baud),
                '--offset',
                str(self.offset),
                self.cfg.bin_file,
            ]
        )
