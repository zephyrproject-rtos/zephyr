# Copyright (c) 2024, Telink Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

import os
import subprocess

from runners.core import ZephyrBinaryRunner, RunnerCaps, BuildConfiguration

class SCTOOLBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for Senscomm Flash Tool.'''

    SERIAL_BAUDE_RATE = '2000000'

    def __init__(self, cfg, sctool_path, usb_port, address, erase=False, update_n22=False):
        super().__init__(cfg)
        self.sctool_path = sctool_path
        self.usb_port = usb_port
        self.baude_rate = self.SERIAL_BAUDE_RATE
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
        parser.add_argument('--address', default=None,
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
        # get flash size
        flash_size = hex(build_conf['CONFIG_FLASH_SIZE'] * 1024)
        # get flash base address
        if self.address is not None: # if user defined use it
            print(f'User defined flash base address: {self.address}')
            flash_base = self.address
        else:
            # calculate flash base address from configuration
            if build_conf.getboolean('CONFIG_MCUBOOT'): # mcuboot
                print('MCUboot app!')
                flash_base = hex(build_conf['CONFIG_FLASH_BASE_ADDRESS'])
            elif build_conf.getboolean('CONFIG_BOOTLOADER_MCUBOOT'): # MCU boot app
                print('MCUboot bootloader app!')
                flash_base = hex(build_conf['CONFIG_FLASH_BASE_ADDRESS'] +
                                 build_conf['CONFIG_FLASH_LOAD_OFFSET'])
            else: # standalone app
                print('Standalone app!')
                flash_base = hex(build_conf['CONFIG_FLASH_BASE_ADDRESS'])
        # get N22 partition address
        n22_partition_addr = hex(build_conf['CONFIG_TELINK_W91_N22_PARTITION_ADDR'] + int(flash_base, 16))
        # download RAM
        pre_erase = subprocess.Popen(['./sctool', '-p', self.usb_port, '-da', 'da/da.ram.bin', '-b', self.baude_rate, '--before', 'hw_reset', '--after', 'no_reset', 'upload_da'], cwd=self.sctool_path)
        if pre_erase.wait():
            exit()
        print('Flashing D25 FW...')
        # erase flash if needed
        if self.erase:
            print('Erasing flash...')
            flash_base_erase = hex(build_conf['CONFIG_FLASH_BASE_ADDRESS'])
            erase = subprocess.Popen(['./sctool', '-p', self.usb_port, '-da', 'da/da.ram.bin', '--manual', 'flash_erase', flash_base_erase, flash_size], cwd=self.sctool_path)
            if erase.wait():
                exit()
        # get binary file
        bin_file = os.path.abspath(self.cfg.bin_file)
        # flash
        flash = subprocess.Popen(['./sctool', '-p', self.usb_port, '-da', 'da/da.ram.bin', '--after', 'hw_reset', '--manual', 'flash_write', flash_base, bin_file], cwd=self.sctool_path)
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
            # flash N22 FW
            flash = subprocess.Popen(['./sctool', '-p', self.usb_port, '-da', 'da/da.ram.bin', '--after', 'hw_reset', '--manual', 'flash_write', n22_partition_addr, n22_fw_bin], cwd=self.sctool_path)
            if flash.wait():
                exit()
            print('N22 FW done!')
