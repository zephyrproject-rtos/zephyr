# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for pyOCD .'''

from os import path
import os

from .core import ZephyrBinaryRunner, get_env_or_bail

DEFAULT_PYOCD_GDB_PORT = 3333


class PyOcdBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for pyOCD.'''

    def __init__(self, target, flashtool='pyocd-flashtool',
                 gdb=None, gdbserver='pyocd-gdbserver',
                 gdb_port=DEFAULT_PYOCD_GDB_PORT, tui=None,
                 bin_name=None, elf_name=None,
                 board_id=None, daparg=None, debug=False):
        super(PyOcdBinaryRunner, self).__init__(debug=debug)

        self.target_args = ['-t', target]
        self.flashtool = flashtool
        self.gdb_cmd = [gdb] if gdb is not None else None
        self.gdbserver = gdbserver
        self.gdb_port = gdb_port
        self.tui_args = [tui] if tui is not None else []
        self.bin_name = bin_name
        self.elf_name = elf_name

        board_args = []
        if board_id is not None:
            board_args = ['-b', board_id]
        self.board_args = board_args

        daparg_args = []
        if daparg is not None:
            daparg_args = ['-da', daparg]
        self.daparg_args = daparg_args

    def replaces_shell_script(shell_script, command):
        return (command in {'flash', 'debug', 'debugserver'} and
                shell_script == 'pyocd.sh')

    def port_args(self):
        return ['-p', str(self.gdb_port)]

    def create_from_env(command, debug):
        '''Create runner from environment.

        Required:

        - PYOCD_TARGET: target override

        Optional:

        - PYOCD_DAPARG: arguments to pass to pyocd tool, default is none
        - PYOCD_BOARD_ID: ID of board to flash, default is to prompt

        Required for 'flash':

        - O: build output directory
        - KERNEL_BIN_NAME: name of kernel binary

        Optional for 'flash':

        - PYOCD_FLASHTOOL: flash tool path, defaults to pyocd-flashtool

        Required for 'debug':

        - O: build output directory
        - KERNEL_ELF_NAME
        - GDB: gdb executable

        Optional for 'debug', 'debugserver':

        - TUI: one additional argument to GDB (e.g. -tui)
        - GDB_PORT: pyocd gdb port, defaults to 3333
        - PYOCD_GDBSERVER: gdb server executable, defaults to pyocd-gdbserver
        '''
        target = get_env_or_bail('PYOCD_TARGET')

        o = os.environ.get('O', None)
        bin_ = os.environ.get('KERNEL_BIN_NAME', None)
        elf = os.environ.get('KERNEL_ELF_NAME', None)
        bin_name = None
        elf_name = None
        if o is not None:
            if bin_ is not None:
                bin_name = path.join(o, bin_)
            if elf is not None:
                elf_name = path.join(o, elf)

        flashtool = os.environ.get('PYOCD_FLASHTOOL', 'pyocd-flashtool')
        board_id = os.environ.get('PYOCD_BOARD_ID', None)
        daparg = os.environ.get('PYOCD_DAPARG', None)
        gdb = os.environ.get('GDB', None)
        gdbserver = os.environ.get('PYOCD_GDBSERVER', 'pyocd-gdbserver')
        gdb_port = os.environ.get('GDB_PORT', DEFAULT_PYOCD_GDB_PORT)
        tui = os.environ.get('TUI', None)

        return PyOcdBinaryRunner(target, flashtool=flashtool, gdb=gdb,
                                 gdbserver=gdbserver, gdb_port=gdb_port,
                                 tui=tui, bin_name=bin_name, elf_name=elf_name,
                                 board_id=board_id, daparg=daparg, debug=debug)

    def do_run(self, command, **kwargs):
        if command not in {'flash', 'debug', 'debugserver'}:
            raise ValueError('{} is not supported'.format(command))

        if command == 'flash':
            self.flash(**kwargs)
        else:
            self.debug_debugserver(command, **kwargs)

    def flash(self, **kwargs):
        if self.bin_name is None:
            raise ValueError('Cannot flash; bin_name is missing')

        cmd = ([self.flashtool] +
               self.daparg_args +
               self.target_args +
               self.board_args +
               [self.bin_name])

        print('Flashing Target Device')
        self.check_call(cmd)

    def print_gdbserver_message(self):
        print('pyOCD GDB server running on port {}'.format(self.gdb_port))

    def debug_debugserver(self, command, **kwargs):
        server_cmd = ([self.gdbserver] +
                      self.daparg_args +
                      self.port_args() +
                      self.target_args +
                      self.board_args)

        if command == 'debugserver':
            self.print_gdbserver_message()
            self.check_call(server_cmd)
        else:
            if self.gdb_cmd is None:
                raise ValueError('Cannot debug; gdb is missing')
            if self.elf_name is None:
                raise ValueError('Cannot debug; elf is missing')
            client_cmd = (self.gdb_cmd +
                          self.tui_args +
                          [self.elf_name] +
                          ['-ex', 'target remote :{}'.format(self.gdb_port),
                           '-ex', 'load',
                           '-ex', 'monitor reset halt'])
            self.print_gdbserver_message()
            self.run_server_and_client(server_cmd, client_cmd)
