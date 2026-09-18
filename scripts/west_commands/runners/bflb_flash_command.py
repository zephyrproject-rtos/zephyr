# Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for the Bouffalo Lab open source command-line flash tool (bflb-flash-command-uart)'''

from runners.core import BuildConfiguration, MissingProgram, RunnerCaps, ZephyrBinaryRunner

DEFAULT_PORT = '/dev/ttyUSB0'
DEFAULT_SPEED = '2000000'
DEFAULT_CHIP = 'bl616cl'
DEFAULT_EXECUTABLE = "bflb-flash-command-uart"
BOOTHEADER_SIZE = 0x2000
DEFAULT_FLASH_ADDR = BOOTHEADER_SIZE
DEFAULT_FLASH_SIZE = 0x400000


class BlFlashCommandBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for bflb-flash-command-uart.'''

    def __init__(
        self,
        cfg,
        port=DEFAULT_PORT,
        baudrate=DEFAULT_SPEED,
        chipname=DEFAULT_CHIP,
        flash_addr=DEFAULT_FLASH_ADDR,
        erase=False,
        bootheader=None,
        flash_size=DEFAULT_FLASH_SIZE,
    ):
        super().__init__(cfg)
        if cfg.file is not None:
            self.file = cfg.file
        else:
            self.file = cfg.bin_file
        self.port = port
        self.baudrate = baudrate
        self.chipname = chipname
        self.flash_addr = flash_addr
        self.erase = bool(erase)
        self.bootheader = bootheader
        self.flash_size = flash_size

    @classmethod
    def name(cls):
        return 'bflb_flash_command'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, erase=True, dev_id=True, flash_addr=True, file=True)

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
            choices=['bl602', 'bl616', 'bl616cl', 'bl618dg', 'bl702', 'bl702l'],
        )
        parser.add_argument(
            '--bootheader',
            required=False,
            default=None,
            help='''Flash bootheader from the path provided when required, for example to recover
                    an erased bootheader. Example of files usable for this purpose are the boot2
                    files available in the bsp/board/<soc/builtin_imgs folder of the vendor SDK.''',
        )

    @classmethod
    def do_create(cls, cfg, args):
        build_conf = BuildConfiguration(cfg.build_dir)
        code_flash = build_conf.edt.chosen_node('zephyr,flash')
        if args.dt_flash:
            flash_addr = cls.flash_address_from_build_conf(build_conf)
        else:
            flash_addr = DEFAULT_FLASH_ADDR

        return BlFlashCommandBinaryRunner(
            cfg,
            port=args.dev_id,
            baudrate=args.baudrate,
            chipname=args.chipname,
            flash_addr=flash_addr,
            erase=args.erase,
            bootheader=args.bootheader,
            flash_size=code_flash.regs[0].size,
        )

    def do_run(self, command, **kwargs):
        try:
            self.require(DEFAULT_EXECUTABLE)
        except MissingProgram as err:
            self.logger.error(
                "You may use `pip install bflb-flash-command-uart "
                "to install bflb-flash-command-uart"
            )
            raise err
        self.ensure_output('bin')

        if self.erase is True:
            if self.bootheader is not None:
                cmd_erase = [
                    DEFAULT_EXECUTABLE,
                    '--port',
                    self.port,
                    '--baudrate',
                    self.baudrate,
                    '--chipname',
                    self.chipname,
                    '--flash',
                    '--erase',
                    '--whole_chip',
                ]
                self.check_call(cmd_erase)
            else:
                cmd_erase = [
                    DEFAULT_EXECUTABLE,
                    '--port',
                    self.port,
                    '--baudrate',
                    self.baudrate,
                    '--chipname',
                    self.chipname,
                    '--flash',
                    '--erase',
                    '--start',
                    hex(BOOTHEADER_SIZE),
                    '--end',
                    hex(self.flash_size - 1),
                ]
                self.check_call(cmd_erase)

        if self.bootheader is not None:
            cmd_bootheader = [
                DEFAULT_EXECUTABLE,
                '--port',
                self.port,
                '--baudrate',
                self.baudrate,
                '--chipname',
                self.chipname,
                '--flash',
                '--write',
                '--start',
                '0x0',
                '--file',
                self.bootheader,
            ]
            self.check_call(cmd_bootheader)

        cmd_flash = [
            DEFAULT_EXECUTABLE,
            '--port',
            self.port,
            '--baudrate',
            self.baudrate,
            '--chipname',
            self.chipname,
            '--flash',
            '--write',
            '--start',
            hex(self.flash_addr),
            '--file',
            self.file,
        ]
        self.check_call(cmd_flash)
