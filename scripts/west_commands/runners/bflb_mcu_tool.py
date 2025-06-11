# Copyright (c) 2024-2025 MASSDRIVER EI (massdriver.space)
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for the Official Bouffalo Lab open source command-line flash tool (bflb-mcu-tool)'''

from runners.core import MissingProgram, RunnerCaps, ZephyrBinaryRunner

DEFAULT_PORT = '/dev/ttyUSB0'
DEFAULT_SPEED = '115200'
DEFAULT_CHIP = 'bl602'
DEFAULT_EXECUTABLE = "bflb-mcu-tool-uart"


class BlFlashCommandBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for bflb-mcu-tool.'''

    def __init__(
        self, cfg, port=DEFAULT_PORT, baudrate=DEFAULT_SPEED, chipname=DEFAULT_CHIP, erase=False
    ):
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
        return RunnerCaps(commands={'flash'}, erase=True, dev_id=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.set_defaults(dev_id=DEFAULT_PORT)
        parser.add_argument(
            '-b',
            '--baudrate',
            default=DEFAULT_SPEED,
            help=f"serial port speed to use, default is {str(DEFAULT_SPEED)}",
        )
        parser.add_argument(
            '-ch',
            '--chipname',
            default=DEFAULT_CHIP,
            help=f"chip model, default is {str(DEFAULT_CHIP)}",
            choices=['bl602', 'bl606p', 'bl616', 'bl702', 'bl702l', 'bl808'],
        )

    @classmethod
    def do_create(cls, cfg, args):
        return BlFlashCommandBinaryRunner(
            cfg, port=args.dev_id, baudrate=args.baudrate, chipname=args.chipname, erase=args.erase
        )

    def do_run(self, command, **kwargs):
        try:
            self.require(DEFAULT_EXECUTABLE)
        except MissingProgram as err:
            self.logger.error(
                "You may use `pip install bflb-mcu-tool-uart` to install bflb-mcu-tool-uart"
            )
            raise err
        self.ensure_output('bin')
        cmd_flash = [
            DEFAULT_EXECUTABLE,
            '--port',
            self.port,
            '--baudrate',
            self.baudrate,
            '--chipname',
            self.chipname,
            '--firmware',
            self.cfg.bin_file,
        ]
        if self.erase is True:
            cmd_flash.append("--erase")
        self.check_call(cmd_flash)
