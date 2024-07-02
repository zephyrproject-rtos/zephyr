# Copyright (c) 2024 Dhiru Kholia
# SPDX-License-Identifier: Apache-2.0

'''picorvd-specific runner - https://github.com/kholia/picorvd'''

import os
import pathlib
import pickle
import platform
import subprocess
import sys
import time

from runners.core import ZephyrBinaryRunner, RunnerCaps

class PicorvdBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for picorvd.'''

    def __init__(self, cfg, picorvd='gdb-multiarch'):
        super().__init__(cfg)
        self.picorvd = picorvd

    @classmethod
    def name(cls):
        return 'picorvd'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--gdb-multiarch', default='gdb-multiarch',
                            help='path to picorvd, default is picorvd')

    @classmethod
    def do_create(cls, cfg, args):
        return PicorvdBinaryRunner(cfg, picorvd="gdb-multiarch")

    def supports(self, flag):
        """Check if picorvd supports a flag by searching the help"""
        for line in self.read_help():
            if flag in line:
                return True
        return True

    def make_picorvd_cmd(self):
        # TODO: Figure out the device path dynamically
        cmd = ["gdb-multiarch", "-ex", "set confirm off", "-ex", "target extended-remote /dev/ttyACM0", "-ex", "load", "-ex", "monitor reset", "-ex", "continue&", "-ex", "quit", self.cfg.elf_file]
        print(cmd)

        return cmd

    def do_run(self, command, **kwargs):
        self.require(self.picorvd)
        self.check_call(self.make_picorvd_cmd())
