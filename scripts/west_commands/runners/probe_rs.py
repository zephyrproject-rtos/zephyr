# Copyright (c) 2024 Chen Xingyu <hi@xingrz.me>
# SPDX-License-Identifier: Apache-2.0

'''Runner for probe-rs.'''

from runners.core import RunnerCaps, ZephyrBinaryRunner

DEFAULT_PROBE_RS_GDB_HOST = 'localhost'
DEFAULT_PROBE_RS_GDB_PORT = 1337

class ProbeRsBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for probe-rs.'''

    def __init__(self, cfg, chip,
                 probe_rs='probe-rs',
                 gdb_host=DEFAULT_PROBE_RS_GDB_HOST,
                 gdb_port=DEFAULT_PROBE_RS_GDB_PORT,
                 dev_id=None,
                 erase=False,
                 tool_opt=None):
        super().__init__(cfg)

        self.probe_rs = probe_rs
        self.erase = erase

        self.args = ['--chip', chip]

        if dev_id is not None:
            self.args += ['--probe', dev_id]

        if tool_opt is not None:
            self.args += tool_opt

        self.elf_name = cfg.elf_file

        self.gdb_cmd = cfg.gdb
        self.gdb_host = gdb_host
        self.gdb_port = gdb_port

    @classmethod
    def name(cls):
        return 'probe-rs'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash', 'debug', 'debugserver'},
                          dev_id=True,
                          erase=True,
                          tool_opt=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--chip', required=True,
                            help='chip name')
        parser.add_argument('--probe-rs', default='probe-rs',
                            help='path to probe-rs tool, default is probe-rs')
        parser.add_argument('--gdb-host', default=DEFAULT_PROBE_RS_GDB_HOST,
                            help=f'probe-rs gdb host, defaults to {DEFAULT_PROBE_RS_GDB_HOST}')
        parser.add_argument('--gdb-port', default=DEFAULT_PROBE_RS_GDB_PORT,
                            help=f'probe-rs gdb port, defaults to {DEFAULT_PROBE_RS_GDB_PORT}')


    @classmethod
    def dev_id_help(cls) -> str:
        return '''select a specific probe, in the form `VID:PID:<Serial>`'''

    @classmethod
    def tool_opt_help(cls) -> str:
        return '''additional options for probe-rs,
                  e.g. --chip-description-path=/path/to/chip.yml'''

    @classmethod
    def do_create(cls, cfg, args):
        return ProbeRsBinaryRunner(cfg, args.chip,
                                   probe_rs=args.probe_rs,
                                   dev_id=args.dev_id,
                                   erase=args.erase,
                                   tool_opt=args.tool_opt)

    def do_run(self, command, **kwargs):
        self.require(self.probe_rs)
        if command == 'flash':
            self.do_flash(**kwargs)
        elif command in ('debug', 'debugserver'):
            self.do_debug_debugserver(command, **kwargs)

    def do_flash(self, **kwargs):
        download_args = []
        if self.erase:
            download_args += ['--chip-erase']
        download_args += [self.elf_name]

        self.check_call([self.probe_rs, 'download']
                        + self.args + download_args)

        self.check_call([self.probe_rs, 'reset']
                        + self.args)

    def do_debug_debugserver(self, command, **kwargs):
        debug_args = ['--gdb-connection-string', f"{self.gdb_host}:{self.gdb_port}"]
        if command == 'debug':
            debug_args += [self.elf_name]
            debug_args += ['--gdb', self.gdb_cmd]

        self.check_call([self.probe_rs, 'gdb']
                        + self.args + debug_args)
