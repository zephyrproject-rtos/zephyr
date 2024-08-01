# Copyright (c) 2024, Telink Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

import os
import subprocess

from runners.core import ZephyrBinaryRunner, RunnerCaps, BuildConfiguration

class SCTOOLBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for Senscomm Flash Tool.'''

    def __init__(self, cfg, sctool_path, usb_port, address, erase=False, update_n22=False):
        super().__init__(cfg)
        self.sctool_path = sctool_path
        self.usb_port = usb_port
        self.address = address
        self.erase = bool(erase)
        self.update_n22 = bool(update_n22)

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
        parser.add_argument("--update-n22", action="store_true",
            help="upadet N22 FW")

    @classmethod
    def do_create(cls, cfg, args):
        if args.sctool_path:
            sctool_path = args.sctool_path
        else:
            sctool_path = os.getenv('TELINK_SCTOOL_BASE_DIR')

        return SCTOOLBinaryRunner(cfg, sctool_path, args.usb_port, args.address, args.erase, args.update_n22)

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
        print('Flashing D25 FW...')
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
        print('D25 FW done!')

        if self.update_n22:
            print('Flashing N22 FW...')
            # check N22 firmware
            n22_fw_bin = os.path.dirname(bin_file) + '/n22.bin'
            if not os.path.isfile(n22_fw_bin):
                print(f'N22 FW not exist {n22_fw_bin}')
                exit()
            # erase N22 area
            erase = subprocess.Popen(['./sctool', '-p', self.usb_port, '-da', 'da/da.ram.bin', '-b', '115200', '--manual', 'flash_erase', '0x80300000', '0x100000'], cwd=self.sctool_path)
            if erase.wait():
                exit()
            # flash N22 FW
            flash = subprocess.Popen(['./sctool', '-p', self.usb_port, '-da', 'da/da.ram.bin', '--after', 'hw_reset', '-b', '115200', '--manual', 'flash_write', '0x80300000', n22_fw_bin], cwd=self.sctool_path)
            if flash.wait():
                exit()
            print('N22 FW done!')
