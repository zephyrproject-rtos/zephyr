# Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing Beken BK7258 SoCs through the BootROM over UART.

The protocol is spoken by soc/beken/bk7258/bkfil.py, a standalone tool; this
runner picks the serial port and runs it.
'''

import logging
import os
import subprocess
import sys

from runners.core import FileType, RunnerCaps, ZephyrBinaryRunner
from zephyr_ext_common import ZEPHYR_BASE

BKFIL = os.path.join(ZEPHYR_BASE, 'soc', 'beken', 'bk7258', 'bkfil.py')
DEFAULT_MONITOR_BAUD = '115200'


class BkflashBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for bkfil.py.'''

    def __init__(
        self,
        cfg,
        port=None,
        baud=None,
        offset=None,
        monitor=False,
        monitor_baud=DEFAULT_MONITOR_BAUD,
        dry_run=False,
    ):
        super().__init__(cfg)
        self.port = port
        self.baud = baud
        self.offset = offset
        self.monitor = monitor
        self.monitor_baud = monitor_baud
        self.dry_run = dry_run

    @classmethod
    def name(cls):
        return 'bkflash'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, dry_run=True, file=True)

    @classmethod
    def do_add_parser(cls, parser):
        # Values are passed to bkfil.py as given; it parses and checks them.
        parser.add_argument(
            '--port',
            help='serial port; by default USB serial ports are probed for a BK7258, '
            'which resets each board probed',
        )
        parser.add_argument(
            '--baud-rate',
            help='baud rate for the transfer, default 1000000',
        )
        parser.add_argument(
            '--flash-offset',
            help='flash offset to write at, default 0; any byte offset, flash outside '
            'the programmed range is unchanged',
        )
        parser.add_argument(
            '--monitor', action='store_true', help='show the console after flashing'
        )
        parser.add_argument(
            '--monitor-baud',
            default=DEFAULT_MONITOR_BAUD,
            help='console baud rate for --monitor; the build sets it from the '
            f'zephyr,console current-speed, default {DEFAULT_MONITOR_BAUD}',
        )

    @classmethod
    def do_create(cls, cfg, args):
        return BkflashBinaryRunner(
            cfg,
            port=args.port,
            baud=args.baud_rate,
            offset=args.flash_offset,
            monitor=args.monitor,
            monitor_baud=args.monitor_baud,
            dry_run=args.dry_run,
        )

    def _bkfil(self, port, *args):
        cmd = [sys.executable, BKFIL, '--port', port]
        if self.logger.isEnabledFor(logging.DEBUG):
            cmd.append('--verbose')
        return cmd + list(args)

    def _detect_chip(self, port, failures):
        '''Return "bk7258" if one answers on port, after restarting it.

        Otherwise record why in failures, keyed by port, and return None.'''
        cmd = self._bkfil(port, 'probe')
        try:
            out = self.check_output(cmd, stderr=subprocess.STDOUT)
        except subprocess.CalledProcessError as err:
            out = err.output.decode(errors='replace').rstrip()
            self.logger.debug(out)
            failures[port] = out.splitlines()[-1] if out else f'exit status {err.returncode}'
            return None
        self.logger.debug(out.decode(errors='replace').rstrip())
        return 'bk7258'

    def _resolve_port(self):
        # Probing a port that answers nothing takes a second, and pyserial lists
        # every legacy ttyS* on Linux, so only USB adapters are tried.
        failures = {}
        port = self.resolve_port_by_chip(
            lambda p: self._detect_chip(p, failures), 'bk7258', self.logger, usb_only=True
        )
        if port is None:
            if not failures:
                raise RuntimeError('no USB serial port found; connect the board, or pass --port')
            reasons = ''.join(f'\n  {p}: {why}' for p, why in sorted(failures.items()))
            raise RuntimeError(
                f'no BK7258 answered on any USB serial port:{reasons}\n'
                'A board held by a terminal program does not answer; close it, or pass --port'
            )
        return port

    def do_run(self, command, **kwargs):
        if self.cfg.file:
            if self.cfg.file_type != FileType.BIN:
                raise ValueError('bkflash only writes binary files, already in on-flash format')
            image = self.cfg.file
        else:
            # The build points bin_file at the image with the flash CRC words
            # (zephyr.crc.bin), not at zephyr.bin.
            self.ensure_output('bin')
            image = self.cfg.bin_file

        args = ['write', image]
        if self.offset is not None:
            args += ['--offset', self.offset]
        if self.baud is not None:
            args += ['--baud', self.baud]
        if self.monitor:
            args += ['--monitor', self.monitor_baud]

        if self.dry_run:
            # Probing resets boards, which a dry run must not do.
            port = self.port or '<probed USB serial port>'
        else:
            port = self.port or self._resolve_port()
            self.logger.info(f'Flashing {image} through {port}')
        # Ctrl-C goes to bkfil.py alone, which restores the flash status register
        # if it interrupts a write, and ends the console otherwise.
        self.check_call_ignore_sigint(self._bkfil(port, *args))
