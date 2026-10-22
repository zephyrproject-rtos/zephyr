# SPDX-FileCopyrightText: 2026 Texas Instruments Incorporated
# SPDX-License-Identifier: Apache-2.0

'''Runner for booting TI K3 (AM62L) boards over USB DFU using dfu-util.

The boot ROM exposes a DFU device ("bootloader" alt setting) when the board
is strapped for USB DFU boot mode. The boot flow is:

  1. ROM receives tiboot3.bin (signed TF-A BL1 + TIFS + board config).
  2. BL1 runs, then hands off to ROM to receive the next stage, the ROM
     re-enumerates on USB.
  3. ROM receives tispl.bin (signed TF-A BL31 + Zephyr application).

The images are loaded into RAM, so they are not persistent across a power
cycle.
'''

import re
import subprocess
import sys
import time
from pathlib import Path

from runners.core import RunnerCaps, ZephyrBinaryRunner

DEFAULT_PID = '0451:6165'
DEFAULT_ALT = 'bootloader'


class TiK3DfuBinaryRunner(ZephyrBinaryRunner):
    def __init__(
        self,
        cfg,
        dev_id,
        alt,
        tiboot3,
        tispl,
        exe='dfu-util',
        power_cycle_cmd=None,
        timeout=30.0,
        settle=1.0,
    ):
        super().__init__(cfg)
        self.dev_id = dev_id
        self.alt = alt
        self.tiboot3 = tiboot3
        self.tispl = tispl
        self.exe = exe
        self.power_cycle_cmd = power_cycle_cmd
        self.timeout = timeout
        self.settle = settle

    @classmethod
    def name(cls):
        return 'ti_k3_dfu'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, dev_id=True)

    @classmethod
    def dev_id_help(cls) -> str:
        return f'USB VID:PID of the ROM DFU device (default {DEFAULT_PID}).'

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--pid', dest='dev_id', help=cls.dev_id_help())
        parser.add_argument(
            '--alt',
            default=DEFAULT_ALT,
            help=f'DFU alt setting name or number (default "{DEFAULT_ALT}")',
        )
        parser.add_argument(
            '--tiboot3', help='first stage image, default is <build>/zephyr/tiboot3.bin'
        )
        parser.add_argument(
            '--tispl', help='second stage image, default is <build>/zephyr/tispl.bin'
        )
        parser.add_argument(
            '--dfu-util', default='dfu-util', help='dfu-util executable; defaults to "dfu-util"'
        )
        parser.add_argument(
            '--power-cycle-cmd',
            help='shell command that power cycles the board '
            '(must leave it in DFU boot mode). It is run '
            'before flashing. If omitted, the user is '
            'asked to power cycle the board manually '
            'when the DFU device is not present.',
        )
        parser.add_argument(
            '--timeout',
            type=float,
            default=30.0,
            help='seconds to wait for the DFU device to (re)appear, default 30',
        )

    @classmethod
    def do_create(cls, cfg, args):
        build = Path(cfg.build_dir) / 'zephyr'
        tiboot3 = args.tiboot3 or str(build / 'tiboot3.bin')
        tispl = args.tispl or str(build / 'tispl.bin')
        return TiK3DfuBinaryRunner(
            cfg,
            args.dev_id or DEFAULT_PID,
            args.alt,
            tiboot3,
            tispl,
            exe=args.dfu_util,
            power_cycle_cmd=args.power_cycle_cmd,
            timeout=args.timeout,
        )

    def list_devnums(self):
        '''Return the set of DFU device numbers matching the alt setting.'''
        out = subprocess.run(
            [self.exe, '-d', f',{self.dev_id}', '-l'], capture_output=True, check=False
        )
        text = out.stdout.decode(sys.getdefaultencoding(), errors='replace')
        try:
            pat = f'alt={int(self.alt)},'
        except ValueError:
            pat = f'name="{self.alt}"'
        devs = set()
        for line in text.splitlines():
            if line.startswith('Found DFU') and pat in line:
                m = re.search(r'devnum=(\d+)', line)
                devs.add(m.group(1) if m else line)
        return devs

    def wait_for_device(self, ignore=frozenset()):
        deadline = time.monotonic() + self.timeout
        while time.monotonic() < deadline:
            devs = self.list_devnums() - ignore
            if devs:
                time.sleep(self.settle)  # let the ROM finish enumerating
                return devs
            time.sleep(self.settle)
        raise RuntimeError(f'timed out waiting for DFU device ({self.dev_id}, alt "{self.alt}")')

    def download(self, img):
        self.check_call([self.exe, '-d', f',{self.dev_id}', '-a', self.alt, '-D', img])

    def do_run(self, command, **kwargs):
        self.require(self.exe)
        for img in (self.tiboot3, self.tispl):
            if not Path(img).is_file():
                raise RuntimeError(
                    f'{img} not found. Build with '
                    '-DCONFIG_BUILD_WITH_TFA=y '
                    '-DCONFIG_SOC_FAMILY_TI_K3_BUILD_TIBOOT3=y '
                    '-DCONFIG_SOC_FAMILY_TI_K3_BUILD_TISPL=y, and fetch '
                    'blobs with "west blobs fetch hal_ti".'
                )

        # The ROM only accepts a fresh boot sequence after a power cycle.
        if self.power_cycle_cmd:
            self.logger.info('power cycling board: %s', self.power_cycle_cmd)
            subprocess.run(self.power_cycle_cmd, shell=True, check=True)
            time.sleep(1.0)
        elif not self.list_devnums():
            print('Please power cycle the board in USB DFU boot mode...')

        first = self.wait_for_device()
        self.logger.info('flashing stage 1: %s', self.tiboot3)
        self.download(self.tiboot3)

        # BL1 hands off to ROM, which re-enumerates for the next stage.
        self.wait_for_device(ignore=first)
        self.logger.info('flashing stage 2: %s', self.tispl)
        self.download(self.tispl)
