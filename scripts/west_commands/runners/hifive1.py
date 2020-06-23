# Copyright (c) 2019, Timon Baetz
#
# SPDX-License-Identifier: Apache-2.0

'''HiFive1-specific (flash only) runner.'''

from os import path

from runners.core import ZephyrBinaryRunner, RunnerCaps


class HiFive1BinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for the HiFive1 board, using openocd.'''

    def __init__(self, cfg):
        super().__init__(cfg)
        self.openocd_config = path.join(cfg.board_dir, 'support', 'openocd.cfg')

    @classmethod
    def name(cls):
        return 'hifive1'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        pass

    @classmethod
    def do_create(cls, cfg, args):
        if cfg.gdb is None:
            raise ValueError('--gdb not provided at command line')

        return HiFive1BinaryRunner(cfg)

    def do_run(self, command, **kwargs):
        self.require(self.cfg.openocd)
        self.require(self.cfg.gdb)
        openocd_cmd = ([self.cfg.openocd, '-f', self.openocd_config])
        gdb_cmd = ([self.cfg.gdb, self.cfg.elf_file, '--batch',
                    '-ex', 'set remotetimeout 240',
                    '-ex', 'target extended-remote localhost:3333',
                    '-ex', 'load',
                    '-ex', 'quit'])
        self.run_server_and_client(openocd_cmd, gdb_cmd)
