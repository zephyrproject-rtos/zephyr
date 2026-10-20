# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with the SparkFun Variable Loader (SVL) serial bootloader.'''

import os
import shutil
import sys

from runners.core import RunnerCaps, ZephyrBinaryRunner

# SVL always loads the next stage at this fixed address; it takes no address argument.
SVL_LOAD_ADDRESS = 0x10000

SVL_BAUD_RATES = (57600, 115200, 230400, 460800, 921600)
DEFAULT_BAUD = 115200


class SvlBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for the SparkFun Variable Loader svl.py tool.'''

    def __init__(self, cfg, svl, port, baud=DEFAULT_BAUD):
        super().__init__(cfg)
        self.svl = svl
        self.port = port
        self.baud = baud

    @classmethod
    def name(cls):
        return 'svl'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument(
            '--svl', default='svl.py', help='path to the SparkFun svl.py tool, default svl.py'
        )
        parser.add_argument('--port', required=True, help='serial port to the board')
        parser.add_argument(
            '--baud',
            type=int,
            default=DEFAULT_BAUD,
            choices=SVL_BAUD_RATES,
            help=f'upload baud rate, default {DEFAULT_BAUD}',
        )

    @classmethod
    def do_create(cls, cfg, args):
        return cls(cfg, args.svl, args.port, baud=args.baud)

    def do_run(self, command, **kwargs):
        self.ensure_output('bin')
        self.check_load_address()
        svl = shutil.which(self.svl) or self.svl
        if not os.path.isfile(svl):
            raise RuntimeError(
                f"SVL tool '{self.svl}' not found; pass its path with --svl or add it to your PATH"
            )
        cmd = [sys.executable, svl, self.port, '-b', str(self.baud), '-f', self.cfg.bin_file]
        self.check_call(cmd)

    def check_load_address(self) -> None:
        address = self.flash_address_from_build_conf(self.build_conf)
        if address != SVL_LOAD_ADDRESS:
            raise RuntimeError(
                f'image is linked at 0x{address:x}, but SVL loads at 0x{SVL_LOAD_ADDRESS:x}'
            )
