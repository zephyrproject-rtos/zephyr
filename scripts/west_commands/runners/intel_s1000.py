# Copyright (c) 2018 Intel Corporation.
# Copyright 2018 Open Source Foundries Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for debugging and flashing Intel S1000 CRB'''
from os import path
import time
import signal

from runners.core import ZephyrBinaryRunner

DEFAULT_XT_GDB_PORT = 20000


class IntelS1000BinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for Intel S1000.'''

    def __init__(self, cfg, xt_ocd_dir,
                 ocd_topology, ocd_jtag_instr, gdb_flash_file,
                 gdb_port=DEFAULT_XT_GDB_PORT):
        super().__init__(cfg)
        self.board_dir = cfg.board_dir
        self.elf_name = cfg.elf_file
        self.gdb_cmd = cfg.gdb
        self.xt_ocd_dir = xt_ocd_dir
        self.ocd_topology = ocd_topology
        self.ocd_jtag_instr = ocd_jtag_instr
        self.gdb_flash_file = gdb_flash_file
        self.gdb_port = gdb_port

    @classmethod
    def name(cls):
        return 'intel_s1000'

    @classmethod
    def do_add_parser(cls, parser):
        # Optional
        parser.add_argument(
            '--xt-ocd-dir', default='/opt/tensilica/xocd-12.0.4/xt-ocd',
            help='ocd-dir, defaults to /opt/tensilica/xocd-12.0.4/xt-ocd')
        parser.add_argument(
            '--ocd-topology', default='topology_dsp0_flyswatter2.xml',
            help='ocd-topology, defaults to topology_dsp0_flyswatter2.xml')
        parser.add_argument(
            '--ocd-jtag-instr', default='dsp0_gdb.txt',
            help='ocd-jtag-instr, defaults to dsp0_gdb.txt')
        parser.add_argument(
            '--gdb-flash-file', default='load_elf.txt',
            help='gdb-flash-file, defaults to load_elf.txt')
        parser.add_argument(
            '--gdb-port', default=DEFAULT_XT_GDB_PORT,
            help='xt-gdb port, defaults to 20000')

    @classmethod
    def do_create(cls, cfg, args):
        return IntelS1000BinaryRunner(
            cfg, args.xt_ocd_dir,
            args.ocd_topology, args.ocd_jtag_instr, args.gdb_flash_file,
            gdb_port=args.gdb_port)

    def do_run(self, command, **kwargs):
        self.require(self.xt_ocd_dir)
        kwargs['ocd-topology'] = path.join(self.board_dir, 'support',
                                           self.ocd_topology)
        kwargs['ocd-jtag-instr'] = path.join(self.board_dir, 'support',
                                             self.ocd_jtag_instr)
        kwargs['gdb-flash-file'] = path.join(self.board_dir, 'support',
                                             self.gdb_flash_file)

        if command == 'flash':
            self.flash(**kwargs)
        elif command == 'debugserver':
            self.debugserver(**kwargs)
        else:
            self.do_debug(**kwargs)

    def flash(self, **kwargs):
        if self.gdb_cmd is None:
            raise ValueError('Cannot debug; no gdb specified')
        self.require(self.gdb_cmd)
        topology_file = kwargs['ocd-topology']
        jtag_instr_file = kwargs['ocd-jtag-instr']
        gdb_flash_file = kwargs['gdb-flash-file']

        self.log_gdbserver_message(self.gdb_port)
        server_cmd = [self.xt_ocd_dir,
                      '-c', topology_file,
                      '-I', jtag_instr_file]

        # Start the server
        # Note that XTOCD takes a few seconds to execute and always fails the
        # first time. It has to be relaunched the second time to work.
        server_proc = self.popen_ignore_int(server_cmd)
        time.sleep(6)
        server_proc.terminate()
        server_proc = self.popen_ignore_int(server_cmd)
        time.sleep(6)

        # Start the client
        gdb_cmd = [self.gdb_cmd, '-x', gdb_flash_file]
        client_proc = self.popen_ignore_int(gdb_cmd)

        # Wait for 3 seconds (waiting for XTGDB to finish loading the image)
        time.sleep(3)

        # At this point, the ELF image is loaded and the program is in
        # execution. Now we can quit the client (xt-gdb) and the server
        # (xt-ocd) as they are not needed anymore. The loaded program
        # (ELF) will continue to run though.
        client_proc.terminate()
        server_proc.terminate()

    def do_debug(self, **kwargs):
        if self.elf_name is None:
            raise ValueError('Cannot debug; elf is missing')
        if self.gdb_cmd is None:
            raise ValueError('Cannot debug; no gdb specified')
        self.require(self.gdb_cmd)

        topology_file = kwargs['ocd-topology']
        jtag_instr_file = kwargs['ocd-jtag-instr']

        self.log_gdbserver_message(self.gdb_port)
        server_cmd = [self.xt_ocd_dir,
                      '-c', topology_file,
                      '-I', jtag_instr_file]

        # Start the server
        # Note that XTOCD takes a few seconds to execute and always fails the
        # first time. It has to be relaunched the second time to work.
        server_proc = self.popen_ignore_int(server_cmd)
        time.sleep(6)
        server_proc.terminate()
        server_proc = self.popen_ignore_int(server_cmd)
        time.sleep(6)

        gdb_cmd = [self.gdb_cmd,
                   '-ex', 'target remote :{}'.format(self.gdb_port),
                   self.elf_name]

        # Start the client
        # The below statement will consume the "^C" keypress ensuring
        # the python main application doesn't exit. This is important
        # since ^C in gdb means a "halt" operation.
        previous = signal.signal(signal.SIGINT, signal.SIG_IGN)
        try:
            self.check_call(gdb_cmd)
        finally:
            signal.signal(signal.SIGINT, previous)
            server_proc.terminate()
            server_proc.wait()

    def debugserver(self, **kwargs):
        topology_file = kwargs['ocd-topology']
        jtag_instr_file = kwargs['ocd-jtag-instr']

        self.log_gdbserver_message(self.gdb_port)
        server_cmd = [self.xt_ocd_dir,
                      '-c', topology_file,
                      '-I', jtag_instr_file]

        # Note that XTOCD takes a few seconds to execute and always fails the
        # first time. It has to be relaunched the second time to work.
        server_proc = self.popen_ignore_int(server_cmd)
        time.sleep(6)
        server_proc.terminate()
        self.check_call(server_cmd)

    def log_gdbserver_message(self, gdb_port):
        self.logger.info('Intel S1000 GDB server running on port {}'.
                         format(gdb_port))
