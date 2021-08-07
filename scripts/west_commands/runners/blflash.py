# Copyright (c) 2021 Gerson Fernando Budke <nandojve@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0

'''Bouffalo Lab flash tool (blflash) runner for serial boot ROM'''

from runners.core import ZephyrBinaryRunner, RunnerCaps

DEFAULT_BLFLASH_PORT = '/dev/ttyUSB0'
DEFAULT_BLFLASH_SPEED = '2000000'

class BlFlashBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for blflash.'''

    def __init__(self, cfg, blflash='blflash',
                 port=DEFAULT_BLFLASH_PORT,
                 speed=DEFAULT_BLFLASH_SPEED):
        super().__init__(cfg)
        self.blflash = blflash
        self.port = port
        self.speed = speed

    @classmethod
    def name(cls):
        return 'blflash'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--blflash', default='blflash',
                            help='path to blflash, default is blflash')
        parser.add_argument('--port', default=DEFAULT_BLFLASH_PORT,
                            help='serial port to use, default is ' +
                            str(DEFAULT_BLFLASH_PORT))
        parser.add_argument('--speed', default=DEFAULT_BLFLASH_SPEED,
                            help='serial port speed to use, default is ' +
                            DEFAULT_BLFLASH_SPEED)

    @classmethod
    def do_create(cls, cfg, args):
        return BlFlashBinaryRunner(cfg,
                                   blflash=args.blflash,
                                   port=args.port,
                                   speed=args.speed)

    def do_run(self, command, **kwargs):
        self.require(self.blflash)
        self.ensure_output('bin')

        cmd_flash = [self.blflash,
                     'flash',
                     self.cfg.bin_file,
                     '-p', self.port,
                     '-b', self.speed]

        self.check_call(cmd_flash)
