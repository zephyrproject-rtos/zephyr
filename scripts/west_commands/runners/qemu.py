# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner stub for QEMU.'''

from pathlib import Path
from runners.core import ZephyrBinaryRunner, RunnerCaps


DEFAULT_KERNEL="zephyr.elf"
DEFAULT_QEMU_GDB_PORT = 1234
class QemuBinaryRunner(ZephyrBinaryRunner):
    '''Place-holder for QEMU runner customizations.'''

    def __init__(self, cfg, tui=False, kernel=DEFAULT_KERNEL, gdb_port=DEFAULT_QEMU_GDB_PORT):
        super().__init__(cfg)


        self.qemu_cmd = [cfg.qemu]
        self.qemu_flags = cfg.qemu_flags
        self.kernel = kernel
        self.gdb_port = gdb_port
        self.gdb_cmd = [cfg.gdb] if cfg.gdb else None

        self.elf_name = Path(cfg.elf_file).as_posix()
        self.tui_arg = ['-tui'] if tui else []

    @classmethod
    def name(cls):
        return 'qemu'

    @classmethod
    def capabilities(cls):
        # This is a stub.
        return RunnerCaps(commands=set())

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--kernel', default=DEFAULT_KERNEL,
                    help='Kernel to boot.')
        parser.add_argument('--gdb-port', default=DEFAULT_QEMU_GDB_PORT,
                            help='qemu gdb port, defaults to 1234')

    @classmethod
    def do_create(cls, cfg, args):
        return QemuBinaryRunner(cfg)

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash', 'debug', 'debugserver', 'attach'},
                          flash_addr=True, erase=True)

    def do_run(self, command, **kwargs):
        if command == 'flash':
            self.do_flash(**kwargs)
        elif command == 'debug':
            self.do_debug(**kwargs)
        else:
            self.debugserver(**kwargs)

    def do_flash(self, **kwargs):
        _flags = self.qemu_flags.split(";")
        _flags = list(filter(None, _flags))

        cmd = (self.qemu_cmd + _flags + ['-kernel', self.cfg.elf_file])
        #cmd = (self.qemu_cmd + _flags)
        self.check_call(cmd)


    def do_debug(self, **kwargs):
        if self.gdb_cmd is None:
            raise ValueError('Cannot debug; gdb is missing')

        _flags = self.qemu_flags.split(";")
        _flags = list(filter(None, _flags))

        server_cmd = (self.qemu_cmd + _flags + ['-s', '-S', '-kernel', self.cfg.elf_file])

        gdb_cmd = (self.gdb_cmd + self.tui_arg +
                   ['-ex', 'target remote :{}'.format(self.gdb_port),
                    self.elf_name])

        self.require(gdb_cmd[0])

        self.run_server_and_client(server_cmd, gdb_cmd)

    def debugserver(self, **kwargs):


        _flags = self.qemu_flags.split(";")
        _flags = list(filter(None, _flags))

        server_cmd = (self.qemu_cmd + _flags + ['-s', '-S', '-kernel', self.cfg.elf_file])

        self.check_call(server_cmd)
