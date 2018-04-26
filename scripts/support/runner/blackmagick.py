# Copyright (c) 2018 Roman Tataurov <diytronic@yandex.ru>
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with Black Magick Probe.'''

# https://github.com/blacksphere/blackmagic/wiki

import sys

from .core import ZephyrBinaryRunner, RunnerCaps

class BlackMagickRunner(ZephyrBinaryRunner):
    '''Runner front-end for Black Magick probe.'''

    def __init__(self, gdb, kernel_elf, gdb_serial, debug=False):
        super(BlackMagickRunner, self).__init__(debug=debug)
        self.gdb = gdb
        self.kernel_elf = kernel_elf
        self.gdb_serial = gdb_serial

    @classmethod
    def name(cls):
        return 'blackmagick'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--gdb-serial', default='/dev/ttyACM0',
                            help='GDB port')

    @classmethod
    def create_from_args(cls, args):
        return BlackMagickRunner(args.gdb,
            args.kernel_elf, args.gdb_serial, debug=args.verbose)

    def do_run(self, command, **kwargs):
        command = [
            self.gdb,
            '-ex', "set confirm off",
            '-ex', "target extended-remote {}".format(self.gdb_serial),
            '-ex', "monitor swdp_scan",
            '-ex', "attach 1",
            '-ex', "load {}".format(self.kernel_elf),
            '-ex', "kill",
            '-ex', "quit",
            '-silent'
        ]

        self.check_call(command)
