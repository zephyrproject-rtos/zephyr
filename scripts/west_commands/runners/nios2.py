# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for NIOS II, based on quartus-flash.py and GDB.'''

import os

from runners.core import ZephyrBinaryRunner, NetworkPortHelper


class Nios2BinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for NIOS II.'''

    # From the original shell script:
    #
    #     "XXX [flash] only support[s] cases where the .elf is sent
    #      over the JTAG and the CPU directly boots from __start. CONFIG_XIP
    #      and CONFIG_INCLUDE_RESET_VECTOR must be disabled."

    def __init__(self, cfg, quartus_py=None, cpu_sof=None, tui=False):
        super(Nios2BinaryRunner, self).__init__(cfg)
        self.hex_name = cfg.hex_file
        self.elf_name = cfg.elf_file
        self.cpu_sof = cpu_sof
        self.quartus_py = quartus_py
        self.gdb_cmd = [cfg.gdb] if cfg.gdb else None
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
    def create(cls, cfg, args):
        return Nios2BinaryRunner(cfg,
                                 quartus_py=args.quartus_flash,
                                 cpu_sof=args.cpu_sof,
                                 tui=args.tui)

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
        if not os.path.isfile(self.hex_name):
            raise ValueError('Cannot flash; hex file ({}) does not exist. '.
                             format(self.hex_name) +
                             'Try enabling CONFIG_BUILD_OUTPUT_HEX.')

        self.logger.info('Flashing file: {}'.format(self.hex_name))
        cmd = [self.quartus_py,
               '--sof', self.cpu_sof,
               '--kernel', self.hex_name]
        self.require(cmd[0])
        self.check_call(cmd)

    def print_gdbserver_message(self, gdb_port):
        self.logger.info('Nios II GDB server running on port {}'.
                         format(gdb_port))

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
        self.require(server_cmd[0])

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
            self.require(gdb_cmd[0])

            self.print_gdbserver_message(gdb_port)
            self.run_server_and_client(server_cmd, gdb_cmd)
