# Copyright (c) 2017 Linaro Limited.
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

"""Runner for QEMU-based boards."""

from runners.core import RunnerCaps, ZephyrBinaryRunner

DEFAULT_QEMU_GDB_PORT = 1234


class QemuBinaryRunner(ZephyrBinaryRunner):
    """Runner for QEMU-based boards."""

    def __init__(self, cfg, gdb_port=DEFAULT_QEMU_GDB_PORT):
        super().__init__(cfg)
        self.gdb_port = gdb_port

    @classmethod
    def name(cls):
        return 'qemu'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'run', 'debugserver'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--gdb-port', default=DEFAULT_QEMU_GDB_PORT,
                            type=int,
                            help=f'GDB port, default {DEFAULT_QEMU_GDB_PORT}')

    @classmethod
    def do_create(cls, cfg, args):
        return QemuBinaryRunner(cfg, gdb_port=args.gdb_port)

    def do_run(self, command, **kwargs):
        if command == 'run':
            self.check_call(['cmake', '--build', self.cfg.build_dir,
                             '--target', 'run'])
        elif command == 'debugserver':
            self.logger.info('Starting QEMU GDB server on port %d',
                             self.gdb_port)
            self.check_call(['cmake', '--build', self.cfg.build_dir,
                             '--target', 'debugserver'])
