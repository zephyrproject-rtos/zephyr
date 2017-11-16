# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for debugging with xt-gdb.'''

from os import path

from .core import ZephyrBinaryRunner, RunnerCaps, get_env_or_bail


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

    def create_from_env(command, debug):
        '''Create runner from environment.

        Required:

        - XCC_TOOLS: path to Xtensa tools
        - O: build output directory
        - KERNEL_ELF_NAME: zephyr kernel binary in ELF format
        '''
        xt_gdb = path.join(get_env_or_bail('XCC_TOOLS'), 'bin', 'xt-gdb')
        elf_name = path.join(get_env_or_bail('O'),
                             get_env_or_bail('KERNEL_ELF_NAME'))

        return XtensaBinaryRunner(xt_gdb, elf_name)

    def do_run(self, command, **kwargs):
        gdb_cmd = (self.gdb_cmd + [self.elf_name])

        self.check_call(gdb_cmd)
