# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing ESP32 devices with esptool/espidf.'''

from os import path

from .core import ZephyrBinaryRunner, RunnerCaps


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

    @classmethod
    def name(cls):
        return 'esp32'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        # Required
        parser.add_argument('--esp-idf-path', required=True,
                            help='path to ESP-IDF')

        # Optional
        parser.add_argument('--esp-device', default='/dev/ttyUSB0',
                            help='serial port to flash, default /dev/ttyUSB0')
        parser.add_argument('--esp-baud-rate', default='921600',
                            help='serial baud rate, default 921600')
        parser.add_argument('--esp-flash-size', default='detect',
                            help='flash size, default "detect"')
        parser.add_argument('--esp-flash-freq', default='40m',
                            help='flash frequency, default "40m"')
        parser.add_argument('--esp-flash-mode', default='dio',
                            help='flash mode, default "dio"')
        parser.add_argument(
            '--esp-tool',
            help='''if given, complete path to espidf. default is to search for
            it in [ESP_IDF_PATH]/components/esptool_py/esptool/esptool.py''')

    @classmethod
    def create_from_args(command, args):
        if args.esp_tool:
            espidf = args.esp_tool
        else:
            espidf = path.join(args.esp_idf_path, 'components', 'esptool_py',
                               'esptool', 'esptool.py')

        return Esp32BinaryRunner(
            args.kernel_elf, args.esp_device, baud=args.esp_baud_rate,
            flash_size=args.esp_flash_size, flash_freq=args.esp_flash_freq,
            flash_mode=args.esp_flash_mode, espidf=espidf,
            debug=args.verbose)

    def do_run(self, command, **kwargs):
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
