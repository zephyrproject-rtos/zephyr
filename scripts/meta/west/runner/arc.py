# Copyright (c) 2017 Linaro Limited.
# Copyright (c) 2017 Open Source Foundries Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''ARC architecture-specific runners.'''

from os import path

from .core import ZephyrBinaryRunner

DEFAULT_ARC_TCL_PORT = 6333
DEFAULT_ARC_TELNET_PORT = 4444
DEFAULT_ARC_GDB_PORT = 3333


class EmStarterKitBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for the EM Starterkit board, using openocd.'''

    # This unusual 'flash' implementation matches the original shell script.
    #
    # It works by starting a GDB server in a separate session, connecting a
    # client to it to load the program, and running 'continue' within the
    # client to execute the application.
    #

    def __init__(self, cfg,
                 tui=False, tcl_port=DEFAULT_ARC_TCL_PORT,
                 telnet_port=DEFAULT_ARC_TELNET_PORT,
                 gdb_port=DEFAULT_ARC_GDB_PORT):
        super(EmStarterKitBinaryRunner, self).__init__(cfg)
        self.gdb_cmd = [cfg.gdb] + (['-tui'] if tui else [])
        search_args = []
        if cfg.openocd_search is not None:
            search_args = ['-s', cfg.openocd_search]
        self.openocd_cmd = [cfg.openocd or 'openocd'] + search_args
        self.tcl_port = tcl_port
        self.telnet_port = telnet_port
        self.gdb_port = gdb_port

    @classmethod
    def name(cls):
        return 'em-starterkit'

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--tui', default=False, action='store_true',
                            help='if given, GDB uses -tui')
        parser.add_argument('--tcl-port', default=DEFAULT_ARC_TCL_PORT,
                            help='openocd TCL port, defaults to 6333')
        parser.add_argument('--telnet-port', default=DEFAULT_ARC_TELNET_PORT,
                            help='openocd telnet port, defaults to 4444')
        parser.add_argument('--gdb-port', default=DEFAULT_ARC_GDB_PORT,
                            help='openocd gdb port, defaults to 3333')

    @classmethod
    def create(cls, cfg, args):
        if cfg.gdb is None:
            raise ValueError('--gdb not provided at command line')

        return EmStarterKitBinaryRunner(
            cfg,
            tui=args.tui, tcl_port=args.tcl_port, telnet_port=args.telnet_port,
            gdb_port=args.gdb_port)

    def do_run(self, command, **kwargs):
        kwargs['openocd-cfg'] = path.join(self.cfg.board_dir, 'support',
                                          'openocd.cfg')

        if command in {'flash', 'debug'}:
            self.flash_debug(command, **kwargs)
        else:
            self.debugserver(**kwargs)

    def flash_debug(self, command, **kwargs):
        config = kwargs['openocd-cfg']

        server_cmd = (self.openocd_cmd +
                      ['-f', config] +
                      ['-c', 'tcl_port {}'.format(self.tcl_port),
                       '-c', 'telnet_port {}'.format(self.telnet_port),
                       '-c', 'gdb_port {}'.format(self.gdb_port),
                       '-c', 'init',
                       '-c', 'targets',
                       '-c', 'halt'])

        continue_arg = []
        if command == 'flash':
            continue_arg = ['-ex', 'set confirm off', '-ex', 'monitor resume',
                            '-ex', 'quit']

        gdb_cmd = (self.gdb_cmd +
                   ['-ex', 'target remote :{}'.format(self.gdb_port),
                    '-ex', 'load'] +
                   continue_arg +
                   [self.cfg.kernel_elf])

        self.run_server_and_client(server_cmd, gdb_cmd)

    def debugserver(self, **kwargs):
        config = kwargs['openocd-cfg']
        cmd = (self.openocd_cmd +
               ['-f', config,
                '-c', 'init',
                '-c', 'targets',
                '-c', 'reset halt'])
        self.check_call(cmd)
