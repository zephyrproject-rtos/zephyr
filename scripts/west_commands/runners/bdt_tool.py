# Copyright (c) 2023-2024, Telink Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

import os
import subprocess

from runners.core import ZephyrBinaryRunner, RunnerCaps, BuildConfiguration

class BDTBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for BDT.'''

    def __init__(self, cfg, bdt_path, address, erase=False):
        super().__init__(cfg)
        self.bdt_path = bdt_path
        self.address = address
        self.erase = bool(erase)

    @classmethod
    def name(cls):
        return 'bdt_tool'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, erase=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--bdt-path', default='',
            help='path to BDT installation root')
        parser.add_argument('--address', default='0x0',
            help='start flash address to write')

    @classmethod
    def do_create(cls, cfg, args):
        if args.bdt_path:
            bdt_path = args.bdt_path
        else:
            bdt_path = os.getenv('TELINK_BDT_BASE_DIR')
        return BDTBinaryRunner(cfg, bdt_path, args.address, args.erase)

    def do_run(self, command, **kwargs):
        self.require(self.bdt_path + '/bdt')
        if command == "flash":
            self._flash()
        else:
            self.logger.error(f'{command} not supported!')

    def _flash(self):
        # obtain build configuration
        build_conf = BuildConfiguration(self.cfg.build_dir)
        # get chip
        soc_type = None
        if build_conf['CONFIG_SOC_RISCV_TELINK_B92']:
            soc_type = 'B92'
            print('Telink B92')
        if build_conf['CONFIG_SOC_RISCV_TELINK_B91']:
            soc_type = '9518'
            print('Telink B91')
        if build_conf['CONFIG_SOC_RISCV_TELINK_TL321X']:
            soc_type = 'TL321X'
            print('Telink TL321')
        if soc_type is None:
            print('only Telink chips are supported!')
            exit()
        # get flash size
        flash_size = str(build_conf['CONFIG_FLASH_SIZE']) + 'K'
        # get binary file
        bin_file = os.path.abspath(self.cfg.bin_file)
        # adjust flash offset for MCU boot application
        if 'CONFIG_BOOTLOADER_MCUBOOT' in build_conf:
            if self.address == '0x0' and build_conf['CONFIG_BOOTLOADER_MCUBOOT']:
                self.address = hex(build_conf['CONFIG_FLASH_LOAD_OFFSET'])
                print('set address offset for MCUBOOT application to', self.address)
            else:
                print('default application offset', self.address)
        else:
            print('default application offset', self.address)
        # activate chip
        print('activating...')
        activate = subprocess.Popen(['./bdt', soc_type, 'ac'], cwd=self.bdt_path)
        if activate.wait():
            exit()
        # unlock flash only B92
        if soc_type in ('B92', 'TL321X'):
            print('unlocking flash...')
            unlock = subprocess.Popen(['./bdt', soc_type, 'ulf'], cwd=self.bdt_path)
            if unlock.wait():
                exit()
        # erase flash
        if self.erase:
            print('erasing...')
            erase = subprocess.Popen(['./bdt', soc_type, 'wf', '0', '-e', '-s', flash_size], cwd=self.bdt_path)
            if erase.wait():
                exit()
        # flash
        print('flashing...')
        flash = subprocess.Popen(['./bdt', soc_type, 'wf', self.address, '-i', bin_file], cwd=self.bdt_path)
        if flash.wait():
            exit()
        # reset chip
        print('resetting...')
        reset = subprocess.Popen(['./bdt', soc_type, 'rst'] ,cwd=self.bdt_path)
        if reset.wait():
            exit()
        print('done!')
