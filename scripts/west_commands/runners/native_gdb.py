# Copyright (c) 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

"""This file provides a ZephyrBinaryRunner that launches GDB."""

import argparse
from runners.core import ZephyrBinaryRunner, RunnerCaps, RunnerConfig

class NativeGDBBinaryRunner(ZephyrBinaryRunner):
    """Runs the ELF binary under GDB."""

    @classmethod
    def name(cls):
        return 'native_gdb'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'debug'})

    @classmethod
    def do_add_parser(cls, parser: argparse.ArgumentParser):
        pass

    @classmethod
    def do_create(cls, cfg: RunnerConfig, args: argparse.Namespace) -> ZephyrBinaryRunner:
        return NativeGDBBinaryRunner(cfg)

    def do_run(self, command: str, **kwargs):
        assert command == 'debug'

        # Clues to debug missing RunnerConfig values (in context of `west debug`):
        #   build/zephyr/runners.yaml is missing `gdb` or `elf_file`.
        #   board.cmake should have `board_finalize_runner_args(native_gdb)`.
        #   build/CMakeCache.txt should have `CMAKE_GDB`.

        if self.cfg.gdb is None:
            raise ValueError("The provided RunnerConfig is missing the required field 'gdb'.")

        if self.cfg.exe_file is None:
            raise ValueError("The provided RunnerConfig is missing the required field 'exe_file'.")

        self.call([
            self.cfg.gdb,
            '--quiet',
            self.cfg.exe_file,
        ])
