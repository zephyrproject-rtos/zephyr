# Copyright (c) 2024 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0

"""Runner for flashing with xsdb CLI, the official programming
   utility from AMD platforms.
"""
import argparse
import os

from runners.core import RunnerCaps, RunnerConfig, ZephyrBinaryRunner


class XSDBBinaryRunner(ZephyrBinaryRunner):
    def __init__(self, cfg: RunnerConfig, config=None, bitstream=None,
            fsbl=None, pdi=None, bl31=None, dtb=None):
        super().__init__(cfg)
        self.elf_file = cfg.elf_file
        if not config:
            cfgfile_path = os.path.join(cfg.board_dir, 'support')
            default = os.path.join(cfgfile_path, 'xsdb.cfg')
            if os.path.exists(default):
                config = default
        self.xsdb_cfg_file = config
        self.bitstream = bitstream
        self.fsbl = fsbl
        self.pdi = pdi
        self.bl31 = bl31
        self.dtb = dtb

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
        parser.add_argument('--fsbl', help='path to the fsbl elf file')
        parser.add_argument('--pdi', help='path to the base/boot pdi file')
        parser.add_argument('--bl31', help='path to the bl31(ATF) elf file')
        parser.add_argument('--system-dtb', help='path to the system.dtb file')

    @classmethod
    def do_create(
        cls, cfg: RunnerConfig, args: argparse.Namespace
    ) -> "XSDBBinaryRunner":
        return XSDBBinaryRunner(cfg, config=args.config,
                bitstream=args.bitstream, fsbl=args.fsbl,
                pdi=args.pdi, bl31=args.bl31, dtb=args.system_dtb)

    def do_run(self, command, **kwargs):
        if self.bitstream and self.fsbl:
            cmd = ['xsdb', self.xsdb_cfg_file, self.elf_file, self.bitstream, self.fsbl]
        elif self.bitstream:
            cmd = ['xsdb', self.xsdb_cfg_file, self.elf_file, self.bitstream]
        elif self.fsbl:
            cmd = ['xsdb', self.xsdb_cfg_file, self.elf_file, self.fsbl]
        elif self.pdi and self.bl31 and self.dtb:
            cmd = ['xsdb', self.xsdb_cfg_file, self.elf_file, self.pdi, self.bl31, self.dtb]
        elif self.pdi and self.bl31:
            cmd = ['xsdb', self.xsdb_cfg_file, self.elf_file, self.pdi, self.bl31]
        elif self.pdi:
            cmd = ['xsdb', self.xsdb_cfg_file, self.elf_file, self.pdi]
        else:
            cmd = ['xsdb', self.xsdb_cfg_file, self.elf_file]
        self.check_call(cmd)
