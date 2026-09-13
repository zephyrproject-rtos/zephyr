# Copyright (c) 2026 Anuj Deshpande
#
# SPDX-License-Identifier: Apache-2.0

'''vegadude-specific runner (flash only) for C-DAC VEGA / THEJAS32 boards.'''

from runners.core import RunnerCaps, ZephyrBinaryRunner

DEFAULT_VEGADUDE_PORT = '/dev/ttyUSB0'


class VegadudeBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for vegadude.

    vegadude (https://github.com/rnayabed/vegadude) uploads a raw binary to
    C-DAC VEGA microprocessor boards such as the ARIES v3.0 over a serial port
    using the XMODEM protocol. The board must be in UART boot mode (BOOT SEL
    jumper open) and reset so that its boot ROM is waiting for a transfer.
    '''

    def __init__(self, cfg, vegadude='vegadude', port=DEFAULT_VEGADUDE_PORT):
        super().__init__(cfg)
        self.vegadude = vegadude
        self.port = port

    @classmethod
    def name(cls):
        return 'vegadude'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument(
            '--vegadude', default='vegadude', help='path to vegadude, default is vegadude'
        )
        parser.add_argument(
            '--vegadude-port',
            default=DEFAULT_VEGADUDE_PORT,
            help='serial port to use, default is ' + DEFAULT_VEGADUDE_PORT,
        )

    @classmethod
    def do_create(cls, cfg, args):
        return VegadudeBinaryRunner(cfg, vegadude=args.vegadude, port=args.vegadude_port)

    def do_run(self, command, **kwargs):
        self.require(self.vegadude)
        self.ensure_output('bin')

        cmd = [
            self.vegadude,
            '--target-path',
            self.port,
            '--binary-path',
            self.cfg.bin_file,
            '--aries',
            '--start-after-upload',
        ]

        self.logger.info('Reset the board into UART boot mode (BOOT SEL open) before flashing')
        self.check_call(cmd)
