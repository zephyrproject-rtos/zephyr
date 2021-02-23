# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for debugging with J-Link.'''

import argparse
import os
import platform
import re
import shlex
import sys
import tempfile

from packaging import version
from runners.core import ZephyrBinaryRunner, RunnerCaps, \
    BuildConfiguration
from subprocess import TimeoutExpired

DEFAULT_JLINK_EXE = 'JLink.exe' if sys.platform == 'win32' else 'JLinkExe'
DEFAULT_JLINK_GDB_PORT = 2331

class ToggleAction(argparse.Action):

    def __call__(self, parser, args, ignored, option):
        setattr(args, self.dest, not option.startswith('--no-'))

class JLinkBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for the J-Link GDB server.'''

    def __init__(self, cfg, device,
                 commander=DEFAULT_JLINK_EXE,
                 flash_addr=0x0, erase=True, reset_after_load=False,
                 iface='swd', speed='auto',
                 gdbserver='JLinkGDBServer', gdb_port=DEFAULT_JLINK_GDB_PORT,
                 tui=False, tool_opt=[]):
        super().__init__(cfg)
        self.hex_name = cfg.hex_file
        self.bin_name = cfg.bin_file
        self.elf_name = cfg.elf_file
        self.gdb_cmd = [cfg.gdb] if cfg.gdb else None
        self.device = device
        self.commander = commander
        self.flash_addr = flash_addr
        self.erase = erase
        self.reset_after_load = reset_after_load
        self.gdbserver = gdbserver
        self.iface = iface
        self.speed = speed
        self.gdb_port = gdb_port
        self.tui_arg = ['-tui'] if tui else []

        self.tool_opt = []
        for opts in [shlex.split(opt) for opt in tool_opt]:
            self.tool_opt += opts

    @classmethod
    def name(cls):
        return 'jlink'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash', 'debug', 'debugserver', 'attach'},
                          flash_addr=True, erase=True)

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
        parser.add_argument('--tool-opt', default=[], action='append',
                            help='''Additional options for JLink Commander,
                            e.g. \'-autoconnect 1\' ''')
        parser.add_argument('--commander', default=DEFAULT_JLINK_EXE,
                            help='J-Link Commander, default is JLinkExe')
        parser.add_argument('--reset-after-load', '--no-reset-after-load',
                            dest='reset_after_load', nargs=0,
                            action=ToggleAction,
                            help='reset after loading? (default: no)')

        parser.set_defaults(reset_after_load=False)

    @classmethod
    def do_create(cls, cfg, args):
        build_conf = BuildConfiguration(cfg.build_dir)
        flash_addr = cls.get_flash_address(args, build_conf)
        return JLinkBinaryRunner(cfg, args.device,
                                 commander=args.commander,
                                 flash_addr=flash_addr, erase=args.erase,
                                 reset_after_load=args.reset_after_load,
                                 iface=args.iface, speed=args.speed,
                                 gdbserver=args.gdbserver,
                                 gdb_port=args.gdb_port,
                                 tui=args.tui, tool_opt=args.tool_opt)

    def print_gdbserver_message(self):
        self.logger.info('J-Link GDB server running on port {}'.
                         format(self.gdb_port))

    def read_version(self):
        '''Read the J-Link Commander version output.

        J-Link Commander does not provide neither a stand-alone version string
        output nor command line parameter help output. To find the version, we
        launch it using a bogus command line argument (to get it to fail) and
        read the version information provided to stdout.

        A timeout is used since the J-Link Commander takes up to a few seconds
        to exit upon failure.'''
        if platform.system() == 'Windows':
            # The check below does not work on Microsoft Windows
            return ''

        self.require(self.commander)
        # Match "Vd.dd" substring
        ver_re = re.compile(r'\s+V([.0-9]+)[a-zA-Z]*\s+', re.IGNORECASE)
        cmd = ([self.commander] + ['-bogus-argument-that-does-not-exist'])
        try:
            self.check_output(cmd, timeout=1)
        except TimeoutExpired as e:
            ver_m = ver_re.search(e.output.decode('utf-8'))
            if ver_m:
                return ver_m.group(1)
            else:
                return ''

    def supports_nogui(self):
        ver = self.read_version()
        # -nogui was introduced in J-Link Commander v6.80
        return version.parse(ver) >= version.parse("6.80")

    def do_run(self, command, **kwargs):
        server_cmd = ([self.gdbserver] +
                      ['-select', 'usb', # only USB connections supported
                       '-port', str(self.gdb_port),
                       '-if', self.iface,
                       '-speed', self.speed,
                       '-device', self.device,
                       '-silent',
                       '-singlerun'] +
                      self.tool_opt)

        if command == 'flash':
            self.flash(**kwargs)
        elif command == 'debugserver':
            self.require(self.gdbserver)
            self.print_gdbserver_message()
            self.check_call(server_cmd)
        else:
            self.require(self.gdbserver)
            if self.gdb_cmd is None:
                raise ValueError('Cannot debug; gdb is missing')
            if self.elf_name is None:
                raise ValueError('Cannot debug; elf is missing')
            client_cmd = (self.gdb_cmd +
                          self.tui_arg +
                          [self.elf_name] +
                          ['-ex', 'target remote :{}'.format(self.gdb_port)])
            if command == 'debug':
                client_cmd += ['-ex', 'monitor halt',
                               '-ex', 'monitor reset',
                               '-ex', 'load']
                if self.reset_after_load:
                    client_cmd += ['-ex', 'monitor reset']

            self.print_gdbserver_message()
            self.run_server_and_client(server_cmd, client_cmd)

    def flash(self, **kwargs):
        self.require(self.commander)

        lines = ['r'] # Reset and halt the target

        if self.erase:
            lines.append('erase') # Erase all flash sectors

        # Get the build artifact to flash, prefering .hex over .bin
        if self.hex_name is not None and os.path.isfile(self.hex_name):
            flash_file = self.hex_name
            flash_fmt = 'loadfile {}'
        elif self.bin_name is not None and os.path.isfile(self.bin_name):
            flash_file = self.bin_name
            flash_fmt = 'loadfile {} 0x{:x}'
        else:
            err = 'Cannot flash; no hex ({}) or bin ({}) files found.'
            raise ValueError(err.format(self.hex_name, self.bin_name))

        # Flash the selected build artifact
        lines.append(flash_fmt.format(flash_file, self.flash_addr))

        if self.reset_after_load:
            lines.append('r') # Reset and halt the target

        lines.append('g') # Start the CPU

        # Reset the Debug Port CTRL/STAT register
        # Under normal operation this is done automatically, but if other
        # JLink tools are running, it is not performed.
        # The J-Link scripting layer chains commands, meaning that writes are
        # not actually performed until after the next operation. After writing
        # the register, read it back to perform this flushing.
        lines.append('writeDP 1 0')
        lines.append('readDP 1')

        lines.append('q') # Close the connection and quit

        self.logger.debug('JLink commander script:')
        self.logger.debug('\n'.join(lines))

        # Don't use NamedTemporaryFile: the resulting file can't be
        # opened again on Windows.
        with tempfile.TemporaryDirectory(suffix='jlink') as d:
            fname = os.path.join(d, 'runner.jlink')
            with open(fname, 'wb') as f:
                f.writelines(bytes(line + '\n', 'utf-8') for line in lines)

            if self.supports_nogui():
                nogui = ['-nogui', '1']
            else:
                nogui = []

            cmd = ([self.commander] + nogui +
                   ['-if', self.iface,
                    '-speed', self.speed,
                    '-device', self.device,
                    '-CommanderScript', fname] +
                   self.tool_opt)

            self.logger.info('Flashing file: {}'.format(flash_file))
            self.check_call(cmd)
