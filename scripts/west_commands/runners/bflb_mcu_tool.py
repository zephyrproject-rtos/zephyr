# Copyright (c) 2024 MASSDRIVER EI (massdriver.space)
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for the Official Bouffalo Lab open source command-line flash tool (bflb-mcu-tool)'''

from runners.core import ZephyrBinaryRunner, RunnerCaps

import shutil


DEFAULT_PORT = '/dev/ttyUSB0'
DEFAULT_SPEED = '115200'
DEFAULT_CHIP = 'bl602'
BFLB_SDK_MODULE_NAME = 'hal_bouffalolab'
DEFAULT_EXECUTABLE = "bflb-mcu-tool"

class BlFlashCommandBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for bflb-mcu-tool.'''

    def __init__(self, cfg,
                 port=DEFAULT_PORT,
                 baudrate=DEFAULT_SPEED,
                 chipname=DEFAULT_CHIP,
                 erase=False):
        super().__init__(cfg)
        self.port = port
        self.baudrate = baudrate
        self.chipname = chipname
        self.erase = bool(erase)

    @classmethod
    def name(cls):
        return 'bflb_mcu_tool'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, erase=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('-p', '--port', default=DEFAULT_PORT,
                            help='serial port to use, default is ' +
                            str(DEFAULT_PORT))
        parser.add_argument('-b', '--baudrate', default=DEFAULT_SPEED,
                            help='serial port speed to use, default is ' +
                            DEFAULT_SPEED)
        parser.add_argument('-ch', '--chipname', default=DEFAULT_CHIP,
                            help='chip model, default is ' +
                            DEFAULT_CHIP,
                            choices=['bl602', 'bl606p', 'bl616', 'bl702', 'bl702l', 'bl808'])

    @classmethod
    def do_create(cls, cfg, args):
        print(args)
        if shutil.which(DEFAULT_EXECUTABLE) is None:
            raise ValueError("Couldnt find bflb-mcu-tool in PATH, please install with pip install \
bflb-mcu-tool")
        return BlFlashCommandBinaryRunner(cfg,
                                        port=args.port,
                                        baudrate=args.baudrate,
                                        chipname=args.chipname,
                                        erase=args.erase)

    def do_run(self, command, **kwargs):
        self.ensure_output('bin')
        cmd_flash = [DEFAULT_EXECUTABLE,
                '--interface', 'uart',
                '--port', self.port,
                '--baudrate', self.baudrate,
                '--chipname', self.chipname,
                '--firmware', self.cfg.bin_file]
        if self.erase is True:
            cmd_flash.append("--erase")
        self.check_call(cmd_flash)
