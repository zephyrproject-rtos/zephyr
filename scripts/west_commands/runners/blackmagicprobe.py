# Copyright (c) 2018 Roman Tataurov <diytronic@yandex.ru>
# Modified 2018 Tavish Naruka <tavishnaruka@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
'''Runner for flashing with Black Magic Probe.'''
# https://github.com/blacksphere/blackmagic/wiki

import signal

from runners.core import ZephyrBinaryRunner, RunnerCaps

class BlackMagicProbeRunner(ZephyrBinaryRunner):
    '''Runner front-end for Black Magic probe.'''

    def __init__(self, cfg, gdb_serial, connect_srst=False):
        super().__init__(cfg)
        self.gdb = [cfg.gdb] if cfg.gdb else None
        self.elf_file = cfg.elf_file
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
        return RunnerCaps(commands={'flash', 'debug', 'attach'})

    @classmethod
    def do_create(cls, cfg, args):
        return BlackMagicProbeRunner(cfg, args.gdb_serial, args.connect_srst)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--gdb-serial', default='/dev/ttyACM0',
                            help='GDB serial port')
        parser.add_argument('--connect-srst', action='store_true',
                            help='Assert SRST during connect? (default: no)')

    def bmp_flash(self, command, **kwargs):
        if self.elf_file is None:
            raise ValueError('Cannot debug; elf file is missing')
        command = (self.gdb +
                   ['-ex', "set confirm off",
                    '-ex', "target extended-remote {}".format(
                        self.gdb_serial)] +
                    self.connect_srst_enable_arg +
                   ['-ex', "monitor swdp_scan",
                    '-ex', "attach 1",
                    '-ex', "load {}".format(self.elf_file),
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
        if self.elf_file is None:
            command = (self.gdb +
                       ['-ex', "set confirm off",
                        '-ex', "target extended-remote {}".format(
                            self.gdb_serial)] +
                        self.connect_srst_disable_arg +
                       ['-ex', "monitor swdp_scan",
                        '-ex', "attach 1"])
        else:
            command = (self.gdb +
                       ['-ex', "set confirm off",
                        '-ex', "target extended-remote {}".format(
                            self.gdb_serial)] +
                        self.connect_srst_disable_arg +
                       ['-ex', "monitor swdp_scan",
                        '-ex', "attach 1",
                        '-ex', "file {}".format(self.elf_file)])
        self.check_call_ignore_sigint(command)

    def bmp_debug(self, command, **kwargs):
        if self.elf_file is None:
            raise ValueError('Cannot debug; elf file is missing')
        command = (self.gdb +
                   ['-ex', "set confirm off",
                    '-ex', "target extended-remote {}".format(
                        self.gdb_serial)] +
                    self.connect_srst_enable_arg +
                   ['-ex', "monitor swdp_scan",
                    '-ex', "attach 1",
                    '-ex', "file {}".format(self.elf_file),
                    '-ex', "load {}".format(self.elf_file)])
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
