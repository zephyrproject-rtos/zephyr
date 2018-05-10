# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for pyOCD .'''

import os
import sys
from .core import ZephyrBinaryRunner, RunnerCaps, BuildConfiguration
from .. import log

DEFAULT_PYOCD_GDB_PORT = 3333


class PyOcdBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for pyOCD.'''

    def __init__(self, cfg, target,
                 flashtool='pyocd-flashtool', flash_addr=0x0,
                 flashtool_opts=None,
                 gdbserver='pyocd-gdbserver',
                 gdb_port=DEFAULT_PYOCD_GDB_PORT, tui=False,
                 board_id=None, daparg=None):
        super(PyOcdBinaryRunner, self).__init__(cfg)

        self.target_args = ['-t', target]
        self.flashtool = flashtool
        self.flash_addr_args = ['-a', hex(flash_addr)] if flash_addr else []
        self.gdb_cmd = [cfg.gdb] if cfg.gdb is not None else None
        self.gdbserver = gdbserver
        self.gdb_port = gdb_port
        self.tui_args = ['-tui'] if tui else []
        self.bin_name = cfg.kernel_bin
        self.elf_name = cfg.kernel_elf

        board_args = []
        if board_id is not None:
            board_args = ['-b', board_id]
        self.board_args = board_args

        daparg_args = []
        if daparg is not None:
            daparg_args = ['-da', daparg]
        self.daparg_args = daparg_args

        self.flashtool_extra = flashtool_opts if flashtool_opts else []

    @classmethod
    def name(cls):
        return 'pyocd'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(flash_addr=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--target', required=True,
                            help='target override')

        parser.add_argument('--daparg',
                            help='Additional -da arguments to pyocd tool')
        parser.add_argument('--flashtool', default='pyocd-flashtool',
                            help='flash tool path, default is pyocd-flashtool')
        parser.add_argument('--flashtool-opt', default=[], action='append',
                            help='''Additional options for pyocd-flashtool,
                            e.g. -ce to chip erase''')
        parser.add_argument('--gdbserver', default='pyocd-gdbserver',
                            help='GDB server, default is pyocd-gdbserver')
        parser.add_argument('--gdb-port', default=DEFAULT_PYOCD_GDB_PORT,
                            help='pyocd gdb port, defaults to {}'.format(
                                DEFAULT_PYOCD_GDB_PORT))
        parser.add_argument('--tui', default=False, action='store_true',
                            help='if given, GDB uses -tui')
        parser.add_argument('--board-id',
                            help='ID of board to flash, default is to prompt')

    @classmethod
    def create(cls, cfg, args):
        daparg = os.environ.get('PYOCD_DAPARG')
        if daparg:
            log.wrn('Setting PYOCD_DAPARG in the environment is',
                    'deprecated; use the --daparg option instead.')
            if args.daparg is None:
                log.dbg('Missing --daparg set to {} from environment'.format(
                    daparg), level=log.VERBOSE_VERY)
                args.daparg = daparg

        build_conf = BuildConfiguration(cfg.build_dir)
        flash_addr = cls.get_flash_address(args, build_conf)

        return PyOcdBinaryRunner(
            cfg, args.target, flashtool=args.flashtool,
            flash_addr=flash_addr, flashtool_opts=args.flashtool_opt,
            gdbserver=args.gdbserver, gdb_port=args.gdb_port, tui=args.tui,
            board_id=args.board_id, daparg=args.daparg)

    def port_args(self):
        return ['-p', str(self.gdb_port)]

    def do_run(self, command, **kwargs):
        if command == 'flash':
            self.flash(**kwargs)
        else:
            self.debug_debugserver(command, **kwargs)

    def flash(self, **kwargs):
        if self.bin_name is None:
            raise ValueError('Cannot flash; bin_name is missing')

        cmd = ([self.flashtool] +
               self.flash_addr_args +
               self.daparg_args +
               self.target_args +
               self.board_args +
               self.flashtool_extra +
               [self.bin_name])

        log.inf('Flashing Target Device')
        self.check_call(cmd)

    def print_gdbserver_message(self):
        log.inf('pyOCD GDB server running on port {}'.format(self.gdb_port))

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
