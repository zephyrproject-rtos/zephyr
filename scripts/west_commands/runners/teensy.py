# Copyright (c) 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for teensy .'''

import os
import subprocess

from runners.core import ZephyrBinaryRunner

class TeensyBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for teensy.'''

    def __init__(self, cfg, mcu, teensy_loader):
        super().__init__(cfg)

        self.mcu_args = ['--mcu', mcu]
        self.teensy_loader = teensy_loader
        self.hex_name = cfg.hex_file

    @classmethod
    def name(cls):
        return 'teensy'

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--mcu', required=True,
                            help='Teensy mcu target')
        parser.add_argument('--teensy', default='teensy_loader_cli',
                            help='path to teensy cli tool, default is teensy_loader_cli')

    @classmethod
    def do_create(cls, cfg, args):
        ret = TeensyBinaryRunner(
            cfg, args.mcu,
            teensy_loader=args.teensy)
        return ret

    def do_run(self, command):
        self.require(self.teensy_loader)
        if command == 'flash':
            self.flash()

    def flash(self):
        if self.hex_name is not None and os.path.isfile(self.hex_name):
            fname = self.hex_name
        else:
            raise ValueError(
                'Cannot flash; no hex ({}) file found. '.format(self.hex_name))

        cmd = ([self.teensy_loader] +
               self.mcu_args +
               [fname])

        self.logger.info('Flashing file: {}'.format(fname))

        try:
            self.check_output(cmd)
            self.logger.info('Success')
        except subprocess.CalledProcessError as grepexc:
            self.logger.error("Failure %i" % grepexc.returncode)
