# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for debugging with JLink.'''

from .core import ZephyrBinaryRunner, RunnerCaps

DEFAULT_JLINK_GDB_PORT = 2331


class JLinkBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for the J-Link GDB server.'''

    def __init__(self, device,
                 gdbserver='JLinkGDBServer', iface='swd', elf_name=None,
                 gdb=None, gdb_port=DEFAULT_JLINK_GDB_PORT, tui=False,
                 debug=False):
        super(JLinkBinaryRunner, self).__init__(debug=debug)
        self.device = device
        self.gdbserver_cmd = [gdbserver]
        self.iface = iface
        self.elf_name = elf_name
        self.gdb_cmd = [gdb] if gdb is not None else None
        self.gdb_port = gdb_port
        self.tui_arg = ['-tui'] if tui else []

    @classmethod
    def name(cls):
        return 'jlink'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'debug', 'debugserver'})

    @classmethod
    def do_add_parser(cls, parser):
        # Required:
        parser.add_argument('--device', required=True, help='device name')

        # Optional:
        parser.add_argument('--iface', default='swd',
                            help='interface to use, default is swd')
        parser.add_argument('--tui', default=False, action='store_true',
                            help='if given, GDB uses -tui')
        parser.add_argument('--gdbserver', default='JLinkGDBServer',
                            help='GDB server, default is JLinkGDBServer')
        parser.add_argument('--gdb-port', default=DEFAULT_JLINK_GDB_PORT,
                            help='pyocd gdb port, defaults to {}'.format(
                                DEFAULT_JLINK_GDB_PORT))

    @classmethod
    def create_from_args(cls, args):
        return JLinkBinaryRunner(args.device, gdbserver=args.gdbserver,
                                 iface=args.iface, elf_name=args.kernel_elf,
                                 gdb=args.gdb, gdb_port=args.gdb_port,
                                 tui=args.tui, debug=args.verbose)

    def print_gdbserver_message(self):
        print('JLink GDB server running on port {}'.format(self.gdb_port))

    def do_run(self, command, **kwargs):
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
