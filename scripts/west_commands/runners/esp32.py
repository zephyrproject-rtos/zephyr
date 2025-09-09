# Copyright (c) 2017 Linaro Limited.
# Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing ESP32 devices with esptool.'''

import os

from runners.core import RunnerCaps, ZephyrBinaryRunner


class Esp32BinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for espidf.'''

    def __init__(self, cfg, device, app_address, erase=False, reset=False,
                 baud=921600, flash_size='detect', flash_freq='40m', flash_mode='dio',
                 espidf=None, encrypt=False, no_stub=False):
        super().__init__(cfg)
        self.elf = cfg.elf_file
        self.app_bin = cfg.bin_file
        self.erase = bool(erase)
        self.reset = bool(reset)
        self.device = device
        self.app_address = app_address
        self.baud = baud
        self.flash_size = flash_size
        self.flash_freq = flash_freq
        self.flash_mode = flash_mode
        self.espidf = espidf
        self.encrypt = encrypt
        self.no_stub = no_stub

    @classmethod
    def name(cls):
        return 'esp32'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, erase=True, reset=True)

    @classmethod
    def do_add_parser(cls, parser):
        # Required
        parser.add_argument('--esp-idf-path', required=True,
                            help='path to ESP-IDF')
        # Optional
        parser.add_argument('--esp-boot-address', default='0x1000',
                            help='bootloader load address')
        parser.add_argument('--esp-partition-table-address', default='0x8000',
                            help='partition table load address')
        parser.add_argument('--esp-app-address', default='0x10000',
                            help='application load address')
        parser.add_argument('--esp-device', default=os.environ.get('ESPTOOL_PORT', None),
                            help='serial port to flash')
        parser.add_argument('--esp-baud-rate', default='921600',
                            help='serial baud rate, default 921600')
        parser.add_argument('--esp-monitor-baud', default='115200',
                            help='serial monitor baud rate, default 115200')
        parser.add_argument('--esp-flash-size', default='detect',
                            help='flash size, default "detect"')
        parser.add_argument('--esp-flash-freq', default='40m',
                            help='flash frequency, default "40m"')
        parser.add_argument('--esp-flash-mode', default='dio',
                            help='flash mode, default "dio"')
        parser.add_argument('--esp-encrypt', default=False, action='store_true',
                            help='Encrypt firmware while flashing (correct efuses required)')
        parser.add_argument('--esp-no-stub', default=False, action='store_true',
                            help='Disable launching the flasher stub, only talk to ROM bootloader')

        parser.set_defaults(reset=True)

    @classmethod
    def do_create(cls, cfg, args):

        return Esp32BinaryRunner(
            cfg, args.esp_device, app_address=args.esp_app_address, erase=args.erase,
            reset=args.reset, baud=args.esp_baud_rate, flash_size=args.esp_flash_size,
            flash_freq=args.esp_flash_freq, flash_mode=args.esp_flash_mode,
            espidf=args.esp_idf_path, encrypt=args.esp_encrypt, no_stub=args.esp_no_stub)

    def do_run(self, command, **kwargs):

        cmd_flash = ['esptool']

        if self.device is not None:
            cmd_flash.extend(['--port', self.device])

        if self.erase is True:
            cmd_erase = cmd_flash + ['erase-flash']
            self.check_call(cmd_erase)

        if self.no_stub is True:
            cmd_flash.extend(['--no-stub'])
        cmd_flash.extend(['--baud', self.baud])
        cmd_flash.extend(['--before', 'default-reset'])
        if self.reset:
            cmd_flash.extend(['--after', 'hard-reset', 'write-flash', '-u'])
        cmd_flash.extend(['--flash-mode', self.flash_mode])
        cmd_flash.extend(['--flash-freq', self.flash_freq])
        cmd_flash.extend(['--flash-size', self.flash_size])

        if self.encrypt:
            cmd_flash.extend(['--encrypt'])

        cmd_flash.extend([self.app_address, self.app_bin])

        self.logger.info(f"Flashing esp32 chip on {self.device} ({self.baud}bps)")
        self.check_call(cmd_flash)
