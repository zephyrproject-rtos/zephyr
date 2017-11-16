# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''bossac-specific runner (flash only) for Atmel SAM microcontrollers.'''

from os import path
import os
import platform

from .core import ZephyrBinaryRunner, get_env_or_bail

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
    def handles_command(cls, command):
        return command == 'flash'

    def create_from_env(command, debug):
        '''Create flasher from environment.

        Required:

        - O: build output directory
        - KERNEL_BIN_NAME: name of kernel binary

        Optional:

        - BOSSAC: path to bossac, default is bossac
        - BOSSAC_PORT: serial port to use, default is /dev/ttyACM0
        '''
        bin_name = path.join(get_env_or_bail('O'),
                             get_env_or_bail('KERNEL_BIN_NAME'))
        bossac = os.environ.get('BOSSAC', 'bossac')
        port = os.environ.get('BOSSAC_PORT', DEFAULT_BOSSAC_PORT)
        return BossacBinaryRunner(bin_name, bossac=bossac, port=port,
                                  debug=debug)

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
