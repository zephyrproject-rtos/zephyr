# Copyright (c) 2024 Chen Xingyu <hi@xingrz.me>
# SPDX-License-Identifier: Apache-2.0

'''SOPHGO CVI specific flash only runner.'''

import sys
import time
from os import path

from runners.core import RunnerCaps, ZephyrBinaryRunner


class CviFiptoolBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for the SOPHGO CVI boards, using fiptool.py from SDK.'''

    def __init__(self, cfg, fiptool_py, fip_bin):
        super().__init__(cfg)
        self.fiptool_py = fiptool_py
        self.fip_bin = fip_bin
        self.bin_name = cfg.bin_file

    @classmethod
    def name(cls):
        return 'cvi-fiptool'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--fiptool', required=True, help='path to fiptool.py script from SDK')
        parser.add_argument('--fip-bin', required=True, help='path to fip.bin file to be updated')

    @classmethod
    def do_create(cls, cfg, args):
        return CviFiptoolBinaryRunner(cfg, args.fiptool, args.fip_bin)

    def do_run(self, command, **kwargs):
        self.require(self.fiptool_py)

        fip_dir = path.dirname(self.fip_bin)
        if not path.exists(fip_dir):
            print(f'Waiting for {fip_dir} to be present...')
            while not path.exists(fip_dir):
                time.sleep(1)

        if not path.exists(self.fip_bin):
            raise RuntimeError(f'File {self.fip_bin} not found')

        cmd_flash = [sys.executable, self.fiptool_py]

        cmd_flash.extend(['-v', 'genfip', self.fip_bin])
        cmd_flash.extend(['--OLD_FIP', self.fip_bin])
        cmd_flash.extend(['--BLCP_2ND', self.bin_name])

        self.check_call(cmd_flash)

        print('')
        print('fip.bin updated. Remember to eject the drive before unplugging.')
