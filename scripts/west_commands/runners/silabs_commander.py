# Copyright (c) 2023, Antmicro <www.antmicro.com>
#
# Based on J-Link runner
# Copyright (c) 2017 Linaro Limited.
# SPDX-License-Identifier: Apache-2.0

"""
Runner that implements flashing with SiLabs Simplicity Commander binary tool.
See SiLabs UG162: "Simplicity Commander Reference Guide" for more info.
"""

import os
import shlex

from runners.core import FileType, RunnerCaps, ZephyrBinaryRunner

DEFAULT_APP = 'commander'


class SiLabsCommanderBinaryRunner(ZephyrBinaryRunner):
    def __init__(self, cfg, device, dev_id, commander, dt_flash, erase, speed, tool_opt):
        super().__init__(cfg)
        self.file = cfg.file
        self.file_type = cfg.file_type
        self.hex_name = cfg.hex_file
        self.bin_name = cfg.bin_file
        self.elf_name = cfg.elf_file
        self.device = device
        self.dev_id = dev_id
        self.commander = commander
        self.dt_flash = dt_flash
        self.erase = erase
        self.speed = speed

        self.tool_opt = []
        for opts in [shlex.split(opt) for opt in tool_opt]:
            self.tool_opt += opts

    @classmethod
    def name(cls):
        return 'silabs_commander'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'},
                          dev_id=True, flash_addr=True, erase=True,
                          tool_opt=True, file=True)

    @classmethod
    def dev_id_help(cls) -> str:
        return '''Device identifier. Use it to select the J-Link Serial Number
                  of the device connected over USB.'''

    @classmethod
    def tool_opt_help(cls) -> str:
        return "Additional options for Simplicity Commander, e.g. '--noreset'"

    @classmethod
    def do_add_parser(cls, parser):
        # Required:
        parser.add_argument('--device', required=True,
                            help='device part number')

        # Optional:
        parser.add_argument('--commander', default=DEFAULT_APP,
                            help='path to Simplicity Commander executable')
        parser.add_argument('--speed', default=None,
                            help='JTAG/SWD speed to use')

    @classmethod
    def do_create(cls, cfg, args):
        return SiLabsCommanderBinaryRunner(
                cfg, args.device,
                dev_id=args.dev_id,
                commander=args.commander,
                dt_flash=args.dt_flash,
                erase=args.erase,
                speed=args.speed,
                tool_opt=args.tool_opt)

    def do_run(self, command, **kwargs):
        self.require(self.commander)

        opts = ['--device', self.device]
        if self.erase:
            opts.append('--masserase')
        if self.dev_id:
            opts.extend(['--serialno', self.dev_id])
        if self.speed is not None:
            opts.extend(['--speed', self.speed])

        # Get the build artifact to flash

        if self.dt_flash:
            flash_addr = self.flash_address_from_build_conf(self.build_conf)
        else:
            flash_addr = 0

        if self.file is not None:
            # use file provided by the user
            if not os.path.isfile(self.file):
                raise ValueError(f'Cannot flash; file ({self.file}) not found')

            flash_file = self.file

            if self.file_type == FileType.HEX:
                flash_args = [flash_file]
            elif self.file_type == FileType.BIN:
                flash_args = ['--binary', '--address', f'0x{flash_addr:x}', flash_file]
            else:
                raise ValueError('Cannot flash; this runner only supports hex and bin files')

        else:
            # use hex or bin file provided by the buildsystem, preferring .hex over .bin
            if self.hex_name is not None and os.path.isfile(self.hex_name):
                flash_file = self.hex_name
                flash_args = [flash_file]
            elif self.bin_name is not None and os.path.isfile(self.bin_name):
                flash_file = self.bin_name
                flash_args = ['--binary', '--address', f'0x{flash_addr:x}', flash_file]
            else:
                raise ValueError(f'Cannot flash; no hex ({self.hex_name}) or bin ({self.bin_name}) files found.')

        args = [self.commander, 'flash'] + opts + self.tool_opt + flash_args

        self.logger.info(f'Flashing file: {flash_file}')
        self.check_call(args)
