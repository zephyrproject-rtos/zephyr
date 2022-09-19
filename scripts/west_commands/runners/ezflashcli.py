# Copyright (c) 2022 Renesas Electronics Corporation
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with ezFlashCLI.'''
from runners.core import ZephyrBinaryRunner, RunnerCaps

DEFAULT_EZFLASHCLI = "ezFlashCLI"

class EzFlashCliBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for ezFlashCLI'''

    def __init__(self, cfg, tool, sn, erase=False):
        super().__init__(cfg)
        self.bin_ = cfg.bin_file

        self.tool = tool
        self.sn_arg = ['-j', f'{sn}'] if sn is not None else []
        self.erase = bool(erase)

    @classmethod
    def name(cls):
        return 'ezflashcli'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, erase=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--tool', default=DEFAULT_EZFLASHCLI,
                            help='ezFlashCLI path, default is '
                                f'{DEFAULT_EZFLASHCLI}')
        parser.add_argument('--sn', default=None, required=False,
                            help='J-Link probe serial number')

    @classmethod
    def do_create(cls, cfg, args):
        return EzFlashCliBinaryRunner(cfg, tool=args.tool, sn=args.sn,
                                      erase=args.erase)

    def program_bin(self):
        if self.erase:
            self.logger.info("Erasing flash...")
            self.check_call([self.tool] + self.sn_arg + ["erase_flash"])

        self.logger.info(f"Flashing {self.bin_}...")
        self.check_call([self.tool] + self.sn_arg + ["image_flash", self.bin_])

    def reset(self):
        self.logger.info("Resetting...")
        self.check_call([self.tool] + self.sn_arg + ["go"])

    def do_run(self, command, **kwargs):
        self.require(self.tool)
        self.ensure_output('bin')
        self.program_bin()
        self.reset()
