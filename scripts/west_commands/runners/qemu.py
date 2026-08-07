# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for QEMU.'''

import subprocess

from runners.core import RunnerCaps, ZephyrBinaryRunner

DEFAULT_GDB_PORT = 1234


class QemuBinaryRunner(ZephyrBinaryRunner):
    '''Emulator runner for QEMU.'''

    def __init__(self, cfg, tui=False, gdb_port=DEFAULT_GDB_PORT):
        super().__init__(cfg)
        if cfg.gdb is None:
            self.gdb_cmd = None
        else:
            self.gdb_cmd = [cfg.gdb] + (['-tui'] if tui else [])
        self.gdb_port = gdb_port

    @classmethod
    def name(cls):
        return 'qemu'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash', 'debug', 'debugserver'})

    @classmethod
    def do_add_parser(cls, parser):
        pass  # Nothing to do.

    @classmethod
    def do_create(cls, cfg, args):
        return QemuBinaryRunner(cfg)

    def do_run(self, command, **kwargs):
        # For QEMU emulation, we rely on the CMake targets where available
        # (run_qemu, debugserver_qemu, etc.) to actually run QEMU. This runner
        # is installed to allow west debug/debugserver commands to work with
        # QEMU.
        #
        # The actual QEMU invocation happens via the CMake build system.
        # Here we trigger the appropriate CMake target automatically.
        if command == 'debugserver':
            self.debugserver(**kwargs)
        elif command == 'debug':
            self.debug(**kwargs)
        elif command == 'flash':
            self.do_flash(**kwargs)

    def do_flash(self, **kwargs):
        """Start QEMU using existing target."""
        # Trigger the CMake debugserver_qemu target
        self._run_cmake_target('run_qemu')

    def debug(self, **kwargs):
        if self.gdb_cmd is None:
            raise ValueError('Cannot debug; gdb is missing')

        gdb_cmd = self.gdb_cmd + ['-ex', f'target remote :{self.gdb_port}', self.cfg.elf_file]
        self.require(gdb_cmd[0])

        self.run_client(gdb_cmd)

    def debugserver(self, **kwargs):
        """Start QEMU with GDB server enabled."""
        # Trigger the CMake debugserver_qemu target
        self._run_cmake_target('debugserver_qemu')

    def _run_cmake_target(self, target):
        """Run a CMake target in the build directory."""
        build_dir = self.cfg.build_dir

        # Run cmake --build with the target
        cmd = ['cmake', '--build', str(build_dir), '-t', target]

        self.logger.info(f'Starting QEMU with target: {target}')
        self.logger.debug(f'Running: {" ".join(cmd)}')

        try:
            subprocess.run(cmd, check=True)
        except subprocess.CalledProcessError as e:
            self.logger.error(f'Failed to run {target}: {e}')
            raise
