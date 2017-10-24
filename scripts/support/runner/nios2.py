# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for NIOS II.'''

from os import path

from .core import ZephyrBinaryRunner, get_env_or_bail


class Nios2BinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for NIOS II.'''

    # From the original shell script:
    #
    #     "XXX [flash] only support[s] cases where the .elf is sent
    #      over the JTAG and the CPU directly boots from __start. CONFIG_XIP
    #      and CONFIG_INCLUDE_RESET_VECTOR must be disabled."

    def __init__(self, hex_, cpu_sof, zephyr_base, debug=False):
        super(Nios2BinaryRunner, self).__init__(debug=debug)
        self.hex_ = hex_
        self.cpu_sof = cpu_sof
        self.zephyr_base = zephyr_base

    def replaces_shell_script(shell_script, command):
        return command == 'flash' and shell_script == 'nios2.sh'

    def create_from_env(command, debug):
        '''Create runner from environment.

        Required:

        - O: build output directory
        - KERNEL_HEX_NAME: name of kernel binary in HEX format
        - NIOS2_CPU_SOF: location of the CPU .sof data
        - ZEPHYR_BASE: zephyr Git repository base directory
        '''
        hex_ = path.join(get_env_or_bail('O'),
                         get_env_or_bail('KERNEL_HEX_NAME'))
        cpu_sof = get_env_or_bail('NIOS2_CPU_SOF')
        zephyr_base = get_env_or_bail('ZEPHYR_BASE')

        return Nios2BinaryRunner(hex_, cpu_sof, zephyr_base, debug=debug)

    def run(self, command, **kwargs):
        if command not in {'flash', 'debug', 'debugserver'}:
            raise ValueError('{} is not supported'.format(command))

        if command == 'flash':
            self.flash(**kwargs)
        else:
            self.debug_debugserver(command, **kwargs)

    def flash(self, **kwargs):
        cmd = [path.join(self.zephyr_base, 'scripts', 'support',
                         'quartus-flash.py'),
               '--sof', self.cpu_sof,
               '--kernel', self.hex_]

        self.check_call(cmd)

    def debug_debugserver(command, **kwargs):
        raise NotImplementedError()
