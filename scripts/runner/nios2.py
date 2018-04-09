# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for NIOS II, based on quartus-flash.py and GDB.'''

from .core import ZephyrBinaryRunner, NetworkPortHelper


class Nios2BinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for NIOS II.'''

    # From the original shell script:
    #
    #     "XXX [flash] only support[s] cases where the .elf is sent
    #      over the JTAG and the CPU directly boots from __start. CONFIG_XIP
    #      and CONFIG_INCLUDE_RESET_VECTOR must be disabled."

    def __init__(self, hex_name=None, elf_name=None, cpu_sof=None,
                 quartus_py=None, gdb=None, tui=False, debug=False):
        super(Nios2BinaryRunner, self).__init__(debug=debug)
        self.hex_name = hex_name
        self.elf_name = elf_name
        self.cpu_sof = cpu_sof
        self.quartus_py = quartus_py
        self.gdb_cmd = [gdb] if gdb is not None else None
        self.tui_arg = ['-tui'] if tui else []

    @classmethod
    def name(cls):
        return 'nios2'

    @classmethod
    def do_add_parser(cls, parser):
        # TODO merge quartus-flash.py script into this file.
        parser.add_argument('--quartus-flash', required=True)
        parser.add_argument('--cpu-sof', required=True,
                            help='path to the the CPU .sof data')
        parser.add_argument('--tui', default=False, action='store_true',
                            help='if given, GDB uses -tui')

    @classmethod
    def create_from_args(command, args):
        return Nios2BinaryRunner(hex_name=args.kernel_hex,
                                 elf_name=args.kernel_elf,
                                 cpu_sof=args.cpu_sof,
                                 quartus_py=args.quartus_flash,
                                 gdb=args.gdb, tui=args.tui,
                                 debug=args.verbose)

    def do_run(self, command, **kwargs):
        if command == 'flash':
            self.flash(**kwargs)
        else:
            self.debug_debugserver(command, **kwargs)

    def flash(self, **kwargs):
        if self.quartus_py is None:
            raise ValueError('Cannot flash; --quartus-flash not given.')
        if self.cpu_sof is None:
            raise ValueError('Cannot flash; --cpu-sof not given.')

        cmd = [self.quartus_py,
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
