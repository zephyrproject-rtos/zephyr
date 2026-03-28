# Copyright (c) 2017 Linaro Limited.
# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

'''Dediprog (dpcmd) flash only runner for SPI chips.'''

import platform
import subprocess

from runners.core import RunnerCaps, ZephyrBinaryRunner

DPCMD_EXE = 'dpcmd.exe' if platform.system() == 'Windows' else 'dpcmd'
DEFAULT_MAX_RETRIES = 3

class DediProgBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for DediProg (dpcmd).'''

    def __init__(self, cfg, spi_image, vcc, retries):
        super().__init__(cfg)
        self.spi_image = spi_image
        self.vcc = vcc
        self.dpcmd_retries = retries

    @classmethod
    def name(cls):
        return 'dediprog'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--spi-image', required=True,
                            help='path to SPI image')
        parser.add_argument('--vcc',
                            help='VCC (0=3.5V, 1=2.5V, 2=1.8V)')
        parser.add_argument('--retries', default=5,
                            help='Number of retries (default 5)')

    @classmethod
    def do_create(cls, cfg, args):
        return DediProgBinaryRunner(cfg,
                                    spi_image=args.spi_image,
                                    vcc=args.vcc,
                                    retries=args.retries)

    def do_run(self, command, **kwargs):
        self.require(DPCMD_EXE)
        cmd_flash = [DPCMD_EXE, '--auto', self.spi_image]

        if self.vcc:
            cmd_flash.append('--vcc')
            cmd_flash.append(self.vcc)

        # Allow to flash images smaller than flash device capacity
        cmd_flash.append('-x')
        cmd_flash.append('ff')

        cmd_flash.append('--silent')
        cmd_flash.append('--verify')

        try:
            max_retries = int(self.dpcmd_retries)
        except ValueError:
            max_retries = DEFAULT_MAX_RETRIES

        retries = 0
        while retries <= max_retries:
            try:
                self.check_call(cmd_flash)
            except subprocess.CalledProcessError as cpe:
                retries += 1
                if retries > max_retries:
                    raise cpe
                else:
                    continue

            break
