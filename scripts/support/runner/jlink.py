# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for debugging with JLink.'''

from os import path
import os

from .core import ZephyrBinaryRunner, get_env_or_bail

DEFAULT_JLINK_GDB_PORT = 2331


class JLinkBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for the J-Link GDB server.'''

    def __init__(self, device,
                 gdbserver='JLinkGDBServer', iface='swd', elf_name=None,
                 gdb=None, gdb_port=DEFAULT_JLINK_GDB_PORT, tui=None,
                 debug=False):
        super(JLinkBinaryRunner, self).__init__(debug=debug)
        self.device = device
        self.gdbserver_cmd = [gdbserver]
        self.iface = iface
        self.elf_name = elf_name
        self.gdb_cmd = [gdb] if gdb is not None else None
        self.gdb_port = gdb_port
        self.tui_arg = [tui] if tui is not None else []

    def replaces_shell_script(shell_script, command):
        return (command in {'debug', 'debugserver'} and
                shell_script == 'jlink.sh')

    def create_from_env(command, debug):
        '''Create runner from environment.

        Required:

        - JLINK_DEVICE: device name

        Required for 'debug':

        - GDB: gdb to use
        - O: build output directory
        - KERNEL_ELF_NAME: zephyr kernel binary in ELF format

        Optional for 'debug':

        - TUI: if present, passed to gdb server used to flash

        Optional for 'debug', 'debugserver':

        - JLINK_GDBSERVER: default is JLinkGDBServer
        - GDB_PORT: default is 2331
        - JLINK_IF: default is swd
        '''
        device = get_env_or_bail('JLINK_DEVICE')

        gdb = os.environ.get('GDB', None)
        o = os.environ.get('O', None)
        elf = os.environ.get('KERNEL_ELF_NAME', None)
        elf_name = None
        if o is not None:
            if elf is not None:
                elf_name = path.join(o, elf)
        tui = os.environ.get('TUI', None)

        gdbserver = os.environ.get('JLINK_GDBSERVER', 'JLinkGDBServer')
        gdb_port = int(os.environ.get('GDB_PORT',
                                      str(DEFAULT_JLINK_GDB_PORT)))
        iface = os.environ.get('JLINK_IF', 'swd')

        return JLinkBinaryRunner(device, gdbserver=gdbserver,
                                 iface=iface, elf_name=elf_name,
                                 gdb=gdb, gdb_port=gdb_port, tui=tui,
                                 debug=debug)

    def print_gdbserver_message(self):
        print('JLink GDB server running on port {}'.format(self.gdb_port))

    def run(self, command, **kwargs):
        if command not in {'debug', 'debugserver'}:
            raise ValueError('{} is not supported'.format(command))

        server_cmd = (self.gdbserver_cmd +
                      ['-port', str(self.gdb_port),
                       '-if', self.iface,
                       '-device', self.device,
                       '-silent',
                       '-singlerun'])

        if command == 'debugserver':
            self.print_gdbserver_message()
            self.check_call(server_cmd)
        else:
            if self.gdb_cmd is None:
                raise ValueError('Cannot debug; gdb is missing')
            if self.elf_name is None:
                raise ValueError('Cannot debug; elf is missing')
            client_cmd = (self.gdb_cmd +
                          self.tui_arg +
                          [self.elf_name] +
                          ['-ex', 'target remote :{}'.format(self.gdb_port),
                           '-ex', 'monitor halt',
                           '-ex', 'monitor reset',
                           '-ex', 'load'])
            self.print_gdbserver_message()
            self.run_server_and_client(server_cmd, client_cmd)
