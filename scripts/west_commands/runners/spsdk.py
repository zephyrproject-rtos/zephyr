# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with SPSDK.'''

import logging
import os
import subprocess
from pathlib import Path

from runners.core import RunnerCaps, ZephyrBinaryRunner


class SPSDKBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for SPSDK.'''

    def __init__(
        self,
        cfg,
        dev_id,
        bootdevice=None,
        family=None,
        bootloader=None,
        flashbin=None,
        containers=None,
        commander=None,
    ):
        super().__init__(cfg)
        self.dev_id = dev_id
        self.file = cfg.file
        self.file_type = cfg.file_type
        self.hex_name = cfg.hex_file
        self.bin_name = cfg.bin_file
        self.elf_name = cfg.elf_file

        self.bootdevice = bootdevice
        self.family = family
        self.bootloader = bootloader
        self.flashbin = flashbin
        self.containers = containers
        self.commander = commander

    @classmethod
    def name(cls):
        return 'spsdk'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, dev_id=True)

    @classmethod
    def dev_id_help(cls) -> str:
        return 'SPSDK serial number for the board.'

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--bootdevice', default='spl', help='boot device')
        parser.add_argument('--family', required=True, help='family')
        parser.add_argument('--bootloader', required=True, help='bootloader')
        parser.add_argument('--flashbin', required=True, help='nxp container image flash.bin')
        parser.add_argument(
            '--containers', required=True, help='container count in flash.bin: one or two'
        )
        parser.add_argument(
            '--commander',
            default='nxpuuu',
            help="SPSDK Commander, default is nxpuuu",
        )

    @classmethod
    def do_create(cls, cfg, args):
        return SPSDKBinaryRunner(
            cfg,
            bootdevice=args.bootdevice,
            family=args.family,
            bootloader=args.bootloader,
            flashbin=args.flashbin,
            containers=args.containers,
            commander=args.commander,
            dev_id=args.dev_id,
        )

    def do_run(self, command, **kwargs):
        self.commander = os.fspath(Path(self.require(self.commander)).resolve())

        if command == 'flash':
            self.flash(**kwargs)

    def flash(self, **kwargs):
        self.logger.info(f"Flashing file: {self.flashbin}")

        kwargs = {}
        if not self.logger.isEnabledFor(logging.DEBUG):
            kwargs['stdout'] = subprocess.DEVNULL

        if self.dev_id:
            cmd_with_id = [self.commander] + [(f'-us={self.dev_id}' if self.dev_id else '')]
        else:
            cmd_with_id = [self.commander]
        if self.bootdevice == 'spl':
            if self.containers == 'one':
                cmd = cmd_with_id + ['run'] + [f"SDPS[-t 10000]: boot -f {self.flashbin}"]
                self.logger.info(f"Command: {cmd}")
                self.check_call(cmd, **kwargs)
            elif self.containers == 'two':
                cmd = cmd_with_id + ['run'] + [f"SDPS[-t 10000]: boot -f {self.flashbin}"]
                self.logger.info(f"Command: {cmd}")
                self.check_call(cmd, **kwargs)
                cmd = cmd_with_id + ['run'] + ["SDPV: delay 1000"]
                self.logger.info(f"Command: {cmd}")
                self.check_call(cmd, **kwargs)
                cmd = cmd_with_id + ['run'] + [f"SDPV: write -f {self.flashbin} -skipspl"]
                self.logger.info(f"Command: {cmd}")
                self.check_call(cmd, **kwargs)
                cmd = cmd_with_id + ['run'] + ["SDPV: jump"]
                self.logger.info(f"Command: {cmd}")
                self.check_call(cmd, **kwargs)
            else:
                raise ValueError(f"Invalid containers count: {self.containers}")
        else:
            cmd = (
                [self.commander]
                + ['write']
                + ['-b', f'{self.bootdevice}']
                + ['-f', f'{self.family}']
                + [f'{self.bootloader}']
                + [f'{self.flashbin}']
            )
            self.logger.info(f"Command: {cmd}")
            self.check_call(cmd, **kwargs)
