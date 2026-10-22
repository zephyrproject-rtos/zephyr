# Copyright (c) 2026 James Bennion-Pedley
#
# SPDX-License-Identifier: Apache-2.0

'''WCH runner for WCHISP USB Bootloader Tool'''

import argparse

from runners.core import RunnerCaps, RunnerConfig, ZephyrBinaryRunner


class WchispBinaryRunner(ZephyrBinaryRunner):
    '''Runner for WCH devices using wchisp via the USB bootloader'''

    def __init__(
        self,
        cfg: RunnerConfig,
        wchisp: str,
        erase: bool,
        reset: bool,
        transport_usb: bool,
        transport_serial: bool,
    ):
        super().__init__(cfg)

        self.wchisp = wchisp
        self.erase = erase
        self.reset = reset
        self.transport_usb = transport_usb
        self.transport_serial = transport_serial

    @classmethod
    def name(cls):
        return 'wchisp'

    @classmethod
    def capabilities(cls) -> RunnerCaps:
        return RunnerCaps(commands={'flash'}, flash_addr=True, erase=True, reset=True)

    @classmethod
    def do_add_parser(cls, parser: argparse.ArgumentParser):
        parser.add_argument('--wchisp', default='wchisp', help='path to the wchisp binary')
        parser.add_argument(
            '--usb', action=argparse.BooleanOptionalAction, help='Use USB Transport'
        )
        parser.add_argument(
            '--serial', action=argparse.BooleanOptionalAction, help='Use Serial Transport'
        )
        parser.set_defaults(usb=False)
        parser.set_defaults(serial=False)
        parser.set_defaults(erase=True)
        parser.set_defaults(reset=True)

    @classmethod
    def do_create(cls, cfg: RunnerConfig, args: argparse.Namespace):
        return WchispBinaryRunner(
            cfg,
            wchisp=args.wchisp,
            erase=args.erase,
            reset=args.reset,
            transport_usb=args.usb,
            transport_serial=args.serial,
        )

    def do_run(self, command: str, **kwargs):
        self.require(self.wchisp)

        if command == 'flash':
            self.flash()
        else:
            raise ValueError('BUG: unhandled command f{command}')

    def flash(self):
        self.ensure_output('bin')

        cmd = [self.wchisp]

        if self.transport_usb:
            cmd.append('--usb')

        if self.transport_serial:
            cmd.append('--serial')

        cmd.append('flash')

        if not self.erase:
            cmd.append('--no-erase')

        if not self.reset:
            cmd.append('--no-reset')

        cmd.append(self.cfg.bin_file)

        self.check_output(cmd)
