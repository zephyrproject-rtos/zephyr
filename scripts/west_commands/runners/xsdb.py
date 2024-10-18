# Copyright (c) 2024 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0
# Author: Appana Durga Kedareswara rao

"""Runner for flashing with xsdb CLI, the official programming
   utility from AMD platforms.
"""
import argparse
import os
from runners.core import ZephyrBinaryRunner, RunnerCaps, RunnerConfig

class XSDBBinaryRunner(ZephyrBinaryRunner):
    def __init__(self, cfg: RunnerConfig, config=None, bitstream=None,
            fsbl=None):
        super(XSDBBinaryRunner, self).__init__(cfg)
        self.elf_file = cfg.elf_file
        if not config:
            cfgfile_path = os.path.join(cfg.board_dir, 'support')
            default = os.path.join(cfgfile_path, 'xsdb.cfg')
            board_name = os.path.basename(cfg.board_dir)
            if 'mbv32' in board_name and bitstream is None:
                raise ValueError('Valid bitstream is required for mbv32 platform')
            if 'kv260_r5' in board_name and fsbl is None:
                raise ValueError('Valid fsbl elf is required for kv260_r5 platform')
            if os.path.exists(default):
                config= default
        self.xsdb_cfg_file = config
        self.bitstream = bitstream
        self.fsbl = fsbl

    @classmethod
    def name(cls):
        return 'xsdb'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(flash_addr=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--config', help='if given, override default config file')
        parser.add_argument('--bitstream', help='path to the bitstream file')
        parser.add_argument('--fsbl', help='path to the r5 fsbl elf file')

    @classmethod
    def do_create(
        cls, cfg: RunnerConfig, args: argparse.Namespace
    ) -> "XSDBBinaryRunner":
        return XSDBBinaryRunner(cfg, config=args.config,
                bitstream=args.bitstream, fsbl=args.fsbl)

    def do_run(self, command, **kwargs):
        if self.bitstream:
            cmd = ['xsdb', self.xsdb_cfg_file, self.elf_file, self.bitstream]
        elif self.fsbl:
            cmd = ['xsdb', self.xsdb_cfg_file, self.elf_file, self.fsbl]
        else:
            cmd = ['xsdb', self.xsdb_cfg_file]
        self.check_call(cmd)

if __name__ == '__main__':
    XSDBBinaryRunner.main()
