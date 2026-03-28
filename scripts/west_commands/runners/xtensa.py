# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for debugging with xt-gdb.'''

from os import path

from runners.core import RunnerCaps, ZephyrBinaryRunner


class XtensaBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for xt-gdb.'''

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
    def do_create(cls, cfg, args):
        # Override any GDB with the one provided by the XTensa tools.
        cfg.gdb = path.join(args.xcc_tools, 'bin', 'xt-gdb')
        return XtensaBinaryRunner(cfg)

    def do_run(self, command, **kwargs):
        gdb_cmd = [self.cfg.gdb, self.cfg.elf_file]
        self.require(gdb_cmd[0])
        self.check_call(gdb_cmd)
