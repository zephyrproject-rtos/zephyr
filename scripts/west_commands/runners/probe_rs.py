# Copyright (c) 2024 Chen Xingyu <hi@xingrz.me>
# SPDX-License-Identifier: Apache-2.0

'''Runner for probe-rs.'''

from runners.core import FileType, RunnerCaps, ZephyrBinaryRunner

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
                 reset=False,
                 protocol='swd',
                 speed=None,
                 connect_under_reset=False,
                 verify=False,
                 binary_format='elf',
                 tool_opt=None):
        super().__init__(cfg)

        self.probe_rs = probe_rs
        self.erase = erase
        self.reset = reset
        self.protocol = protocol
        self.speed = speed
        self.connect_under_reset = connect_under_reset
        self.verify = verify
        self.binary_format = binary_format

        self.args = ['--chip', chip]

        if dev_id is not None:
            self.args += ['--probe', dev_id]

        if protocol != 'swd':
            self.args += ['--protocol', protocol]

        if speed is not None:
            self.args += ['--speed', str(speed)]

        if connect_under_reset:
            self.args += ['--connect-under-reset']

        if tool_opt is not None:
            self.args += tool_opt

        self.file = cfg.file
        self.file_type = cfg.file_type
        self.elf_name = cfg.elf_file

        # GDB configuration (upstream compatible)
        self.gdb_cmd = cfg.gdb
        self.gdb_host = gdb_host
        self.gdb_port = gdb_port

    @classmethod
    def name(cls):
        return 'probe-rs'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash', 'debug', 'debugserver', 'attach'},
                          dev_id=True,
                          erase=True,
                          reset=True,
                          tool_opt=True,
                          rtt=True,
                          file=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--chip', required=True,
                            help='chip name')
        parser.add_argument('--probe-rs', default='probe-rs',
                            help='path to probe-rs tool, default is probe-rs')
        parser.add_argument('--protocol', choices=['swd', 'jtag'], default='swd',
                            help='debug protocol to use (swd or jtag)')
        parser.add_argument('--speed', type=int,
                            help='protocol speed in kHz')
        parser.add_argument('--connect-under-reset', action='store_true',
                            help='connect under reset')
        parser.add_argument('--verify', action='store_true',
                            help='verify flash after programming')
        parser.add_argument('--gdb-host', default=DEFAULT_PROBE_RS_GDB_HOST,
                            help=f'probe-rs gdb host, defaults to {DEFAULT_PROBE_RS_GDB_HOST}')
        parser.add_argument('--gdb-port', type=int, default=DEFAULT_PROBE_RS_GDB_PORT,
                            help=f'probe-rs gdb port, defaults to {DEFAULT_PROBE_RS_GDB_PORT}')

    @classmethod
    def dev_id_help(cls) -> str:
        return '''select a specific probe, in the form `VID:PID:<Serial>`'''

    @classmethod
    def tool_opt_help(cls) -> str:
        return '''additional options for probe-rs,
                  e.g. --chip-description-path=/path/to/chip.yml'''

    @classmethod
    def _get_binary_format_from_args(cls, args):
        '''Determine binary format from args.'''
        if args.file_type:
            # Map FileType to probe-rs format names
            format_map = {
                FileType.HEX: 'hex',
                FileType.BIN: 'bin',
                FileType.ELF: 'elf'
            }
            return format_map.get(args.file_type, 'elf')
        elif args.file and args.file.endswith('.hex'):
            return 'hex'
        else:
            return 'elf'

    @classmethod
    def do_create(cls, cfg, args):
        return ProbeRsBinaryRunner(cfg, args.chip,
                                   probe_rs=args.probe_rs,
                                   dev_id=args.dev_id,
                                   erase=args.erase,
                                   reset=args.reset,
                                   protocol=args.protocol,
                                   speed=args.speed,
                                   connect_under_reset=args.connect_under_reset,
                                   verify=args.verify,
                                   gdb_host=args.gdb_host,
                                   gdb_port=args.gdb_port,
                                   binary_format=cls._get_binary_format_from_args(args),
                                   tool_opt=args.tool_opt)

    def do_run(self, command, **kwargs):
        self.require(self.probe_rs)
        if command == 'flash':
            self.do_flash(**kwargs)
        elif command in ('debug', 'debugserver'):
            self.do_debug_debugserver(command, **kwargs)
        elif command == 'attach':
            self.do_attach(**kwargs)

    def do_flash(self, **kwargs):
        download_args = []
        if self.erase:
            download_args += ['--chip-erase']
        if self.verify:
            download_args += ['--verify']
        # Use provided file or default ELF file
        flash_file = self.file if self.file else self.elf_name
        # Determine format based on file_type or binary_format
        if self.file_type:
            format_map = {
                FileType.HEX: 'hex',
                FileType.BIN: 'bin',
                FileType.ELF: 'elf'
            }
            flash_format = format_map.get(self.file_type, 'elf')
        else:
            flash_format = self.binary_format
        download_args += ['--binary-format', flash_format, flash_file]

        self.check_call([self.probe_rs, 'download']
                        + self.args + download_args)

        if self.reset:
            self.check_call([self.probe_rs, 'reset']
                            + self.args)

    def do_debug_debugserver(self, command, **kwargs):
        '''Start GDB server or debug session using probe-rs gdb command.'''
        debug_args = ['--gdb-connection-string', f"{self.gdb_host}:{self.gdb_port}"]
        if command == 'debug':
            if self.gdb_cmd is None:
                raise ValueError('Cannot debug; gdb is missing')
            debug_args += [self.elf_name]
            debug_args += ['--gdb', self.gdb_cmd]
        else:
            # debugserver mode
            self.logger.info(f'probe-rs GDB server running on port {self.gdb_port}')

        self.check_call([self.probe_rs, 'gdb']
                        + self.args + debug_args)

    def do_attach(self, **kwargs):
        '''Attach to RTT logging using probe-rs attach command.'''
        attach_cmd = [self.probe_rs, 'attach'] + self.args + [self.elf_name]
        self.logger.info('Starting RTT session')
        self.check_call(attach_cmd)
