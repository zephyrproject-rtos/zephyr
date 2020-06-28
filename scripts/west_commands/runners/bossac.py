# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''bossac-specific runner (flash only) for Atmel SAM microcontrollers.'''

import subprocess

import platform

from runners.core import ZephyrBinaryRunner, RunnerCaps, BuildConfiguration

DEFAULT_BOSSAC_PORT = '/dev/ttyACM0'


class BossacBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for bossac.'''

    def __init__(self, cfg, bossac='bossac', port=DEFAULT_BOSSAC_PORT,
                 offset=None, flash_address=None):
        super().__init__(cfg)
        self.bossac = bossac
        self.port = port
        self.offset = offset
        self.flash_address = flash_address

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
        parser.add_argument('--offset', default=None,
                            help='start erase/write/read/verify operation '
                                 'at flash OFFSET; OFFSET must be aligned '
                                 ' to a flash page boundary')
        parser.add_argument('--bossac-port', default='/dev/ttyACM0',
                            help='serial port to use, default is /dev/ttyACM0')

    @classmethod
    def do_create(cls, cfg, args):
        # BOSSA means there's a bootloader so always fetch the flash address
        args.dt_flash = True
        build_conf = BuildConfiguration(cfg.build_dir)
        flash_address = cls.get_flash_address(args, build_conf, None)

        return BossacBinaryRunner(cfg, bossac=args.bossac,
                                  port=args.bossac_port, offset=args.offset,
                                  flash_address=flash_address)

    def read_help(self):
        """Run bossac --help and return the output as a list of lines"""
        self.require(self.bossac)
        try:
            self.check_output([self.bossac, '--help'])
            return []
        except subprocess.CalledProcessError as ex:
            return ex.output.decode().split('\n')

    def supports(self, flag):
        """Check if bossac supports a flag by searching the help"""
        for line in self.read_help():
            if flag in line:
                return True
        return False

    def get_offset(self, supports_offset):
        """Validates and returns the flash offset"""
        if supports_offset:
            if self.offset is not None:
                return self.offset
            if self.flash_address is not None:
                return self.flash_address

            self.logger.warning(
                'This version of BOSSA supports the --offset flag but' +
                ' no offset was supplied. If flashing fails, then' +
                ' please specify the size of the bootloader by adding' +
                ' the --offset= flag to board_runner_args in board.cmake')
            return self.offset

        if self.offset is not None:
            self.logger.warning(
                'This version of BOSSA does not support the --offset flag.' +
                ' Please see' +
                ' https://github.com/zephyrproject-rtos/sdk-ng/issues/234' +
                ' which tracks updating the Zephyr SDK.')

        return self.offset

    def do_run(self, command, **kwargs):
        if platform.system() == 'Windows':
            msg = 'CAUTION: BOSSAC runner not support on Windows!'
            raise NotImplementedError(msg)

        self.require(self.bossac)

        if platform.system() == 'Linux':
            self.require('stty')
            cmd_stty = ['stty', '-F', self.port, 'raw', 'ispeed', '1200',
                        'ospeed', '1200', 'cs8', '-cstopb', 'ignpar', 'eol', '255',
                        'eof', '255']
            self.check_call(cmd_stty)

        cmd_flash = [self.bossac, '-p', self.port, '-R', '-e', '-w', '-v',
                     '-b', self.cfg.bin_file]

        offset = self.get_offset(self.supports('--offset'))

        if offset is not None:
            cmd_flash += ['-o', '%s' % offset]

        self.check_call(cmd_flash)
