# Copyright (c) 2018 Roman Tataurov <diytronic@yandex.ru>
# Modified 2018 Tavish Naruka <tavishnaruka@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
'''Runner for flashing with Black Magic Probe.'''
# https://github.com/blacksphere/blackmagic/wiki

import signal
from pathlib import Path
import os

from runners.core import ZephyrBinaryRunner, RunnerCaps, FileType

class BlackMagicProbeRunner(ZephyrBinaryRunner):
    '''Runner front-end for Black Magic probe.'''

    def __init__(self, cfg, gdb_serial,
                 connect_srst=False,
                 dt_flash=True):
        super().__init__(cfg)
        self.gdb = [cfg.gdb] if cfg.gdb else None
        # as_posix() because gdb doesn't recognize backslashes as path
        # separators for the 'load' command we execute in bmp_flash().
        #
        # https://github.com/zephyrproject-rtos/zephyr/issues/50789
        self.file_type = cfg.file_type
        self.file = Path(cfg.file).as_posix() if (cfg.file is not None) else None
        self.hex_file = Path(cfg.hex_file).as_posix() if (cfg.hex_file is not None) else None
        self.bin_file = Path(cfg.bin_file).as_posix() if (cfg.bin_file is not None) else None
        self.elf_file = Path(cfg.elf_file).as_posix() if (cfg.elf_file is not None) else None
        self.dt_flash = dt_flash
        self.gdb_serial = gdb_serial
        if connect_srst:
            self.connect_srst_enable_arg = [
                    '-ex', "monitor connect_srst enable"]
            self.connect_srst_disable_arg = [
                    '-ex', "monitor connect_srst disable"]
        else:
            self.connect_srst_enable_arg = []
            self.connect_srst_disable_arg = []

    @classmethod
    def name(cls):
        return 'blackmagicprobe'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash', 'debug', 'attach'},
                          flash_addr=True, file=True)

    @classmethod
    def do_create(cls, cfg, args):
        return BlackMagicProbeRunner(cfg,
                                     gdb_serial=args.gdb_serial,
                                     connect_srst=args.connect_srst,
                                     dt_flash=args.dt_flash)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--gdb-serial', default='/dev/ttyACM0',
                            help='GDB serial port')
        parser.add_argument('--connect-srst', action='store_true',
                            help='Assert SRST during connect? (default: no)')

    def bmp_flash(self, command, **kwargs):

        if self.file is not None:
            # use file provided by the user
            if not os.path.isfile(self.file):
                err = 'Cannot flash; file ({}) not found'
                raise ValueError(err.format(self.file))
            flash_file = self.file
            if self.file_type in (FileType.HEX, FileType.ELF):
                flash_cmd = f'"{self.file}"'
            elif self.file_type == FileType.BIN:
                if self.dt_flash:
                    flash_addr = self.flash_address_from_build_conf(self.build_conf)
                else:
                    flash_addr = 0
                flash_cmd = f'"{self.file}" 0x{flash_addr:x}'
            else:
                err = f'Cannot flash; {self.name()} runner only supports elf, hex and bin files'
                raise ValueError(err)

        else:
            # use file provided by the buildsystem, preferring .hex over .bin over .elf
            if self.hex_file is not None and os.path.isfile(self.hex_file):
                flash_file = self.hex_file
                flash_cmd = f'"{flash_file}"'
            elif self.bin_file is not None and os.path.isfile(self.bin_file):
                if self.dt_flash:
                    flash_addr = self.flash_address_from_build_conf(self.build_conf)
                else:
                    flash_addr = 0
                flash_file = self.bin_file
                flash_cmd = f'"{flash_file}" 0x{flash_addr:x}'
            elif self.elf_file is not None and os.path.isfile(self.elf_file):
                flash_file = self.elf_file
                flash_cmd = f'"{flash_file}"'
            else:
                raise ValueError('Cannot debug; no file to flash')

        self.logger.info(f'Flashing file: {flash_file}')

        command = (self.gdb +
                   ['-ex', "set confirm off",
                    '-ex', "target extended-remote {}".format(
                        self.gdb_serial)] +
                    self.connect_srst_enable_arg +
                   ['-ex', "monitor swdp_scan",
                    '-ex', "attach 1",
                    '-ex', f'load {flash_cmd}',
                    '-ex', "kill",
                    '-ex', "quit",
                    '-silent'])
        self.check_call(command)

    def check_call_ignore_sigint(self, command):
        previous = signal.signal(signal.SIGINT, signal.SIG_IGN)
        try:
            self.check_call(command)
        finally:
            signal.signal(signal.SIGINT, previous)

    def bmp_attach(self, command, **kwargs):
        if self.file is not None:
            if self.file_type != FileType.ELF:
                raise ValueError('Cannot debug; elf file required')
            elf_name = self.file
        elif self.elf_file is not None:
            elf_name = self.elf_file
        else:
            elf_name = None

        command = (self.gdb +
                    ['-ex', "set confirm off",
                    '-ex', "target extended-remote {}".format(
                        self.gdb_serial)] +
                    self.connect_srst_disable_arg +
                    ['-ex', "monitor swdp_scan",
                    '-ex', "attach 1"])
        if elf_name is not None:
            command = command + ['-ex', "file {}".format(elf_name)]

        self.check_call_ignore_sigint(command)

    def bmp_debug(self, command, **kwargs):
        if self.file is not None:
            if self.file_type != FileType.ELF:
                raise ValueError('Cannot debug; elf file required')
            elf_name = self.file
        elif self.elf_file is None:
            raise ValueError('Cannot debug; elf file is missing')
        else:
            elf_name = self.elf_file

        command = (self.gdb +
                   ['-ex', "set confirm off",
                    '-ex', "target extended-remote {}".format(
                        self.gdb_serial)] +
                    self.connect_srst_enable_arg +
                   ['-ex', "monitor swdp_scan",
                    '-ex', "attach 1",
                    '-ex', "file {}".format(elf_name),
                    '-ex', "load {}".format(elf_name)])
        self.check_call_ignore_sigint(command)

    def do_run(self, command, **kwargs):
        if self.gdb is None:
            raise ValueError('Cannot execute; gdb not specified')
        self.require(self.gdb[0])

        if command == 'flash':
            self.bmp_flash(command, **kwargs)
        elif command == 'debug':
            self.bmp_debug(command, **kwargs)
        elif command == 'attach':
            self.bmp_attach(command, **kwargs)
        else:
            self.bmp_flash(command, **kwargs)
