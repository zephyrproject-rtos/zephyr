# Copyright (c) 2024 Dhiru Kholia
# SPDX-License-Identifier: Apache-2.0

'''minichlink-specific runner'''

from runners.core import ZephyrBinaryRunner, RunnerCaps

class MinichlinkbinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for minichlink.'''

    def __init__(self, cfg, minichlink='minichlink'):
        super().__init__(cfg)
        self.minichlink = minichlink

    @classmethod
    def name(cls):
        return 'minichlink'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--minichlink', default='minichlink',
                            help='path to minichlink, default is minichlink')

    @classmethod
    def do_create(cls, cfg, args):
        return MinichlinkbinaryRunner(cfg, minichlink=args.minichlink)

    def supports(self, flag):
        """Check if minichlink supports a flag by searching the help"""
        for line in self.read_help():
            if flag in line:
                return True
        return True

    def make_minichlink_cmd(self):
        self.ensure_output('bin')

        cmd = ["minichlink", "-w", self.cfg.bin_file, "flash", "-b"]
        print(cmd)

        return cmd

    def do_run(self, command, **kwargs):
        self.require(self.minichlink)
        self.check_call(self.make_minichlink_cmd())
