# Copyright (c) 2017 Linaro Limited.
# Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing ESP32 devices with esptool.'''

import os
import shutil
from pathlib import Path

from runners.core import RunnerCaps, ZephyrBinaryRunner


class Esp32BinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for espidf.'''

    def __init__(self, cfg, args):
        super().__init__(cfg)
        self.elf = cfg.elf_file
        self.app_bin = cfg.bin_file
        self.erase = bool(args.erase)
        self.reset = bool(args.reset)
        self.device = args.esp_device
        self.app_address = args.esp_app_address
        self.baud = args.esp_baud_rate
        self.flash_size = args.esp_flash_size
        self.flash_freq = args.esp_flash_freq
        self.flash_mode = args.esp_flash_mode
        self.espidf = args.esp_idf_path
        self.encrypt = args.esp_encrypt
        self.no_stub = args.esp_no_stub
        self.reset_type = args.reset_type
        self.skip_flashed = bool(args.esp_skip_flashed)
        self.diff = bool(args.esp_diff)
        self.no_progress = bool(args.esp_no_progress)

    @classmethod
    def name(cls):
        return 'esp32'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, erase=True, reset=True, reset_types=True,
                          reset_types_supported=['hard-reset', 'watchdog-reset'])

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
        parser.add_argument('--esp-skip-flashed', default=False, action='store_true',
                            help='Skip flashing if the image is already in flash '
                                 '(verified with an MD5 check on the device)')
        parser.add_argument('--esp-diff', default=False, action='store_true',
                            help='Flash only regions that differ from the previous flash. '
                                 'Trusts the locally cached image, so use only when the '
                                 'flash was not modified by any other tool since.')
        parser.add_argument('--esp-no-progress', default=False, action='store_true',
                            help='Suppress esptool progress output')

        parser.set_defaults(reset=True)

    def _get_prev_bin_path(self):
        return self.app_bin + '.prev'

    def _get_soc_name(self):
        try:
            return self.build_conf.get('CONFIG_SOC', 'esp32')
        except FileNotFoundError:
            return 'esp32'

    @staticmethod
    def _detect_chip_name(port):
        '''Connect to port and return the chip name esptool reports.'''
        from esptool import detect_chip
        esp = None
        try:
            esp = detect_chip(port, connect_attempts=2)
            return esp.CHIP_NAME
        finally:
            if esp is not None and esp._port:
                esp._port.close()

    def _resolve_port_by_soc(self):
        '''Probe serial ports and return the first one whose connected chip
        matches CONFIG_SOC. Returns None to let esptool auto-detect when no
        matching port is found.'''
        try:
            import esptool  # noqa: F401
        except ImportError as err:
            self.logger.debug(f'SoC port matching unavailable: {err}')
            return None

        # Native Espressif USB devices use VID 0x303A; probe them first.
        ESPRESSIF_VID = 0x303A
        port = self.resolve_port_by_chip(
            self._detect_chip_name,
            self.normalize_chip(self._get_soc_name()),
            self.logger,
            priority_vids=[ESPRESSIF_VID])
        if port is None:
            self.logger.warning(
                f'No port with a {self._get_soc_name()} chip found; '
                'falling back to esptool auto-detection')
        return port

    def _append_flash_opts(self, cmd_flash):
        if self.no_progress:
            cmd_flash.append('-p')

        prev_bin = self._get_prev_bin_path()
        use_diff = self.diff and not self.erase and os.path.exists(prev_bin) and not self.encrypt
        use_skip = self.skip_flashed and not self.encrypt

        if use_diff:
            cmd_flash.extend(['--diff-with', prev_bin])
        elif use_skip:
            cmd_flash.append('-s')

    def _cache_flashed_bin(self):
        if self.diff and not self.encrypt and os.path.exists(self.app_bin):
            prev_bin = self._get_prev_bin_path()
            Path(prev_bin).parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(self.app_bin, prev_bin)

    @classmethod
    def do_create(cls, cfg, args):
        return Esp32BinaryRunner(cfg, args)

    def do_run(self, command, **kwargs):

        cmd_flash = ['esptool']

        if self.device is None:
            self.device = self._resolve_port_by_soc()

        if self.device is not None:
            cmd_flash.extend(['--port', self.device])

        if self.erase is True:
            cmd_erase = cmd_flash + ['erase-flash']
            self.check_call(cmd_erase)

        if self.no_stub is True:
            cmd_flash.extend(['--no-stub'])
        cmd_flash.extend(['--baud', self.baud])
        cmd_flash.extend(['--before', 'default-reset'])
        if self.reset_type is not None and not self.reset:
            self.logger.warning(
                '--reset-type is ignored because reset is disabled (--no-reset)')
        if self.reset:
            cmd_flash.extend(['--after', self.reset_type or 'hard-reset'])
        cmd_flash.extend(['write-flash', '-u'])
        cmd_flash.extend(['--flash-mode', self.flash_mode])
        cmd_flash.extend(['--flash-freq', self.flash_freq])
        cmd_flash.extend(['--flash-size', self.flash_size])

        if self.encrypt:
            cmd_flash.extend(['--encrypt'])

        self._append_flash_opts(cmd_flash)

        cmd_flash.extend([self.app_address, self.app_bin])

        device_str = self.device if self.device else 'auto-detect'
        self.logger.info(
            f"Flashing {self._get_soc_name()} chip on {device_str} ({self.baud}bps)")

        self.check_call(cmd_flash)

        self._cache_flashed_bin()
