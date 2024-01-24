# Copyright (c) 2018 Roman Tataurov <diytronic@yandex.ru>
# Modified 2018 Tavish Naruka <tavishnaruka@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
'''Runner for flashing with Black Magic Probe.'''
# https://github.com/blacksphere/blackmagic/wiki

import glob
import os
import signal
import sys
from pathlib import Path

from runners.core import ZephyrBinaryRunner, RunnerCaps

try:
    import serial.tools.list_ports
    MISSING_REQUIREMENTS = False
except ImportError:
    MISSING_REQUIREMENTS = True

# Default path for linux, based on the project udev.rules file.
DEFAULT_LINUX_BMP_PATH = '/dev/ttyBmpGdb'

# Interface descriptor for the GDB port as defined in the BMP firmware.
BMP_GDB_INTERFACE = 'Black Magic GDB Server'

# Product string as defined in the BMP firmware.
BMP_GDB_PRODUCT = "Black Magic Probe"

# BMP vendor and product ID.
BMP_GDB_VID = 0x1d50
BMP_GDB_PID = 0x6018

LINUX_SERIAL_GLOB = '/dev/ttyACM*'
DARWIN_SERIAL_GLOB = '/dev/cu.usbmodem*'

def blackmagicprobe_gdb_serial_linux():
    '''Guess the GDB port on Linux platforms.'''
    if os.path.exists(DEFAULT_LINUX_BMP_PATH):
        return DEFAULT_LINUX_BMP_PATH

    if not MISSING_REQUIREMENTS:
        for port in serial.tools.list_ports.comports():
            if port.interface == BMP_GDB_INTERFACE:
                return port.device

    ports = glob.glob(LINUX_SERIAL_GLOB)
    if not ports:
        raise RuntimeError(
                f'cannot find any valid port matching {LINUX_SERIAL_GLOB}')
    return sorted(ports)[0]

def blackmagicprobe_gdb_serial_darwin():
    '''Guess the GDB port on Darwin platforms.'''
    if not MISSING_REQUIREMENTS:
        bmp_ports = []
        for port in serial.tools.list_ports.comports():
            if port.description and port.description.startswith(
                    BMP_GDB_PRODUCT):
                bmp_ports.append(port.device)
        if bmp_ports:
            return sorted(bmp_ports)[0]

    ports = glob.glob(DARWIN_SERIAL_GLOB)
    if not ports:
        raise RuntimeError(
                f'cannot find any valid port matching {DARWIN_SERIAL_GLOB}')
    return sorted(ports)[0]

def blackmagicprobe_gdb_serial_win32():
    '''Guess the GDB port on Windows platforms.'''
    if not MISSING_REQUIREMENTS:
        bmp_ports = []
        for port in serial.tools.list_ports.comports():
            if port.vid == BMP_GDB_VID and port.pid == BMP_GDB_PID:
                bmp_ports.append(port.device)
        if bmp_ports:
            return sorted(bmp_ports)[0]

    return 'COM1'

def blackmagicprobe_gdb_serial(port):
    '''Guess the GDB port for the probe.

    Return the port to use, in order of priority:
        - the port specified manually
        - the port in the BMP_GDB_SERIAL environment variable
        - a guessed one depending on the host
    '''
    if port:
        return port

    if 'BMP_GDB_SERIAL' in os.environ:
        return os.environ['BMP_GDB_SERIAL']

    platform = sys.platform
    if platform.startswith('linux'):
        return blackmagicprobe_gdb_serial_linux()
    elif platform.startswith('darwin'):
        return blackmagicprobe_gdb_serial_darwin()
    elif platform.startswith('win32'):
        return blackmagicprobe_gdb_serial_win32()
    else:
        raise RuntimeError(f'unsupported platform: {platform}')


class BlackMagicProbeRunner(ZephyrBinaryRunner):
    '''Runner front-end for Black Magic probe.'''

    def __init__(self, cfg, gdb_serial, connect_rst=False):
        super().__init__(cfg)
        self.gdb = [cfg.gdb] if cfg.gdb else None
        # as_posix() because gdb doesn't recognize backslashes as path
        # separators for the 'load' command we execute in bmp_flash().
        #
        # https://github.com/zephyrproject-rtos/zephyr/issues/50789
        self.elf_file = Path(cfg.elf_file).as_posix()
        if cfg.hex_file is not None:
            self.hex_file = Path(cfg.hex_file).as_posix()
        else:
            self.hex_file = None
        self.gdb_serial = blackmagicprobe_gdb_serial(gdb_serial)
        self.logger.info(f'using GDB serial: {self.gdb_serial}')
        if connect_rst:
            self.connect_rst_enable_arg = [
                    '-ex', "monitor connect_rst enable",
                    '-ex', "monitor connect_srst enable",
                    ]
            self.connect_rst_disable_arg = [
                    '-ex', "monitor connect_rst disable",
                    '-ex', "monitor connect_srst disable",
                    ]
        else:
            self.connect_rst_enable_arg = []
            self.connect_rst_disable_arg = []

    @classmethod
    def name(cls):
        return 'blackmagicprobe'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash', 'debug', 'attach'})

    @classmethod
    def do_create(cls, cfg, args):
        return BlackMagicProbeRunner(cfg, args.gdb_serial, args.connect_rst)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--gdb-serial', help='GDB serial port')
        parser.add_argument('--connect-rst', '--connect-srst', action='store_true',
                            help='Assert SRST during connect? (default: no)')

    def bmp_flash(self, command, **kwargs):
        # if hex file is present and signed, use it else use elf file
        if self.hex_file:
            split = self.hex_file.split('.')
            # eg zephyr.signed.hex
            if len(split) >= 3 and split[-2] == 'signed':
                flash_file = self.hex_file
            else:
                flash_file = self.elf_file
        else:
            flash_file = self.elf_file

        if flash_file is None:
            raise ValueError('Cannot flash; elf file is missing')

        command = (self.gdb +
                   ['-ex', "set confirm off",
                    '-ex', "target extended-remote {}".format(
                        self.gdb_serial)] +
                    self.connect_rst_enable_arg +
                   ['-ex', "monitor swdp_scan",
                    '-ex', "attach 1",
                    '-ex', "load {}".format(flash_file),
                    '-ex', "kill",
                    '-ex', "quit",
                    '-silent'])
        self.check_call(command)

    def check_call_ignore_sigint(self, command):
        previous = signal.signal(signal.SIGINT, signal.SIG_IGN)
        try:
            self.check_call(command)
        finally:
            signal.signal(signal.SIGINT, previous)

    def bmp_attach(self, command, **kwargs):
        if self.elf_file is None:
            command = (self.gdb +
                       ['-ex', "set confirm off",
                        '-ex', "target extended-remote {}".format(
                            self.gdb_serial)] +
                        self.connect_rst_disable_arg +
                       ['-ex', "monitor swdp_scan",
                        '-ex', "attach 1"])
        else:
            command = (self.gdb +
                       ['-ex', "set confirm off",
                        '-ex', "target extended-remote {}".format(
                            self.gdb_serial)] +
                        self.connect_rst_disable_arg +
                       ['-ex', "monitor swdp_scan",
                        '-ex', "attach 1",
                        '-ex', "file {}".format(self.elf_file)])
        self.check_call_ignore_sigint(command)

    def bmp_debug(self, command, **kwargs):
        if self.elf_file is None:
            raise ValueError('Cannot debug; elf file is missing')
        command = (self.gdb +
                   ['-ex', "set confirm off",
                    '-ex', "target extended-remote {}".format(
                        self.gdb_serial)] +
                    self.connect_rst_enable_arg +
                   ['-ex', "monitor swdp_scan",
                    '-ex', "attach 1",
                    '-ex', "file {}".format(self.elf_file),
                    '-ex', "load {}".format(self.elf_file)])
        self.check_call_ignore_sigint(command)

    def do_run(self, command, **kwargs):
        if self.gdb is None:
            raise ValueError('Cannot execute; gdb not specified')
        self.require(self.gdb[0])

        if command == 'flash':
            self.bmp_flash(command, **kwargs)
        elif command == 'debug':
            self.bmp_debug(command, **kwargs)
        elif command == 'attach':
            self.bmp_attach(command, **kwargs)
        else:
            self.bmp_flash(command, **kwargs)
