# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for debugging with xt-gdb.'''

from os import path

from .core import ZephyrBinaryRunner, RunnerCaps


class XtensaBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for xt-gdb.'''

    def __init__(self, gdb, elf_name, debug=False):
        super(XtensaBinaryRunner, self).__init__(debug=debug)
        self.gdb_cmd = [gdb]
        self.elf_name = elf_name

    @classmethod
    def name(cls):
        return 'xtensa'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'debug'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--xcc-tools', required=True,
                            help='path to XTensa tools')

    @classmethod
    def create_from_args(command, args):
        xt_gdb = path.join(args.xcc_tools, 'bin', 'xt-gdb')
        return XtensaBinaryRunner(xt_gdb, args.kernel_elf, args.verbose)

    def do_run(self, command, **kwargs):
        gdb_cmd = (self.gdb_cmd + [self.elf_name])

        self.check_call(gdb_cmd)
