# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for openocd.'''

from os import path
from pathlib import Path

try:
    from elftools.elf.elffile import ELFFile
except ImportError:
    ELFFile = None

from runners.core import ZephyrBinaryRunner

DEFAULT_OPENOCD_TCL_PORT = 6333
DEFAULT_OPENOCD_TELNET_PORT = 4444
DEFAULT_OPENOCD_GDB_PORT = 3333


class OpenOcdBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for openocd.'''

    def __init__(self, cfg, pre_init=None, pre_load=None,
                 load_cmd=None, verify_cmd=None, post_verify=None,
                 tui=None, config=None, serial=None, use_elf=None,
                 tcl_port=DEFAULT_OPENOCD_TCL_PORT,
                 telnet_port=DEFAULT_OPENOCD_TELNET_PORT,
                 gdb_port=DEFAULT_OPENOCD_GDB_PORT):
        super().__init__(cfg)

        if not config:
            default = path.join(cfg.board_dir, 'support', 'openocd.cfg')
            if path.exists(default):
                config = default
        self.openocd_config = config

        if self.openocd_config is not None and path.exists(self.openocd_config):
            search_args = ['-s', path.dirname(self.openocd_config)]
        else:
            search_args = []

        if cfg.openocd_search is not None:
            search_args.extend(['-s', cfg.openocd_search])
        self.openocd_cmd = [cfg.openocd] + search_args
        # openocd doesn't cope with Windows path names, so convert
        # them to POSIX style just to be sure.
        self.elf_name = Path(cfg.elf_file).as_posix()
        self.pre_init = pre_init or []
        self.pre_load = pre_load or []
        self.load_cmd = load_cmd
        self.verify_cmd = verify_cmd
        self.post_verify = post_verify or []
        self.tcl_port = tcl_port
        self.telnet_port = telnet_port
        self.gdb_port = gdb_port
        self.gdb_cmd = [cfg.gdb] if cfg.gdb else None
        self.tui_arg = ['-tui'] if tui else []
        self.serial = ['-c set _ZEPHYR_BOARD_SERIAL ' + serial] if serial else []
        self.use_elf = use_elf

    @classmethod
    def name(cls):
        return 'openocd'

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--config',
                            help='if given, override default config file')
        parser.add_argument('--serial', default="",
                            help='if given, selects FTDI instance by its serial number, defaults to empty')
        parser.add_argument('--use-elf', default=False, action='store_true',
                            help='if given, Elf file will be used for loading instead of HEX image')
        # Options for flashing:
        parser.add_argument('--cmd-pre-init', action='append',
                            help='''Command to run before calling init;
                            may be given multiple times''')
        parser.add_argument('--cmd-pre-load', action='append',
                            help='''Command to run before flashing;
                            may be given multiple times''')
        parser.add_argument('--cmd-load',
                            help='''Command to load/flash binary
                            (required when flashing)''')
        parser.add_argument('--cmd-verify',
                            help='''Command to verify flashed binary''')
        parser.add_argument('--cmd-post-verify', action='append',
                            help='''Command to run after verification;
                            may be given multiple times''')

        # Options for debugging:
        parser.add_argument('--tui', default=False, action='store_true',
                            help='if given, GDB uses -tui')
        parser.add_argument('--tcl-port', default=DEFAULT_OPENOCD_TCL_PORT,
                            help='openocd TCL port, defaults to 6333')
        parser.add_argument('--telnet-port',
                            default=DEFAULT_OPENOCD_TELNET_PORT,
                            help='openocd telnet port, defaults to 4444')
        parser.add_argument('--gdb-port', default=DEFAULT_OPENOCD_GDB_PORT,
                            help='openocd gdb port, defaults to 3333')

    @classmethod
    def do_create(cls, cfg, args):
        return OpenOcdBinaryRunner(
            cfg,
            pre_init=args.cmd_pre_init,
            pre_load=args.cmd_pre_load, load_cmd=args.cmd_load,
            verify_cmd=args.cmd_verify, post_verify=args.cmd_post_verify,
            tui=args.tui, config=args.config, serial=args.serial, use_elf=args.use_elf,
            tcl_port=args.tcl_port, telnet_port=args.telnet_port,
            gdb_port=args.gdb_port)

    def do_run(self, command, **kwargs):
        self.require(self.openocd_cmd[0])
        if ELFFile is None:
            raise RuntimeError(
                'elftools missing; please "pip3 install elftools"')

        self.cfg_cmd = []
        if self.openocd_config is not None:
            self.cfg_cmd = ['-f', self.openocd_config]

        if command == 'flash' and self.use_elf:
            self.do_flash_elf(**kwargs)
        elif command == 'flash':
            self.do_flash(**kwargs)
        elif command in ('attach', 'debug'):
            self.do_attach_debug(command, **kwargs)
        elif command == 'load':
            self.do_load(**kwargs)
        else:
            self.do_debugserver(**kwargs)

    def do_flash(self, **kwargs):
        self.ensure_output('hex')
        if self.load_cmd is None:
            raise ValueError('Cannot flash; load command is missing')
        if self.verify_cmd is None:
            raise ValueError('Cannot flash; verify command is missing')

        # openocd doesn't cope with Windows path names, so convert
        # them to POSIX style just to be sure.
        hex_name = Path(self.cfg.hex_file).as_posix()

        self.logger.info('Flashing file: {}'.format(hex_name))

        pre_init_cmd = []
        pre_load_cmd = []
        post_verify_cmd = []
        for i in self.pre_init:
            pre_init_cmd.append("-c")
            pre_init_cmd.append(i)

        for i in self.pre_load:
            pre_load_cmd.append("-c")
            pre_load_cmd.append(i)

        for i in self.post_verify:
            post_verify_cmd.append("-c")
            post_verify_cmd.append(i)

        cmd = (self.openocd_cmd + self.serial + self.cfg_cmd +
               pre_init_cmd + ['-c', 'init',
                                '-c', 'targets'] +
               pre_load_cmd + ['-c', 'reset halt',
                                '-c', self.load_cmd + ' ' + hex_name,
                                '-c', 'reset halt'] +
               ['-c', self.verify_cmd + ' ' + hex_name] +
               post_verify_cmd +
               ['-c', 'reset run',
                '-c', 'shutdown'])
        self.check_call(cmd)

    def do_flash_elf(self, **kwargs):
        if self.elf_name is None:
            raise ValueError('Cannot debug; no .elf specified')

        # Extract entry point address from Elf to use it later with
        # "resume" command of OpenOCD.
        with open(self.elf_name, 'rb') as f:
            ep_addr = f"0x{ELFFile(f).header['e_entry']:016x}"

        pre_init_cmd = []
        for i in self.pre_init:
            pre_init_cmd.append("-c")
            pre_init_cmd.append(i)

        cmd = (self.openocd_cmd + self.serial + self.cfg_cmd +
                      pre_init_cmd + ['-c', 'init',
                                       '-c', 'targets',
                                       '-c', 'reset halt',
                                       '-c', 'load_image ' + self.elf_name,
                                       '-c', 'resume ' + ep_addr,
                                       '-c', 'shutdown'])
        self.check_call(cmd)

    def do_attach_debug(self, command, **kwargs):
        if self.gdb_cmd is None:
            raise ValueError('Cannot debug; no gdb specified')
        if self.elf_name is None:
            raise ValueError('Cannot debug; no .elf specified')

        pre_init_cmd = []
        for i in self.pre_init:
            pre_init_cmd.append("-c")
            pre_init_cmd.append(i)

        server_cmd = (self.openocd_cmd + self.serial + self.cfg_cmd +
                      ['-c', 'tcl_port {}'.format(self.tcl_port),
                       '-c', 'telnet_port {}'.format(self.telnet_port),
                       '-c', 'gdb_port {}'.format(self.gdb_port)] +
                      pre_init_cmd + ['-c', 'init',
                                       '-c', 'targets',
                                       '-c', 'halt'])
        gdb_cmd = (self.gdb_cmd + self.tui_arg +
                   ['-ex', 'target remote :{}'.format(self.gdb_port),
                    self.elf_name])
        if command == 'debug':
            gdb_cmd.extend(['-ex', 'load'])
        self.require(gdb_cmd[0])
        self.run_server_and_client(server_cmd, gdb_cmd)

    def do_debugserver(self, **kwargs):
        pre_init_cmd = []
        for i in self.pre_init:
            pre_init_cmd.append("-c")
            pre_init_cmd.append(i)

        cmd = (self.openocd_cmd + self.cfg_cmd +
               ['-c', 'tcl_port {}'.format(self.tcl_port),
                '-c', 'telnet_port {}'.format(self.telnet_port),
                '-c', 'gdb_port {}'.format(self.gdb_port)] +
               pre_init_cmd + ['-c', 'init',
                                '-c', 'targets',
                                '-c', 'reset halt'])
        self.check_call(cmd)
