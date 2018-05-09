# Copyright (c) 2018 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for debugging and flashing Intel_s1000 devices'''
from os import path
from .core import ZephyrBinaryRunner
import time
import signal

DEFAULT_XT_GDB_PORT = 20000

class intel_s1000BinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for Intel_s1000.'''

    def __init__(self,
                 board_dir, xt_ocd_dir,
                 ocd_topology, ocd_jtag_instr, gdb_flash_file,
                 elf_name, gdb,
                 gdb_port=DEFAULT_XT_GDB_PORT, debug=False):
        super(intel_s1000BinaryRunner, self).__init__(debug=debug)
        self.board_dir = board_dir
        self.xt_ocd_dir = xt_ocd_dir
        self.ocd_topology = ocd_topology
        self.ocd_jtag_instr = ocd_jtag_instr
        self.gdb_flash_file = gdb_flash_file
        self.elf_name = elf_name
        self.gdb_cmd = gdb
        self.gdb_port = gdb_port

    @classmethod
    def name(cls):
        return 'intel_s1000'

    @classmethod
    def do_add_parser(cls, parser):
        # Required

        # Optional
        parser.add_argument('--gdb-port', default=DEFAULT_XT_GDB_PORT,
                            help='xt-gdb port, defaults to 20000')
        parser.add_argument('--xt-ocd-dir', default='/opt/Tensilica/xocd-12.0.4/xt-ocd',
                            help='ocd-dir, defaults to /opt/Tensilica/xocd-12.0.4/xt-ocd')
        parser.add_argument('--ocd-topology', default='topology_dsp0_flyswatter2.xml',
                            help='ocd-topology, defaults to topology_dsp0_flyswatter2.xml')
        parser.add_argument('--ocd-jtag-instr', default='dsp0_gdb.txt',
                            help='ocd-jtag-instr, defaults to dsp0_gdb.txt')
        parser.add_argument('--gdb-flash-file', default='load_elf.txt',
                            help='gdb-flash-file, defaults to load_elf.txt')

    @classmethod
    def create_from_args(command, args):
        return intel_s1000BinaryRunner(
            args.board_dir, args.xt_ocd_dir,
            args.ocd_topology, args.ocd_jtag_instr, args.gdb_flash_file,
            args.kernel_elf, args.gdb,
            gdb_port=args.gdb_port, debug=args.verbose)

    def do_run(self, command, **kwargs):
        kwargs['ocd-topology'] =  path.join(self.board_dir, 'support',  self.ocd_topology)
        kwargs['ocd-jtag-instr'] = path.join(self.board_dir, 'support', self.ocd_jtag_instr)
        kwargs['gdb-flash-file'] = path.join(self.board_dir, 'support', self.gdb_flash_file)

        if command == 'flash':
            self.flash(**kwargs)
        elif command == 'debugserver':
            self.debugserver(**kwargs)
        else:
            self.do_debug()

    def flash(self, **kwargs):
        topology_file = kwargs['ocd-topology']
        jtag_instr_file = kwargs['ocd-jtag-instr']
        gdb_flash_file = kwargs['gdb-flash-file']

        self.print_gdbserver_message(self.gdb_port)
        server_cmd = [self.xt_ocd_dir,
                        '-c', topology_file,
                        '-I', jtag_instr_file]

        # Start the server
        # Note that XTOCD always fails the first time. It has to be
        # relaunched the second time to work.
        self.popen_ignore_int(server_cmd)
        time.sleep(3)
        server_proc = self.popen_ignore_int(server_cmd)
        time.sleep(3)

        # Start the client
        gdb_cmd = [self.gdb_cmd, '-x', gdb_flash_file]
        client_proc = self.popen_ignore_int(gdb_cmd)

        # Wait for 3 seconds (waiting for XTGDB to finish)
        time.sleep(3)

        # At this point, the ELF image is loaded and the program is in
        # execution. Now we can quit the client (xt-gdb) and the server
        # (xt-ocd) as they are not needed anymore. The loaded program
        # (ELF) will continue to run though.
        client_proc.terminate()
        server_proc.terminate()

    def do_debug(self):
        if self.elf_name is None:
            raise ValueError('Cannot debug; elf is missing')
        if self.gdb_cmd is None:
            raise ValueError('Cannot debug; no gdb specified')

        gdb_cmd = [self.gdb_cmd,
                    '-ex', 'target remote :{}'.format(self.gdb_port),
                    self.elf_name]

        # The below statement will consume the "^C" keypress ensuring
        # the python main application doesn't exit. This is important
        # since ^C in gdb means a "halt" operation.
        previous = signal.signal(signal.SIGINT, signal.SIG_IGN)
        try:
            self.check_call(gdb_cmd)
        finally:
            signal.signal(signal.SIGINT, previous)

    def print_gdbserver_message(self, gdb_port):
        print('Intel S1000 GDB server running on port {}'.format(gdb_port))

    def debugserver(self, **kwargs):
        topology_file = kwargs['ocd-topology']
        jtag_instr_file = kwargs['ocd-jtag-instr']

        self.print_gdbserver_message(self.gdb_port)
        server_cmd = [self.xt_ocd_dir,
                        '-c', topology_file,
                        '-I', jtag_instr_file]

        # Note that XTOCD always fails the first time. It has to be
        # relaunched the second time to work.
        self.popen_ignore_int(server_cmd)
        time.sleep(3)
        self.check_call(server_cmd)
