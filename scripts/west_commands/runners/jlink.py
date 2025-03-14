# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for debugging with J-Link.'''

import argparse
import ipaddress
import logging
import os
import shlex
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path

from runners.core import FileType, RunnerCaps, ZephyrBinaryRunner

try:
    import pylink
    from pylink.library import Library
    MISSING_REQUIREMENTS = False
except ImportError:
    MISSING_REQUIREMENTS = True

# Populated in do_add_parser()
DEFAULT_JLINK_EXE = None
DEFAULT_JLINK_GDB_PORT = 2331
DEFAULT_JLINK_RTT_PORT = 19021

def is_ip(ip):
    if not ip:
        return False
    try:
        ipaddress.ip_address(ip.split(':')[0])
    except ValueError:
        return False
    return True

def is_tunnel(tunnel):
    return tunnel.startswith("tunnel:") if tunnel else False

class ToggleAction(argparse.Action):

    def __call__(self, parser, args, ignored, option):
        setattr(args, self.dest, not option.startswith('--no-'))

class JLinkBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for the J-Link GDB server.'''

    def __init__(self, cfg, device, dev_id=None,
                 commander=DEFAULT_JLINK_EXE,
                 dt_flash=True, erase=True, reset=False,
                 iface='swd', speed='auto', flash_script = None,
                 loader=None,
                 gdbserver='JLinkGDBServer',
                 gdb_host='',
                 gdb_port=DEFAULT_JLINK_GDB_PORT,
                 rtt_port=DEFAULT_JLINK_RTT_PORT,
                 tui=False, tool_opt=None):
        super().__init__(cfg)
        self.file = cfg.file
        self.file_type = cfg.file_type
        self.hex_name = cfg.hex_file
        self.bin_name = cfg.bin_file
        self.elf_name = cfg.elf_file
        self.gdb_cmd = [cfg.gdb] if cfg.gdb else None
        self.device = device
        self.dev_id = dev_id
        self.commander = commander
        self.flash_script = flash_script
        self.dt_flash = dt_flash
        self.erase = erase
        self.reset = reset
        self.gdbserver = gdbserver
        self.iface = iface
        self.speed = speed
        self.gdb_host = gdb_host
        self.gdb_port = gdb_port
        self.tui_arg = ['-tui'] if tui else []
        self.loader = loader
        self.rtt_port = rtt_port

        self.tool_opt = []
        if tool_opt is not None:
            for opts in [shlex.split(opt) for opt in tool_opt]:
                self.tool_opt += opts

    @classmethod
    def name(cls):
        return 'jlink'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash', 'debug', 'debugserver', 'attach', 'rtt'},
                          dev_id=True, flash_addr=True, erase=True, reset=True,
                          tool_opt=True, file=True, rtt=True)

    @classmethod
    def dev_id_help(cls) -> str:
        return '''Device identifier. Use it to select the J-Link Serial Number
                  of the device connected over USB. If the J-Link is connected over ip,
                  the Device identifier is the ip.'''

    @classmethod
    def tool_opt_help(cls) -> str:
        return "Additional options for JLink Commander, e.g. '-autoconnect 1'"

    @staticmethod
    def default_jlink():
        global DEFAULT_JLINK_EXE

        if sys.platform == 'win32':
            # JLink.exe can collide with the JDK executable of the same name
            # Locate the executable using the registry
            try:
                import winreg

                # Note that when multiple JLink versions are installed on the
                # machine this points to the one that was installed
                # last, and not to the latest version.
                key = winreg.OpenKeyEx(
                    winreg.HKEY_CURRENT_USER, r"Software\SEGGER\J-Link")
                DEFAULT_JLINK_EXE = (
                    Path(winreg.QueryValueEx(key, "InstallPath")[0])
                    / "JLink.exe")
            except Exception:
                # Not found via the registry, hope that $PATH is correct
                DEFAULT_JLINK_EXE = "JLink.exe"
        else:
            DEFAULT_JLINK_EXE = "JLinkExe"

    @classmethod
    def do_add_parser(cls, parser):

        # Find the default JLink executable
        cls.default_jlink()

        # Required:
        parser.add_argument('--device', required=True, help='device name')

        # Optional:
        parser.add_argument('--loader', required=False, dest='loader',
                            help='specifies a loader type')
        parser.add_argument('--id', required=False, dest='dev_id',
                            help='obsolete synonym for -i/--dev-id')
        parser.add_argument('--iface', default='swd',
                            help='interface to use, default is swd')
        parser.add_argument('--speed', default='auto',
                            help='interface speed, default is autodetect')
        parser.add_argument('--flash-script', default=None,
                            help='Custom flashing script, default is None')
        parser.add_argument('--tui', default=False, action='store_true',
                            help='if given, GDB uses -tui')
        parser.add_argument('--gdbserver', default='JLinkGDBServer',
                            help='GDB server, default is JLinkGDBServer')
        parser.add_argument('--gdb-host', default='',
                            help='custom gdb host, defaults to the empty string '
                            'and runs a gdb server')
        parser.add_argument('--gdb-port', default=DEFAULT_JLINK_GDB_PORT,
                            help=f'pyocd gdb port, defaults to {DEFAULT_JLINK_GDB_PORT}')
        parser.add_argument('--commander', default=DEFAULT_JLINK_EXE,
                            help=f'''J-Link Commander, default is
                            {DEFAULT_JLINK_EXE}''')
        parser.add_argument('--reset-after-load', '--no-reset-after-load',
                            dest='reset', nargs=0,
                            action=ToggleAction,
                            help='obsolete synonym for --reset/--no-reset')
        parser.add_argument('--rtt-client', default='JLinkRTTClient',
                            help='RTT client, default is JLinkRTTClient')
        parser.add_argument('--rtt-port', default=DEFAULT_JLINK_RTT_PORT,
                            help=f'jlink rtt port, defaults to {DEFAULT_JLINK_RTT_PORT}')

        parser.set_defaults(reset=False)

    @classmethod
    def do_create(cls, cfg, args):
        return JLinkBinaryRunner(cfg, args.device,
                                 dev_id=args.dev_id,
                                 commander=args.commander,
                                 dt_flash=args.dt_flash,
                                 erase=args.erase,
                                 reset=args.reset,
                                 iface=args.iface, speed=args.speed,
                                 flash_script=args.flash_script,
                                 gdbserver=args.gdbserver,
                                 loader=args.loader,
                                 gdb_host=args.gdb_host,
                                 gdb_port=args.gdb_port,
                                 rtt_port=args.rtt_port,
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

    def print_rttserver_message(self):
        self.logger.info(f'J-Link RTT server running on port {self.rtt_port}')

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
            # pylink 0.14.0/0.14.1 exposes JLink SDK DLL (libjlinkarm) in
            # JLINK_SDK_STARTS_WITH, while other versions use JLINK_SDK_NAME
            if pylink.__version__ in ('0.14.0', '0.14.1'):
                sdk = Library.JLINK_SDK_STARTS_WITH
            else:
                sdk = Library.JLINK_SDK_NAME

            plat = sys.platform
            if plat.startswith('win32'):
                libname = Library.get_appropriate_windows_sdk_name() + '.dll'
            elif plat.startswith('linux'):
                libname = sdk + '.so'
            elif plat.startswith('darwin'):
                libname = sdk + '.dylib'
            else:
                self.logger.warning(f'unknown platform {plat}; assuming UNIX')
                libname = sdk + '.so'

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

    @property
    def supports_loader(self):
        return self.jlink_version >= (7, 70, 4)

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
        self.logger.debug(f'JLink executable: {self.commander}')
        self.logger.info(f'JLink version: {self.jlink_version_str}')

        rtos = self.thread_info_enabled and self.supports_thread_info
        plugin_dir = os.fspath(Path(self.commander).parent / 'GDBServer' /
                               'RTOSPlugin_Zephyr')
        big_endian = self.build_conf.getboolean('CONFIG_BIG_ENDIAN')

        server_cmd = (
            [self.gdbserver]
            + [
                '-select',
                ('ip' if (is_ip(self.dev_id) or is_tunnel(self.dev_id)) else 'usb')
                + (f'={self.dev_id}' if self.dev_id else ''),
            ]
            + ['-port', str(self.gdb_port)]
            + ['-if', self.iface]
            + ['-speed', self.speed]
            + ['-device', self.device]
            + ['-silent']
            + ['-endian', 'big' if big_endian else 'little']
            + ['-singlerun']
            + (['-nogui'] if self.supports_nogui else [])
            + (['-rtos', plugin_dir] if rtos else [])
            + ['-rtttelnetport', str(self.rtt_port)]
            + self.tool_opt
        )

        if command == 'flash':
            self.flash(**kwargs)
        elif command == 'debugserver':
            if self.gdb_host:
                raise ValueError('Cannot run debugserver with --gdb-host')
            self.require(self.gdbserver)
            self.print_gdbserver_message()
            self.check_call(server_cmd)
        elif command == 'rtt':
            self.print_gdbserver_message()
            self.print_rttserver_message()
            server_cmd += ['-nohalt']
            server_proc = self.popen_ignore_int(server_cmd)
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                # wait for the port to be open
                while server_proc.poll() is None:
                    try:
                        sock.connect(('localhost', self.rtt_port))
                        break
                    except ConnectionRefusedError:
                        time.sleep(0.1)
                self.run_telnet_client('localhost', self.rtt_port, sock)
            except Exception as e:
                self.logger.error(e)
            finally:
                server_proc.terminate()
                server_proc.wait()
        else:
            if self.gdb_cmd is None:
                raise ValueError('Cannot debug; gdb is missing')
            if self.file is not None:
                if self.file_type != FileType.ELF:
                    raise ValueError('Cannot debug; elf file required')
                elf_name = self.file
            elif self.elf_name is None:
                raise ValueError('Cannot debug; elf is missing')
            else:
                elf_name = self.elf_name
            client_cmd = (self.gdb_cmd +
                          self.tui_arg +
                          [elf_name] +
                          ['-ex', f'target remote {self.gdb_host}:{self.gdb_port}'])
            if command == 'debug':
                client_cmd += ['-ex', 'monitor halt',
                               '-ex', 'monitor reset',
                               '-ex', 'load']
                if self.reset:
                    client_cmd += ['-ex', 'monitor reset']
            if not self.gdb_host:
                self.require(self.gdbserver)
                self.print_gdbserver_message()
                self.run_server_and_client(server_cmd, client_cmd)
            else:
                self.run_client(client_cmd)

    def get_default_flash_commands(self):
        lines = [
            'ExitOnError 1',  # Treat any command-error as fatal
            'r',  # Reset and halt the target
            'BE' if self.build_conf.getboolean('CONFIG_BIG_ENDIAN') else 'LE'
        ]

        if self.erase:
            lines.append('erase') # Erase all flash sectors

        # Get the build artifact to flash
        if self.file is not None:
            # use file provided by the user
            if not os.path.isfile(self.file):
                err = 'Cannot flash; file ({}) not found'
                raise ValueError(err.format(self.file))

            flash_file = self.file

            if self.file_type == FileType.HEX:
                flash_cmd = f'loadfile "{self.file}"'
            elif self.file_type == FileType.BIN:
                if self.dt_flash:
                    flash_addr = self.flash_address_from_build_conf(self.build_conf)
                else:
                    flash_addr = 0
                flash_cmd = f'loadfile "{self.file}" 0x{flash_addr:x}'
            else:
                err = 'Cannot flash; jlink runner only supports hex and bin files'
                raise ValueError(err)

        else:
            # Use hex, bin or elf file provided by the buildsystem.
            # Preferring .hex over .bin and .elf
            if self.hex_name is not None and os.path.isfile(self.hex_name):
                flash_file = self.hex_name
                flash_cmd = f'loadfile "{self.hex_name}"'
            # Preferring .bin over .elf
            elif self.bin_name is not None and os.path.isfile(self.bin_name):
                if self.dt_flash:
                    flash_addr = self.flash_address_from_build_conf(self.build_conf)
                else:
                    flash_addr = 0
                flash_file = self.bin_name
                flash_cmd = f'loadfile "{self.bin_name}" 0x{flash_addr:x}'
            elif self.elf_name is not None and os.path.isfile(self.elf_name):
                flash_file = self.elf_name
                flash_cmd = f'loadfile "{self.elf_name}"'
            else:
                err = 'Cannot flash; no hex ({}), bin ({}) or elf ({}) files found.'
                raise ValueError(err.format(self.hex_name, self.bin_name, self.elf_name))

        # Flash the selected build artifact
        lines.append(flash_cmd)

        if self.reset:
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
        return flash_file, lines

    def run_flash_cmd(self, fname, flash_file, **kwargs):
        loader_details = ""
        if self.supports_loader and self.loader:
            loader_details = "?" + self.loader

        cmd = (
            [self.commander]
            + (
                ['-IP', f'{self.dev_id}']
                if (is_ip(self.dev_id) or is_tunnel(self.dev_id))
                else (['-USB', f'{self.dev_id}'] if self.dev_id else [])
            )
            + (['-nogui', '1'] if self.supports_nogui else [])
            + ['-if', self.iface]
            + ['-speed', self.speed]
            + ['-device', self.device + loader_details]
            + ['-CommanderScript', fname]
            + (['-nogui', '1'] if self.supports_nogui else [])
            + self.tool_opt
        )

        if flash_file:
            self.logger.info(f'Flashing file: {flash_file}')
        kwargs = {}
        if not self.logger.isEnabledFor(logging.DEBUG):
            kwargs['stdout'] = subprocess.DEVNULL
        self.check_call(cmd, **kwargs)

    def flash(self, **kwargs):
        fname = self.flash_script
        if fname is None:
            # Don't use NamedTemporaryFile: the resulting file can't be
            # opened again on Windows.
            with tempfile.TemporaryDirectory(suffix='jlink') as d:
                flash_file, lines = self.get_default_flash_commands()
                fname = os.path.join(d, 'runner.jlink')
                with open(fname, 'wb') as f:
                    f.writelines(bytes(line + '\n', 'utf-8') for line in lines)

                self.run_flash_cmd(fname, flash_file, **kwargs)
        else:
            self.run_flash_cmd(fname, None, **kwargs)
