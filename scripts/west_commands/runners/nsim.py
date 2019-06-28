# Copyright (c) 2018 Synopsys Inc.
# Copyright (c) 2017 Open Source Foundries Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''ARC architecture-specific runners.'''

from os import path

from runners.core import ZephyrBinaryRunner

DEFAULT_ARC_GDB_PORT = 3333
DEFAULT_PROPS_FILE = 'nsim_em.props'


class NsimBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for the ARC nSIM.'''

    # This unusual 'flash' implementation matches the original shell script.
    #
    # It works by starting a GDB server in a separate session, connecting a
    # client to it to load the program, and running 'continue' within the
    # client to execute the application.
    #

    def __init__(self, cfg,
                 tui=False,
                 gdb_port=DEFAULT_ARC_GDB_PORT,
                 props=DEFAULT_PROPS_FILE):
        super(NsimBinaryRunner, self).__init__(cfg)
        self.gdb_cmd = [cfg.gdb] + (['-tui'] if tui else [])
        self.nsim_cmd = ['nsimdrv']
        self.gdb_port = gdb_port
        self.props = props

    @classmethod
    def name(cls):
        return 'arc-nsim'

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--gdb-port', default=DEFAULT_ARC_GDB_PORT,
                            help='nsim gdb port, defaults to 3333')
        parser.add_argument('--props', default=DEFAULT_PROPS_FILE,
                            help='nsim props file, defaults to nsim.props')

    @classmethod
    def create(cls, cfg, args):
        if cfg.gdb is None:
            raise ValueError('--gdb not provided at command line')

        return NsimBinaryRunner(
            cfg,
            gdb_port=args.gdb_port,
            props=args.props)

    def do_run(self, command, **kwargs):
        self.require(self.nsim_cmd[0])
        kwargs['nsim-cfg'] = path.join(self.cfg.board_dir, 'support',
                                       self.props)

        if command == 'flash':
            self.do_flash(**kwargs)
        elif command == 'debug':
            self.do_debug(**kwargs)
        else:
            self.debugserver(**kwargs)

    def do_flash(self, **kwargs):
        config = kwargs['nsim-cfg']

        cmd = (self.nsim_cmd + ['-propsfile', config, self.cfg.elf_file])
        self.check_call(cmd)

    def do_debug(self, **kwargs):
        config = kwargs['nsim-cfg']

        server_cmd = (self.nsim_cmd + ['-gdb',
                                       '-port={}'.format(self.gdb_port),
                                       '-propsfile', config])
        gdb_cmd = (self.gdb_cmd +
                   ['-ex', 'target remote :{}'.format(self.gdb_port),
                    '-ex', 'load', self.cfg.elf_file])
        self.require(gdb_cmd[0])

        self.run_server_and_client(server_cmd, gdb_cmd)

    def debugserver(self, **kwargs):
        config = kwargs['nsim-cfg']

        cmd = (self.nsim_cmd +
               ['-gdb', '-port={}'.format(self.gdb_port),
                '-propsfile', config])

        self.check_call(cmd)
