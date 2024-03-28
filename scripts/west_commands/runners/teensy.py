# Copyright (c) 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for teensy .'''

import os
from os import path

from runners.core import ZephyrBinaryRunner, RunnerCaps

class TeensyBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for teensy.'''

    def __init__(self, cfg, mcu,
                 teensy='teensy', erase=False):
        super().__init__(cfg)

        default = path.join(cfg.board_dir, 'support', 'teensy.yaml')
        if path.exists(default):
            self.teensyconfig = default
        else:
            self.teensyconfig = None

        self.mcu_args = ['--mcu', mcu]
        self.teensy = teensy
        self.hex_name = cfg.hex_file

    @classmethod
    def name(cls):
        return 'teensy'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'},
                          dev_id=True, flash_addr=True, erase=True,
                          tool_opt=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--mcu', required=True,
                            help='Teensy muc target')
        parser.add_argument('--teensy', default='teensy_loader_cli',
                            help='path to teensy cli tool, default is teensy_loader_cli')

    @classmethod
    def do_create(cls, cfg, args):
        ret = TeensyBinaryRunner(
            cfg, args.mcu,
            teensy=args.teensy,
            erase=args.erase)
        return ret

    def do_run(self, command, **kwargs):
        self.require(self.teensy)
        if command == 'flash':
            self.flash(**kwargs)

    def flash(self, **kwargs):
        if self.hex_name is not None and os.path.isfile(self.hex_name):
            fname = self.hex_name
        else:
            raise ValueError(
                'Cannot flash; no hex ({}) files found. '.format(self.hex_name))

        cmd = ([self.teensy] +
               ['-v'] +
               self.mcu_args +
               [fname])

        self.logger.info('Flashing file: {}'.format(fname))

        teensy_output = self.check_output(cmd)
        if "Booting" in teensy_output.decode():
            self.logger.info('Success')
        else:
            self.logger.error('Failure')
