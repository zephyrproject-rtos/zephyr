# Copyright (c) 2026 James Bennion-Pedley
#
# SPDX-License-Identifier: Apache-2.0

'''WCH runner for Wlink CLI'''

import argparse

from runners.core import RunnerCaps, RunnerConfig, ZephyrBinaryRunner


class WlinkBinaryRunner(ZephyrBinaryRunner):
    '''Runner for WCH devices using wlink'''

    def __init__(
        self,
        cfg: RunnerConfig,
        wlink: str,
        chip: str,
        speed: str,
        erase: bool,
    ):
        super().__init__(cfg)

        self.wlink = wlink
        self.chip = chip
        self.speed = speed
        self.erase = erase

    @classmethod
    def name(cls):
        return 'wlink'

    @classmethod
    def capabilities(cls) -> RunnerCaps:
        return RunnerCaps(commands={'flash'}, flash_addr=True, erase=True)

    @classmethod
    def do_add_parser(cls, parser: argparse.ArgumentParser):
        parser.add_argument('--wlink', default='wlink', help='path to the wlink binary')
        parser.add_argument('--chip', help='target SOC family')
        parser.add_argument(
            '--speed',
            default='high',
            help='interface speed, default is high',
            choices=['low', 'medium', 'high'],
        )

    @classmethod
    def do_create(cls, cfg: RunnerConfig, args: argparse.Namespace):
        return WlinkBinaryRunner(
            cfg, wlink=args.wlink, chip=args.chip, speed=args.speed, erase=args.erase
        )

    def do_run(self, command: str, **kwargs):
        self.require(self.wlink)

        if command == 'flash':
            self.flash()
        else:
            raise ValueError('BUG: unhandled command f{command}')

    def flash(self):
        self.ensure_output('bin')

        cmd = [self.wlink]
        cmd.extend(['--chip', self.chip])
        cmd.extend(['--speed', self.speed])

        flash_addr = self.flash_address_from_build_conf(self.build_conf)
        cmd.extend(['flash', '--address', f'0x{flash_addr:x}'])

        if self.erase:
            cmd.append('--erase')

        cmd.append(self.cfg.bin_file)

        self.check_output(cmd)
