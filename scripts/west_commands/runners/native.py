# Copyright (c) 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

"""This file provides a ZephyrBinaryRunner that launches GDB and enables
flashing (running) a native application."""

import argparse
from runners.core import ZephyrBinaryRunner, RunnerCaps, RunnerConfig

DEFAULT_GDB_PORT = 3333

class NativeSimBinaryRunner(ZephyrBinaryRunner):
    """Runs the ELF binary under GDB."""

    def __init__(self, cfg,
                 tui=False,
                 gdb_port=DEFAULT_GDB_PORT):
        super().__init__(cfg)
        self.gdb_port = gdb_port

        if cfg.gdb is None:
            self.gdb_cmd = None
        else:
            self.gdb_cmd = [cfg.gdb] + (['-tui'] if tui else [])

        if self.cfg.gdb is None:
            raise ValueError("The provided RunnerConfig is missing the required field 'gdb'.")

        if self.cfg.exe_file is None:
            raise ValueError("The provided RunnerConfig is missing the required field 'exe_file'.")


    @classmethod
    def name(cls):
        return 'native'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'debug', 'debugserver', 'flash'})

    @classmethod
    def do_add_parser(cls, parser: argparse.ArgumentParser):
        parser.add_argument('--tui', default=False, action='store_true',
                            help='if given, GDB uses -tui')
        parser.add_argument('--gdb-port', default=DEFAULT_GDB_PORT,
                            help='gdb port, defaults to {}'.format(
                                DEFAULT_GDB_PORT))

    @classmethod
    def do_create(cls, cfg: RunnerConfig, args: argparse.Namespace) -> ZephyrBinaryRunner:
        return NativeSimBinaryRunner(cfg,
                                     tui=args.tui,
                                     gdb_port=args.gdb_port)

    def do_run(self, command: str, **kwargs):
        if command == 'flash':
            self.do_flash(**kwargs)
        elif command == 'debug':
            self.do_debug(**kwargs)
        elif command == 'debugserver':
            self.do_debugserver(**kwargs)
        else:
            assert False

    def do_flash(self, **kwargs):
        cmd = [self.cfg.exe_file]
        self.check_call(cmd)

    def do_debug(self, **kwargs):
        # Clues to debug missing RunnerConfig values (in context of `west debug`):
        #   build/zephyr/runners.yaml is missing `gdb` or `elf_file`.
        #   board.cmake should have `board_finalize_runner_args(native)`.
        #   build/CMakeCache.txt should have `CMAKE_GDB`.

        cmd = (self.gdb_cmd + ['--quiet', self.cfg.exe_file])
        self.check_call(cmd)

    def do_debugserver(self, **kwargs):
        cmd = (['gdbserver', ':{}'.format(self.gdb_port), self.cfg.exe_file])

        self.check_call(cmd)
