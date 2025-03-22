# Copyright (c) 2025 sensry.io
# SPDX-License-Identifier: Apache-2.0

'''Runner for Sensry SY1xx Soc Flashing Tool.'''

import importlib.util
import subprocess
import sys

from runners.core import RunnerCaps, ZephyrBinaryRunner


class Sy1xxBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for Sensry SY1xx Soc'''

    def __init__(self, cfg, serial=None):
        super().__init__(cfg)
        self.bin_file = cfg.bin_file
        self.serial = serial

    @classmethod
    def name(cls):
        return 'sy1xx'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument(
            '--serial', required=True, dest='serial', help='Device identifier such as /dev/ttyUSB0'
        )

    @classmethod
    def do_create(cls, cfg, args):
        # make sure the ganymed tools are installed
        if importlib.util.find_spec('ganymed') is None:
            subprocess.check_call([sys.executable, '-m', 'pip', 'install', 'ganymed'])

        return Sy1xxBinaryRunner(cfg, args.serial)

    def do_run(self, command, **kwargs):
        if command == 'flash':
            self.flash(**kwargs)

    def flash(self, **kwargs):
        self.logger.info(f'Flashing file: {self.bin_file} to {self.serial}')

        from ganymed.bootloader import Bootloader

        # convert binary to application ganymed-image
        application_gnm = Bootloader.convert_zephyr_bin(self.bin_file)

        # create the loader
        flash_loader = Bootloader()

        # connect to serial
        flash_loader.connect(self.serial)

        # set the controller into bootloader mode
        flash_loader.enter_loading_mode()

        # clear the internal flash
        flash_loader.clear_mram()

        # write the new binary
        flash_loader.write_image(application_gnm)

        self.logger.info('Flashing SY1xx finished. You may reset the device.')
