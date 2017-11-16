# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for openocd.'''

from os import path
import os
import shlex

from .core import ZephyrBinaryRunner, get_env_or_bail, get_env_strip_or

DEFAULT_OPENOCD_TCL_PORT = 6333
DEFAULT_OPENOCD_TELNET_PORT = 4444
DEFAULT_OPENOCD_GDB_PORT = 3333


class OpenOcdBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for openocd.'''

    def __init__(self, openocd_config,
                 openocd='openocd', default_path=None,
                 bin_name=None, elf_name=None,
                 load_cmd=None, verify_cmd=None, pre_cmd=None, post_cmd=None,
                 extra_init=None,
                 tcl_port=DEFAULT_OPENOCD_TCL_PORT,
                 telnet_port=DEFAULT_OPENOCD_TELNET_PORT,
                 gdb_port=DEFAULT_OPENOCD_GDB_PORT,
                 gdb=None, tui=None, debug=False):
        super(OpenOcdBinaryRunner, self).__init__(debug=debug)
        self.openocd_config = openocd_config

        search_args = []
        if default_path is not None:
            search_args = ['-s', default_path]
        self.openocd_cmd = [openocd] + search_args
        self.bin_name = bin_name
        self.elf_name = elf_name
        self.load_cmd = load_cmd
        self.verify_cmd = verify_cmd
        self.pre_cmd = pre_cmd
        self.post_cmd = post_cmd
        self.extra_init = extra_init if extra_init is not None else []
        self.tcl_port = tcl_port
        self.telnet_port = telnet_port
        self.gdb_port = gdb_port
        self.gdb_cmd = [gdb] if gdb is not None else None
        self.tui_arg = [tui] if tui is not None else []

    @classmethod
    def name(cls):
        return 'openocd'

    def create_from_env(command, debug):
        '''Create runner from environment.

        Required:

        - ZEPHYR_BASE: zephyr Git repository base directory
        - BOARD_DIR: directory of board definition

        Optional:

        - OPENOCD: path to openocd, defaults to openocd
        - OPENOCD_DEFAULT_PATH: openocd search path to use

        Required for 'flash':

        - O: build output directory
        - KERNEL_BIN_NAME: zephyr kernel binary
        - OPENOCD_LOAD_CMD: command to load binary into flash
        - OPENOCD_VERIFY_CMD: command to verify flash executed correctly

        Optional for 'flash':

        - OPENOCD_PRE_CMD: command to run before any others
        - OPENOCD_POST_CMD: command to run after verifying flash write

        Required for 'debug':

        - GDB: GDB executable
        - O: build output directory
        - KERNEL_ELF_NAME: zephyr kernel binary, ELF format

        Optional for 'debug':

        - TUI: one additional argument to GDB (e.g. -tui)
        - OPENOCD_EXTRA_INIT: additional arguments to pass to openocd
        - TCL_PORT: openocd TCL port, defaults to 6333
        - TELNET_PORT: openocd telnet port, defaults to 4444
        - GDB_PORT: openocd gdb port, defaults to 3333
        '''
        zephyr_base = get_env_or_bail('ZEPHYR_BASE')
        board_dir = get_env_or_bail('BOARD_DIR')
        openocd_config = path.join(board_dir, 'support', 'openocd.cfg')

        openocd = os.environ.get('OPENOCD', 'openocd')
        default_path = os.environ.get('OPENOCD_DEFAULT_PATH', None)

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

        load_cmd = get_env_strip_or('OPENOCD_LOAD_CMD', '"', None)
        verify_cmd = get_env_strip_or('OPENOCD_VERIFY_CMD', '"', None)
        pre_cmd = get_env_strip_or('OPENOCD_PRE_CMD', '"', None)
        post_cmd = get_env_strip_or('OPENOCD_POST_CMD', '"', None)

        gdb = os.environ.get('GDB', None)
        tui = os.environ.get('TUI', None)
        extra_init = os.environ.get('OPENOCD_EXTRA_INIT', None)
        if extra_init is not None:
            extra_init = shlex.split(extra_init)
        tcl_port = int(os.environ.get('TCL_PORT',
                                      str(DEFAULT_OPENOCD_TCL_PORT)))
        telnet_port = int(os.environ.get('TELNET_PORT',
                                         str(DEFAULT_OPENOCD_TELNET_PORT)))
        gdb_port = int(os.environ.get('GDB_PORT',
                                      str(DEFAULT_OPENOCD_GDB_PORT)))

        return OpenOcdBinaryRunner(openocd_config,
                                   openocd=openocd, default_path=default_path,
                                   bin_name=bin_name, elf_name=elf_name,
                                   load_cmd=load_cmd, verify_cmd=verify_cmd,
                                   pre_cmd=pre_cmd, post_cmd=post_cmd,
                                   extra_init=extra_init, tcl_port=tcl_port,
                                   telnet_port=telnet_port, gdb_port=gdb_port,
                                   gdb=gdb, tui=tui, debug=debug)

    def do_run(self, command, **kwargs):
        if command == 'flash':
            self.do_flash(**kwargs)
        elif command == 'debug':
            self.do_debug(**kwargs)
        else:
            self.do_debugserver(**kwargs)

    def do_flash(self, **kwargs):
        if self.bin_name is None:
            raise ValueError('Cannot flash; binary name is missing')
        if self.load_cmd is None:
            raise ValueError('Cannot flash; load command is missing')
        if self.verify_cmd is None:
            raise ValueError('Cannot flash; verify command is missing')

        pre_cmd = []
        if self.pre_cmd is not None:
            pre_cmd = ['-c', self.pre_cmd]

        post_cmd = []
        if self.post_cmd is not None:
            post_cmd = ['-c', self.post_cmd]

        cmd = (self.openocd_cmd +
               ['-f', self.openocd_config,
                '-c', 'init',
                '-c', 'targets'] +
               pre_cmd +
               ['-c', 'reset halt',
                '-c', self.load_cmd,
                '-c', 'reset halt',
                '-c', self.verify_cmd] +
               post_cmd +
               ['-c', 'reset run',
                '-c', 'shutdown'])
        self.check_call(cmd)

    def do_debug(self, **kwargs):
        if self.gdb_cmd is None:
            raise ValueError('Cannot debug; no gdb specified')
        if self.elf_name is None:
            raise ValueError('Cannot debug; no .elf specified')

        server_cmd = (self.openocd_cmd +
                      ['-f', self.openocd_config] +
                      self.extra_init +
                      ['-c', 'tcl_port {}'.format(self.tcl_port),
                       '-c', 'telnet_port {}'.format(self.telnet_port),
                       '-c', 'gdb_port {}'.format(self.gdb_port),
                       '-c', 'init',
                       '-c', 'targets',
                       '-c', 'halt'])

        gdb_cmd = (self.gdb_cmd + self.tui_arg +
                   ['-ex', 'target remote :{}'.format(self.gdb_port),
                    self.elf_name])

        self.run_server_and_client(server_cmd, gdb_cmd)

    def do_debugserver(self, **kwargs):
        cmd = (self.openocd_cmd +
               ['-f', self.openocd_config,
                '-c', 'init',
                '-c', 'targets',
                '-c', 'reset halt'])
        self.check_call(cmd)
