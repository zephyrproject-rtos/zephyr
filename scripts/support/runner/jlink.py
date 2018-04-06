# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for debugging with J-Link.'''

import os
import tempfile
from .core import ZephyrBinaryRunner, RunnerCaps, BuildConfiguration

DEFAULT_JLINK_GDB_PORT = 2331


class JLinkBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for the J-Link GDB server.'''

    def __init__(self, device,
                 commander='JLinkExe', bin_name=None,
                 flash_addr=0x0, erase=True,
                 gdbserver='JLinkGDBServer', iface='swd', speed='auto',
                 elf_name=None, gdb=None, gdb_port=DEFAULT_JLINK_GDB_PORT,
                 tui=False, debug=False):
        super(JLinkBinaryRunner, self).__init__(debug=debug)
        self.device = device
        self.commander = commander
        self.bin_name = bin_name
        self.flash_addr = flash_addr
        self.erase = erase
        self.gdbserver_cmd = [gdbserver]
        self.iface = iface
        self.speed = speed
        self.elf_name = elf_name
        self.gdb_cmd = [gdb] if gdb is not None else None
        self.gdb_port = gdb_port
        self.tui_arg = ['-tui'] if tui else []

    @classmethod
    def name(cls):
        return 'jlink'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(flash_addr=True)

    @classmethod
    def do_add_parser(cls, parser):
        # Required:
        parser.add_argument('--device', required=True, help='device name')

        # Optional:
        parser.add_argument('--iface', default='swd',
                            help='interface to use, default is swd')
        parser.add_argument('--speed', default='auto',
                            help='interface speed, default is autodetect')
        parser.add_argument('--tui', default=False, action='store_true',
                            help='if given, GDB uses -tui')
        parser.add_argument('--gdbserver', default='JLinkGDBServer',
                            help='GDB server, default is JLinkGDBServer')
        parser.add_argument('--gdb-port', default=DEFAULT_JLINK_GDB_PORT,
                            help='pyocd gdb port, defaults to {}'.format(
                                DEFAULT_JLINK_GDB_PORT))
        parser.add_argument('--commander', default='JLinkExe',
                            help='J-Link Commander, default is JLinkExe')
        parser.add_argument('--erase', default=False, action='store_true',
                            help='if given, mass erase flash before loading')

    @classmethod
    def create_from_args(cls, args):
        build_conf = BuildConfiguration(os.getcwd())
        flash_addr = cls.get_flash_address(args, build_conf)
        return JLinkBinaryRunner(args.device, gdbserver=args.gdbserver,
                                 commander=args.commander,
                                 bin_name=args.kernel_bin,
                                 flash_addr=flash_addr, erase=args.erase,
                                 iface=args.iface, speed=args.speed,
                                 elf_name=args.kernel_elf,
                                 gdb=args.gdb, gdb_port=args.gdb_port,
                                 tui=args.tui, debug=args.verbose)

    def print_gdbserver_message(self):
        print('J-Link GDB server running on port {}'.format(self.gdb_port))

    def do_run(self, command, **kwargs):
        server_cmd = (self.gdbserver_cmd +
                      ['-select', 'usb', # only USB connections supported
                       '-port', str(self.gdb_port),
                       '-if', self.iface,
                       '-speed', self.speed,
                       '-device', self.device,
                       '-silent',
                       '-singlerun'])

        if command == 'flash':
            self.flash(**kwargs)
        elif command == 'debugserver':
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

    def flash(self, **kwargs):
        if self.bin_name is None:
            raise ValueError('Cannot flash; bin_name is missing')

        lines = ['r'] # Reset and halt the target

        if self.erase:
            lines.append('erase') # Erase all flash sectors

        lines.append('loadfile {} 0x{:x}'.format(self.bin_name,
                                                 self.flash_addr))
        lines.append('g') # Start the CPU
        lines.append('q') # Close the connection and quit

        # Don't use NamedTemporaryFile: the resulting file can't be
        # opened again on Windows.
        with tempfile.TemporaryDirectory(suffix='jlink') as d:
            fname = os.path.join(d, 'runner.jlink')
            with open(fname, 'wb') as f:
                f.writelines(bytes(line + '\n', 'utf-8') for line in lines)

            cmd = ([self.commander] +
                   ['-if', self.iface,
                    '-speed', self.speed,
                    '-device', self.device,
                    '-CommanderScript', fname])

            print('Flashing Target Device')
            self.check_call(cmd)
