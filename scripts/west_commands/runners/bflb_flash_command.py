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
        baud_rate=DEFAULT_SPEED,
        chipname=DEFAULT_CHIP,
        flash_addr=DEFAULT_FLASH_ADDR,
        erase=False,
        boot_header=None,
        flash_size=DEFAULT_FLASH_SIZE,
    ):
        super().__init__(cfg)
        if cfg.file is not None:
            self.file = cfg.file
        else:
            self.file = cfg.bin_file
        self.port = port
        self.baud_rate = baud_rate
        self.chipname = chipname
        self.flash_addr = flash_addr
        self.erase = bool(erase)
        self.boot_header = boot_header
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
            '--baud-rate',
            default=DEFAULT_SPEED,
            help=f"serial port speed to use, default is {str(DEFAULT_SPEED)}",
        )
        parser.add_argument(
            '-ch',
            '--chipname',
            default=DEFAULT_CHIP,
            help=f"chip model, default is {str(DEFAULT_CHIP)}",
            # Allow use with all possible values. Only BL616CL and BL618DG may work without
            # additional tooling-side configuration: Older SoCs typically require generating
            # boot headers appropriate for the specific flash configuration of the chip and/or
            # module it is setup in. BL616CL and BL618DG avoid this by having fewer and more generic
            # possible configurations. They still require using a bootheader with the appropriate
            # defaults and less common variants may not be able to use this runner as-is either.
            choices=['bl602', 'bl616', 'bl616cl', 'bl618dg', 'bl702', 'bl702l'],
        )
        parser.add_argument(
            '--boot-header',
            required=False,
            default=None,
            help='''Flash boot-header from the path provided when required, for example to recover
                    an erased boot-header. Example of files usable for this purpose are the boot2
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
            baud_rate=args.baud_rate,
            chipname=args.chipname,
            flash_addr=flash_addr,
            erase=args.erase,
            boot_header=args.boot_header,
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

        common_cmd = [
            DEFAULT_EXECUTABLE,
            '--port',
            self.port,
            '--baudrate',
            self.baud_rate,
            '--chipname',
            self.chipname,
            '--flash',
        ]

        if self.erase is True:
            if self.boot_header is not None:
                cmd_erase = common_cmd + [
                    '--erase',
                    '--whole_chip',
                ]
                self.check_call(cmd_erase)
            else:
                cmd_erase = common_cmd + [
                    '--erase',
                    '--start',
                    hex(BOOTHEADER_SIZE),
                    '--end',
                    hex(self.flash_size - 1),
                ]
                self.check_call(cmd_erase)

        if self.boot_header is not None:
            cmd_boot_header = common_cmd + [
                '--write',
                '--start',
                '0x0',
                '--file',
                self.boot_header,
            ]
            self.check_call(cmd_boot_header)

        cmd_flash = common_cmd + [
            '--write',
            '--start',
            hex(self.flash_addr),
            '--file',
            self.file,
        ]
        self.check_call(cmd_flash)
