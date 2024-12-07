# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with SPSDK.'''

import logging
import os
from pathlib import Path
import subprocess
from runners.core import ZephyrBinaryRunner, RunnerCaps

class SPSDKBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for SPSDK.'''
    def __init__(self, cfg, bootdevice=None, family=None, bootloader=None, flashbin=None, commander=None):
        super().__init__(cfg)
        self.file = cfg.file
        self.file_type = cfg.file_type
        self.hex_name = cfg.hex_file
        self.bin_name = cfg.bin_file
        self.elf_name = cfg.elf_file

        self.bootdevice = bootdevice
        self.family = family
        self.bootloader = bootloader
        self.flashbin = flashbin
        self.commander = commander

    @classmethod
    def name(cls):
        return 'spsdk'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--bootdevice', required=True,
                            help='boot device')
        parser.add_argument('--family', required=True,
                            help='family')
        parser.add_argument('--bootloader', required=True,
                            help='bootloader')
        parser.add_argument('--flashbin', required=True,
                            help='nxp container image flash.bin')
        parser.add_argument('--commander', default='nxpuuu',
                            help=f'''SPSDK Commander, default is
                            nxpuuu''')

    @classmethod
    def do_create(cls, cfg, args):
        return SPSDKBinaryRunner(cfg, bootdevice=args.bootdevice, family=args.family,
                                 bootloader=args.bootloader, flashbin=args.flashbin, commander=args.commander)

    def do_run(self, command, **kwargs):
        self.commander = os.fspath(
            Path(self.require(self.commander)).resolve())

        if command == 'flash':
            self.flash(**kwargs)

    def flash(self, **kwargs):

        if self.bootdevice == 'spl':
            cmd = [self.commander] + ['run'] + [f"SDPS[-t 10000]: boot -f {self.flashbin}"]
        else:
            cmd = ([self.commander] + ['write'] +
                   ['-b', f'{self.bootdevice}'] +
                   ['-f', f'{self.family}'] +
                   [f'{self.bootloader}'] +
                   [f'{self.flashbin}'])

        self.logger.info('Flashing file: {}'.format(self.flashbin))
        self.logger.info('Command: {}'.format(cmd))

        kwargs = {}
        if not self.logger.isEnabledFor(logging.DEBUG):
            kwargs['stdout'] = subprocess.DEVNULL

        self.check_call(cmd, **kwargs)
