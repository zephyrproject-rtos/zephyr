# Copyright (c) 2022 Renesas Electronics Corporation
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with ezFlashCLI.'''
import shlex

from runners.core import ZephyrBinaryRunner, RunnerCaps

DEFAULT_EZFLASHCLI = "ezFlashCLI"

class EzFlashCliBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for ezFlashCLI'''

    def __init__(self, cfg, tool, tool_opt=[], erase=False, reset=True):
        super().__init__(cfg)
        self.bin_ = cfg.bin_file

        self.tool = tool
        self.erase = bool(erase)
        self.reset = bool(reset)

        self.tool_opt = []
        for opts in [shlex.split(opt) for opt in tool_opt]:
            self.tool_opt += opts

    @classmethod
    def name(cls):
        return 'ezflashcli'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, tool_opt=True, erase=True, reset=True)

    @classmethod
    def tool_opt_help(cls) -> str:
        return "Additional options for ezFlashCLI e.g. '--verbose'"

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--tool', default=DEFAULT_EZFLASHCLI,
                            help='ezFlashCLI path, default is '
                                f'{DEFAULT_EZFLASHCLI}')
        parser.set_defaults(reset=True)

    @classmethod
    def do_create(cls, cfg, args):
        return EzFlashCliBinaryRunner(cfg, tool=args.tool, tool_opt=args.tool_opt,
                                      erase=args.erase)

    def needs_product_header(self):
        # Applications linked to code partition are meant to be run by MCUboot
        # and do not require product header. Other applications and MCUboot itself
        # are run by internal bootloader and thus require valid product header.

        is_mcuboot = self.build_conf.getboolean('CONFIG_MCUBOOT')
        uses_code_partition = self.build_conf.getboolean('CONFIG_USE_DT_CODE_PARTITION')

        return is_mcuboot or not uses_code_partition

    def program_bin(self):

        if self.erase:
            self.logger.info("Erasing flash...")
            self.check_call([self.tool] + self.tool_opt + ["erase_flash"])

        self.logger.info(f"Flashing {self.bin_}...")
        if self.needs_product_header():
            # Write product header and application image at fixed offset as required
            # by internal bootloader.
            self.check_call([self.tool] + self.tool_opt + ["image_flash", self.bin_])
        else:
            load_offset = self.build_conf['CONFIG_FLASH_LOAD_OFFSET']
            self.check_call([self.tool] + self.tool_opt + ["write_flash", f'0x{load_offset:x}', self.bin_])

    def reset_device(self):
        self.logger.info("Resetting...")
        self.check_call([self.tool] + self.tool_opt + ["go"])

    def do_run(self, command, **kwargs):
        self.require(self.tool)
        self.ensure_output('bin')
        self.program_bin()
        if self.reset:
            self.reset_device()
