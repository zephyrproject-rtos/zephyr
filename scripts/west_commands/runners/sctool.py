# Copyright (c) 2024, Telink Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

import os
import subprocess

from runners.core import ZephyrBinaryRunner, RunnerCaps, BuildConfiguration

class SCTOOLBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for Senscomm Flash Tool.'''

    def __init__(self, cfg, sctool_path, usb_port, address, erase=False):
        super().__init__(cfg)
        self.sctool_path = sctool_path
        self.usb_port = usb_port
        self.address = address
        self.erase = bool(erase)

    @classmethod
    def name(cls):
        return 'sctool'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, erase=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--sctool-path', default='',
            help='path to sctool installation root')
        parser.add_argument('--usb-port', default='/dev/ttyUSB0',
            help='USB CDC to which device is attached')
        parser.add_argument('--address', default='0x80000000',
            help='start flash address to write')

    @classmethod
    def do_create(cls, cfg, args):
        if args.sctool_path:
            sctool_path = args.sctool_path
        else:
            sctool_path = os.getenv('TELINK_SCTOOL_BASE_DIR')

        return SCTOOLBinaryRunner(cfg, sctool_path, args.usb_port, args.address, args.erase)

    def do_run(self, command, **kwargs):
        self.require(self.sctool_path + '/sctool')
        if command == "flash":
            self._flash()
        else:
            self.logger.error(f'{command} not supported!')

    def _flash(self):
        # obtain build configuration
        build_conf = BuildConfiguration(self.cfg.build_dir)
        if not build_conf['CONFIG_SOC_RISCV_TELINK_W91']:
            print('only Telink W91 chip is supported!')
            exit()
        # download RAM
        pre_erase = subprocess.Popen(['./sctool', '-p', self.usb_port, '-da', 'da/da.ram.bin', '--before', 'hw_reset', '--after', 'no_reset', '-b', '115200', 'upload_da'], cwd=self.sctool_path)
        if pre_erase.wait():
            exit()
        # get flash size
        flash_size = hex(build_conf['CONFIG_FLASH_SIZE'] * 1024)
        # erase flash
        if self.erase:
            erase = subprocess.Popen(['./sctool', '-p', self.usb_port, '-da', 'da/da.ram.bin', '-b', '115200', '--manual', 'flash_erase', '0x80000000', flash_size], cwd=self.sctool_path)
            if erase.wait():
                exit()
        # get binary file
        bin_file = os.path.abspath(self.cfg.bin_file)
        # flash
        flash = subprocess.Popen(['./sctool', '-p', self.usb_port, '-da', 'da/da.ram.bin', '--after', 'hw_reset', '-b', '115200', '--manual', 'flash_write', self.address, bin_file], cwd=self.sctool_path)
        if flash.wait():
            exit()
