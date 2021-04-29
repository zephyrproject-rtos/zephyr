# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for debugging with J-Link.'''

import argparse
import logging
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

from runners.core import ZephyrBinaryRunner, RunnerCaps

try:
    from pylink.library import Library
    MISSING_REQUIREMENTS = False
except ImportError:
    MISSING_REQUIREMENTS = True

DEFAULT_JLINK_EXE = 'JLink.exe' if sys.platform == 'win32' else 'JLinkExe'
DEFAULT_JLINK_GDB_PORT = 2331

class ToggleAction(argparse.Action):

    def __call__(self, parser, args, ignored, option):
        setattr(args, self.dest, not option.startswith('--no-'))

class JLinkBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for the J-Link GDB server.'''

    def __init__(self, cfg, device,
                 commander=DEFAULT_JLINK_EXE,
                 dt_flash=True, erase=True, reset_after_load=False,
                 iface='swd', speed='auto',
                 gdbserver='JLinkGDBServer',
                 gdb_host='',
                 gdb_port=DEFAULT_JLINK_GDB_PORT,
                 tui=False, tool_opt=[]):
        super().__init__(cfg)
        self.hex_name = cfg.hex_file
        self.bin_name = cfg.bin_file
        self.elf_name = cfg.elf_file
        self.gdb_cmd = [cfg.gdb] if cfg.gdb else None
        self.device = device
        self.commander = commander
        self.dt_flash = dt_flash
        self.erase = erase
        self.reset_after_load = reset_after_load
        self.gdbserver = gdbserver
        self.iface = iface
        self.speed = speed
        self.gdb_host = gdb_host
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
        parser.add_argument('--gdb-host', default='',
                            help='custom gdb host, defaults to the empty string '
                            'and runs a gdb server')
        parser.add_argument('--gdb-port', default=DEFAULT_JLINK_GDB_PORT,
                            help='pyocd gdb port, defaults to {}'.format(
                                DEFAULT_JLINK_GDB_PORT))
        parser.add_argument('--tool-opt', default=[], action='append',
                            help='''Additional options for JLink Commander,
                            e.g. \'-autoconnect 1\' ''')
        parser.add_argument('--commander', default=DEFAULT_JLINK_EXE,
                            help=f'''J-Link Commander, default is
                            {DEFAULT_JLINK_EXE}''')
        parser.add_argument('--reset-after-load', '--no-reset-after-load',
                            dest='reset_after_load', nargs=0,
                            action=ToggleAction,
                            help='reset after loading? (default: no)')

        parser.set_defaults(reset_after_load=False)

    @classmethod
    def do_create(cls, cfg, args):
        return JLinkBinaryRunner(cfg, args.device,
                                 commander=args.commander,
                                 dt_flash=args.dt_flash,
                                 erase=args.erase,
                                 reset_after_load=args.reset_after_load,
                                 iface=args.iface, speed=args.speed,
                                 gdbserver=args.gdbserver,
                                 gdb_host=args.gdb_host,
                                 gdb_port=args.gdb_port,
                                 tui=args.tui, tool_opt=args.tool_opt)

    def print_gdbserver_message(self):
        if not self.thread_info_enabled:
            thread_msg = '; no thread info available'
        elif self.supports_thread_info:
            thread_msg = '; thread info enabled'
        else:
            thread_msg = '; update J-Link software for thread info'
        self.logger.info('J-Link GDB server running on port '
                         f'{self.gdb_port}{thread_msg}')

    @property
    def jlink_version(self):
        # Get the J-Link version as a (major, minor, rev) tuple of integers.
        #
        # J-Link's command line tools provide neither a standalone
        # "--version" nor help output that contains the version. Hack
        # around this deficiency by using the third-party pylink library
        # to load the shared library distributed with the tools, which
        # provides an API call for getting the version.
        if not hasattr(self, '_jlink_version'):
            plat = sys.platform
            if plat.startswith('win32'):
                libname = Library.get_appropriate_windows_sdk_name() + '.dll'
            elif plat.startswith('linux'):
                libname = Library.JLINK_SDK_NAME + '.so'
            elif plat.startswith('darwin'):
                libname = Library.JLINK_SDK_NAME + '.dylib'
            else:
                self.logger.warning(f'unknown platform {plat}; assuming UNIX')
                libname = Library.JLINK_SDK_NAME + '.so'

            lib = Library(dllpath=os.fspath(Path(self.commander).parent /
                                            libname))
            version = int(lib.dll().JLINKARM_GetDLLVersion())
            self.logger.debug('JLINKARM_GetDLLVersion()=%s', version)
            # The return value is an int with 2 decimal digits per
            # version subfield.
            self._jlink_version = (version // 10000,
                                   (version // 100) % 100,
                                   version % 100)

        return self._jlink_version

    @property
    def jlink_version_str(self):
        # Converts the numeric revision tuple to something human-readable.
        if not hasattr(self, '_jlink_version_str'):
            major, minor, rev = self.jlink_version
            rev_str = chr(ord('a') + rev - 1) if rev else ''
            self._jlink_version_str = f'{major}.{minor:02}{rev_str}'
        return self._jlink_version_str

    @property
    def supports_nogui(self):
        # -nogui was introduced in J-Link Commander v6.80
        return self.jlink_version >= (6, 80, 0)

    @property
    def supports_thread_info(self):
        # RTOSPlugin_Zephyr was introduced in 7.11b
        return self.jlink_version >= (7, 11, 2)

    def do_run(self, command, **kwargs):
        if MISSING_REQUIREMENTS:
            raise RuntimeError('one or more Python dependencies were missing; '
                               "see the getting started guide for details on "
                               "how to fix")
        # Convert commander to a real absolute path. We need this to
        # be able to find the shared library that tells us what
        # version of the tools we're using.
        self.commander = os.fspath(
            Path(self.require(self.commander)).resolve())
        self.logger.info(f'JLink version: {self.jlink_version_str}')

        rtos = self.thread_info_enabled and self.supports_thread_info
        plugin_dir = os.fspath(Path(self.commander).parent / 'GDBServer' /
                               'RTOSPlugin_Zephyr')

        server_cmd = ([self.gdbserver] +
                      ['-select', 'usb', # only USB connections supported
                       '-port', str(self.gdb_port),
                       '-if', self.iface,
                       '-speed', self.speed,
                       '-device', self.device,
                       '-silent',
                       '-singlerun'] +
                      (['-nogui'] if self.supports_nogui else []) +
                      (['-rtos', plugin_dir] if rtos else []) +
                      self.tool_opt)

        if command == 'flash':
            self.flash(**kwargs)
        elif command == 'debugserver':
            if self.gdb_host:
                raise ValueError('Cannot run debugserver with --gdb-host')
            self.require(self.gdbserver)
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
                          ['-ex', 'target remote {}:{}'.format(self.gdb_host, self.gdb_port)])
            if command == 'debug':
                client_cmd += ['-ex', 'monitor halt',
                               '-ex', 'monitor reset',
                               '-ex', 'load']
                if self.reset_after_load:
                    client_cmd += ['-ex', 'monitor reset']
            if not self.gdb_host:
                self.require(self.gdbserver)
                self.print_gdbserver_message()
                self.run_server_and_client(server_cmd, client_cmd)
            else:
                self.run_client(client_cmd)

    def flash(self, **kwargs):
        lines = ['r'] # Reset and halt the target

        if self.erase:
            lines.append('erase') # Erase all flash sectors

        # Get the build artifact to flash, prefering .hex over .bin
        if self.hex_name is not None and os.path.isfile(self.hex_name):
            flash_file = self.hex_name
            flash_cmd = f'loadfile {self.hex_name}'
        elif self.bin_name is not None and os.path.isfile(self.bin_name):
            if self.dt_flash:
                flash_addr = self.flash_address_from_build_conf(self.build_conf)
            else:
                flash_addr = 0
            flash_file = self.bin_name
            flash_cmd = f'loadfile {self.bin_name} 0x{flash_addr:x}'
        else:
            err = 'Cannot flash; no hex ({}) or bin ({}) files found.'
            raise ValueError(err.format(self.hex_name, self.bin_name))

        # Flash the selected build artifact
        lines.append(flash_cmd)

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

        self.logger.debug('JLink commander script:\n' +
                          '\n'.join(lines))

        # Don't use NamedTemporaryFile: the resulting file can't be
        # opened again on Windows.
        with tempfile.TemporaryDirectory(suffix='jlink') as d:
            fname = os.path.join(d, 'runner.jlink')
            with open(fname, 'wb') as f:
                f.writelines(bytes(line + '\n', 'utf-8') for line in lines)

            cmd = ([self.commander] +
                   (['-nogui', '1'] if self.supports_nogui else []) +
                   ['-if', self.iface,
                    '-speed', self.speed,
                    '-device', self.device,
                    '-CommanderScript', fname] +
                   (['-nogui', '1'] if self.supports_nogui else []) +
                   self.tool_opt)

            self.logger.info('Flashing file: {}'.format(flash_file))
            kwargs = {}
            if not self.logger.isEnabledFor(logging.DEBUG):
                kwargs['stdout'] = subprocess.DEVNULL
            self.check_call(cmd, **kwargs)
