# Copyright (c) 2019, Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

'''Catch-all module for miscellaneous devices which can't use a
generic or widely used tool like J-Link, OpenOCD, etc.

Please use this sparingly and only when your setup is exotic and
you're willing to handle requests for help. E.g. if your "board" is a
core on a special-purpose SoC which requires a complicated script to
network boot.'''

import argparse

from runners.core import RunnerCaps, ZephyrBinaryRunner


class MiscFlasher(ZephyrBinaryRunner):
    '''Runner for handling special purpose flashing commands.'''

    def __init__(self, cfg, cmd, args):
        super().__init__(cfg)
        if not cmd:
            # This is a board definition error, not a user error,
            # so we can do it now and not in do_run().
            raise ValueError('no command was given')
        self.cmd = cmd
        self.args = args

    @classmethod
    def name(cls):
        return 'misc-flasher'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('cmd',
                            help='''command to run; it will be passed the
                            build directory as its first argument''')
        parser.add_argument('args', nargs=argparse.REMAINDER,
                            help='''additional arguments to pass after the build
                            directory''')

    @classmethod
    def do_create(cls, cfg, args):
        return MiscFlasher(cfg, args.cmd, args.args)

    def do_run(self, *args, **kwargs):
        self.require(self.cmd)
        self.check_call([self.cmd, self.cfg.build_dir] + self.args)
