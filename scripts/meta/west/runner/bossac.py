# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''bossac-specific runner (flash only) for Atmel SAM microcontrollers.'''

import platform

from .core import ZephyrBinaryRunner, RunnerCaps

DEFAULT_BOSSAC_PORT = '/dev/ttyACM0'


class BossacBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for bossac.'''

    def __init__(self, bin_name, bossac='bossac',
                 port=DEFAULT_BOSSAC_PORT, debug=False):
        super(BossacBinaryRunner, self).__init__(debug=debug)
        self.bin_name = bin_name
        self.bossac = bossac
        self.port = port

    @classmethod
    def name(cls):
        return 'bossac'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--bossac', default='bossac',
                            help='path to bossac, default is bossac')
        parser.add_argument('--bossac-port', default='/dev/ttyACM0',
                            help='serial port to use, default is /dev/ttyACM0')

    @classmethod
    def create_from_args(command, args):
        return BossacBinaryRunner(args.kernel_bin, bossac=args.bossac,
                                  port=args.bossac_port, debug=args.verbose)

    def do_run(self, command, **kwargs):
        if platform.system() != 'Linux':
            msg = 'CAUTION: No flash tool for your host system found!'
            raise NotImplementedError(msg)

        cmd_stty = ['stty', '-F', self.port, 'raw', 'ispeed', '1200',
                    'ospeed', '1200', 'cs8', '-cstopb', 'ignpar', 'eol', '255',
                    'eof', '255']
        cmd_flash = [self.bossac, '-p', self.port, '-R', '-e', '-w', '-v',
                     '-b', self.bin_name]

        self.check_call(cmd_stty)
        self.check_call(cmd_flash)
