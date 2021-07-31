# Copyright (c) 2017 Linaro Limited.
# Copyright (c) 2019 Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with adafruit-nrfutil.'''

import os
from pathlib import Path
import shlex
import subprocess
import sys
from re import fullmatch, escape

from runners.core import ZephyrBinaryRunner, RunnerCaps


class AdafruitNrfUtilBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for adafruit-nrfutil.'''

    def __init__(self, cfg, port):
        super().__init__(cfg)
        self.hex_ = cfg.hex_file
        self.port = port

    @classmethod
    def name(cls):
        return 'adafruit-nrfutil'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, erase=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--port', required=True,
                            help="""Serial port COM Port to which the
                            device is connected.""")

    @classmethod
    def do_create(cls, cfg, args):
        return AdafruitNrfUtilBinaryRunner(cfg, args.port)

    def program_hex(self):
        self.logger.info('Creating package from hex: {}'.format(self.hex_))

        # What nrfjprog commands do we need to flash this target?
        zip = self.hex_ + '.zip'
        program_commands = [
            ['adafruit-nrfutil', '--verbose',
            'dfu', 'genpkg', '--dev-type', '0x0052', '--application',
            self.hex_, zip],
            ['adafruit-nrfutil', '--verbose',
            'dfu', 'serial', '-b', '115200', '--singlebank',
            '-p', self.port,
            '--package', zip],
            ]

        for command in program_commands:
            self.check_call(command)

    def do_run(self, command, **kwargs):
        self.require('adafruit-nrfutil')
        self.ensure_output('hex')

        self.program_hex()

