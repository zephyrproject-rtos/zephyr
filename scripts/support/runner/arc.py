# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''ARC architecture-specific runner.'''

from os import path
import os
import shlex

from .core import ZephyrBinaryRunner, get_env_or_bail

DEFAULT_ARC_TCL_PORT = 6333
DEFAULT_ARC_TELNET_PORT = 4444
DEFAULT_ARC_GDB_PORT = 3333


class ArcBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for the EM Starterkit board, using openocd.'''

    # This unusual 'flash' implementation matches the original shell script.
    #
    # It works by starting a GDB server in a separate session, connecting a
    # client to it to load the program, and running 'continue' within the
    # client to execute the application.
    #
    # TODO: exit immediately when flashing is done, leaving Zephyr running.

    def __init__(self, elf, zephyr_base, board_dir,
                 gdb, openocd='openocd', extra_init=None, default_path=None,
                 tui=None, tcl_port=DEFAULT_ARC_TCL_PORT,
                 telnet_port=DEFAULT_ARC_TELNET_PORT,
                 gdb_port=DEFAULT_ARC_GDB_PORT, debug=False):
        super(ArcBinaryRunner, self).__init__(debug=debug)
        self.elf = elf
        self.zephyr_base = zephyr_base
        self.board_dir = board_dir
        self.gdb = gdb
        search_args = []
        if default_path is not None:
            search_args = ['-s', default_path]
        self.openocd_cmd = [openocd] + search_args
        self.extra_init = extra_init if extra_init is not None else []
        self.tui = tui
        self.tcl_port = tcl_port
        self.telnet_port = telnet_port
        self.gdb_port = gdb_port

    def replaces_shell_script(shell_script, command):
        return (command in {'flash', 'debug', 'debugserver'} and
                shell_script == 'arc_debugger.sh')

    def create_from_env(command, debug):
        '''Create runner from environment.

        Required:

        - O: build output directory
        - KERNEL_ELF_NAME: zephyr kernel binary in ELF format
        - ZEPHYR_BASE: zephyr Git repository base directory
        - BOARD_DIR: board directory
        - GDB: gdb executable

        Optional:

        - OPENOCD: path to openocd, defaults to openocd
        - OPENOCD_EXTRA_INIT: initialization command for GDB server
        - OPENOCD_DEFAULT_PATH: openocd search path to use
        - TUI: if present, passed to gdb server used to flash
        - TCL_PORT: openocd TCL port, defaults to 6333
        - TELNET_PORT: openocd telnet port, defaults to 4444
        - GDB_PORT: openocd gdb port, defaults to 3333
        '''
        elf = path.join(get_env_or_bail('O'),
                        get_env_or_bail('KERNEL_ELF_NAME'))
        zephyr_base = get_env_or_bail('ZEPHYR_BASE')
        board_dir = get_env_or_bail('BOARD_DIR')
        gdb = get_env_or_bail('GDB')

        openocd = os.environ.get('OPENOCD', 'openocd')
        extra_init = os.environ.get('OPENOCD_EXTRA_INIT', None)
        if extra_init is not None:
            extra_init = shlex.split(extra_init)
        default_path = os.environ.get('OPENOCD_DEFAULT_PATH', None)
        tui = os.environ.get('TUI', None)
        tcl_port = int(os.environ.get('TCL_PORT',
                                      str(DEFAULT_ARC_TCL_PORT)))
        telnet_port = int(os.environ.get('TELNET_PORT',
                                         str(DEFAULT_ARC_TELNET_PORT)))
        gdb_port = int(os.environ.get('GDB_PORT',
                                      str(DEFAULT_ARC_GDB_PORT)))

        return ArcBinaryRunner(elf, zephyr_base, board_dir,
                               gdb, openocd=openocd, extra_init=extra_init,
                               default_path=default_path, tui=tui,
                               tcl_port=tcl_port, telnet_port=telnet_port,
                               gdb_port=gdb_port, debug=debug)

    def do_run(self, command, **kwargs):
        if command not in {'flash', 'debug', 'debugserver'}:
            raise ValueError('{} is not supported'.format(command))

        kwargs['openocd-cfg'] = path.join(self.board_dir, 'support',
                                          'openocd.cfg')

        if command in {'flash', 'debug'}:
            self.flash_debug(command, **kwargs)
        else:
            self.debugserver(**kwargs)

    def flash_debug(self, command, **kwargs):
        config = kwargs['openocd-cfg']

        server_cmd = (self.openocd_cmd +
                      ['-f', config] +
                      self.extra_init +
                      ['-c', 'tcl_port {}'.format(self.tcl_port),
                       '-c', 'telnet_port {}'.format(self.telnet_port),
                       '-c', 'gdb_port {}'.format(self.gdb_port),
                       '-c', 'init',
                       '-c', 'targets',
                       '-c', 'halt'])

        tui_arg = []
        if self.tui is not None:
            tui_arg = [self.tui]

        continue_arg = []
        if command == 'flash':
            continue_arg = ['-ex', 'c']

        gdb_cmd = ([self.gdb] +
                   tui_arg +
                   ['-ex', 'target remote :{}'.format(self.gdb_port),
                    '-ex', 'load'] +
                   continue_arg +
                   [self.elf])

        self.run_server_and_client(server_cmd, gdb_cmd)

    def debugserver(self, **kwargs):
        config = kwargs['openocd-cfg']
        cmd = (self.openocd_cmd +
               ['-f', config,
                '-c', 'init',
                '-c', 'targets',
                '-c', 'reset halt'])
        self.check_call(cmd)
