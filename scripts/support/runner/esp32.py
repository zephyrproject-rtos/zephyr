# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing ESP32 devices with esptool/espidf.'''

from os import path
import os

from .core import ZephyrBinaryRunner, get_env_or_bail


class Esp32BinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for espidf.'''

    def __init__(self, elf, device, baud=921600, flash_size='detect',
                 flash_freq='40m', flash_mode='dio', espidf='espidf',
                 debug=False):
        super(Esp32BinaryRunner, self).__init__(debug=debug)
        self.elf = elf
        self.device = device
        self.baud = baud
        self.flash_size = flash_size
        self.flash_freq = flash_freq
        self.flash_mode = flash_mode
        self.espidf = espidf

    def replaces_shell_script(shell_script, command):
        return command == 'flash' and shell_script == 'esp32.sh'

    def create_from_env(command, debug):
        '''Create flasher from environment.

        Required:

        - O: build output directory
        - KERNEL_ELF_NAME: name of kernel binary in ELF format

        Optional:

        - ESP_DEVICE: serial port to flash, default /dev/ttyUSB0
        - ESP_BAUD_RATE: serial baud rate, default 921600
        - ESP_FLASH_SIZE: flash size, default 'detect'
        - ESP_FLASH_FREQ: flash frequency, default '40m'
        - ESP_FLASH_MODE: flash mode, default 'dio'
        - ESP_TOOL: complete path to espidf, or set to 'espidf' to look for it
          in $ESP_IDF_PATH/components/esptool_py/esptool/esptool.py
        '''
        elf = path.join(get_env_or_bail('O'),
                        get_env_or_bail('KERNEL_ELF_NAME'))

        # TODO add sane device defaults on other platforms than Linux.
        device = os.environ.get('ESP_DEVICE', '/dev/ttyUSB0')
        baud = os.environ.get('ESP_BAUD_RATE', '921600')
        flash_size = os.environ.get('ESP_FLASH_SIZE', 'detect')
        flash_freq = os.environ.get('ESP_FLASH_FREQ', '40m')
        flash_mode = os.environ.get('ESP_FLASH_MODE', 'dio')
        espidf = os.environ.get('ESP_TOOL', 'espidf')

        if espidf == 'espidf':
            idf_path = get_env_or_bail('ESP_IDF_PATH')
            espidf = path.join(idf_path, 'components', 'esptool_py', 'esptool',
                               'esptool.py')

        return Esp32BinaryRunner(elf, device, baud=baud,
                                 flash_size=flash_size, flash_freq=flash_freq,
                                 flash_mode=flash_mode, espidf=espidf,
                                 debug=debug)

    def do_run(self, command, **kwargs):
        if command != 'flash':
            raise ValueError('only flash is supported')

        bin_name = path.splitext(self.elf)[0] + path.extsep + 'bin'
        cmd_convert = [self.espidf, '--chip', 'esp32', 'elf2image', self.elf]
        cmd_flash = [self.espidf, '--chip', 'esp32', '--port', self.device,
                     '--baud', self.baud, '--before', 'default_reset',
                     '--after', 'hard_reset', 'write_flash', '-u',
                     '--flash_mode', self.flash_mode,
                     '--flash_freq', self.flash_freq,
                     '--flash_size', self.flash_size,
                     '0x1000', bin_name]

        print("Converting ELF to BIN")
        self.check_call(cmd_convert)

        print("Flashing ESP32 on {} ({}bps)".format(self.device, self.baud))
        self.check_call(cmd_flash)
