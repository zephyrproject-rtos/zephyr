# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for NIOS II, based on quartus-flash.py and GDB.'''

from os import path
import os

from .core import ZephyrBinaryRunner, NetworkPortHelper


class Nios2BinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for NIOS II.'''

    # From the original shell script:
    #
    #     "XXX [flash] only support[s] cases where the .elf is sent
    #      over the JTAG and the CPU directly boots from __start. CONFIG_XIP
    #      and CONFIG_INCLUDE_RESET_VECTOR must be disabled."

    def __init__(self, hex_name=None, elf_name=None, cpu_sof=None,
                 zephyr_base=None, gdb=None, tui=None, debug=False):
        super(Nios2BinaryRunner, self).__init__(debug=debug)
        self.hex_name = hex_name
        self.elf_name = elf_name
        self.cpu_sof = cpu_sof
        self.zephyr_base = zephyr_base
        self.gdb_cmd = [gdb] if gdb is not None else None
        self.tui_arg = [tui] if tui is not None else []

    def replaces_shell_script(shell_script, command):
        return (command in {'flash', 'debug', 'debugserver'} and
                shell_script == 'nios2.sh')

    def create_from_env(command, debug):
        '''Create runner from environment.

        Required for 'flash', 'debug':

        - O: build output directory

        Required for 'flash':

        - KERNEL_HEX_NAME: name of kernel binary in HEX format
        - NIOS2_CPU_SOF: location of the CPU .sof data
        - ZEPHYR_BASE: zephyr Git repository base directory

        Required for 'debug':

        - KERNEL_ELF_NAME: name of kernel binary in ELF format
        - GDB: GDB executable

        Optional for 'debug':

        - TUI: one additional argument to GDB (e.g. -tui)
        '''
        cpu_sof = os.environ.get('NIOS2_CPU_SOF', None)
        zephyr_base = os.environ.get('ZEPHYR_BASE', None)

        o = os.environ.get('O', None)
        hex_ = os.environ.get('KERNEL_HEX_NAME', None)
        elf = os.environ.get('KERNEL_ELF_NAME', None)
        hex_name = None
        elf_name = None
        if o is not None:
            if hex_ is not None:
                hex_name = path.join(o, hex_)
            if elf is not None:
                elf_name = path.join(o, elf)

        gdb = os.environ.get('GDB', None)
        tui = os.environ.get('TUI', None)

        return Nios2BinaryRunner(hex_name=hex_name, elf_name=elf_name,
                                 cpu_sof=cpu_sof, zephyr_base=zephyr_base,
                                 gdb=gdb, tui=tui, debug=debug)

    def do_run(self, command, **kwargs):
        if command not in {'flash', 'debug', 'debugserver'}:
            raise ValueError('{} is not supported'.format(command))

        if command == 'flash':
            self.flash(**kwargs)
        else:
            self.debug_debugserver(command, **kwargs)

    def flash(self, **kwargs):
        sof_msg = (
            'Cannot flash; '
            'Please set NIOS2_CPU_SOF variable to location of CPU .sof data')

        if self.zephyr_base is None:
            raise ValueError('Cannot flash; ZEPHYR_BASE is missing.')
        if self.cpu_sof is None:
            raise ValueError(sof_msg)
        if self.hex_name is None:
            raise ValueError('Cannot flash; .hex binary name is missing')

        cmd = [path.join(self.zephyr_base, 'scripts', 'support',
                         'quartus-flash.py'),
               '--sof', self.cpu_sof,
               '--kernel', self.hex_name]

        self.check_call(cmd)

    def print_gdbserver_message(self, gdb_port):
        print('Nios II GDB server running on port {}'.format(gdb_port))

    def debug_debugserver(self, command, **kwargs):
        # Per comments in the shell script, the NIOSII GDB server
        # doesn't exit gracefully, so it's better to explicitly search
        # for an unused port. The script picks a random value in
        # between 1024 and 49151, but we'll start with the
        # "traditional" 3333 choice.
        gdb_start = 3333
        nh = NetworkPortHelper()
        gdb_port = nh.get_unused_ports([gdb_start])[0]

        server_cmd = (['nios2-gdb-server',
                       '--tcpport', str(gdb_port),
                       '--stop', '--reset-target'])

        if command == 'debugserver':
            self.print_gdbserver_message(gdb_port)
            self.check_call(server_cmd)
        else:
            if self.elf_name is None:
                raise ValueError('Cannot debug; elf is missing')
            if self.gdb_cmd is None:
                raise ValueError('Cannot debug; no gdb specified')

            gdb_cmd = (self.gdb_cmd +
                       self.tui_arg +
                       [self.elf_name,
                        '-ex', 'target remote :{}'.format(gdb_port)])

            self.print_gdbserver_message(gdb_port)
            self.run_server_and_client(server_cmd, gdb_cmd)
