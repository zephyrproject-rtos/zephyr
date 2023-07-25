# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Modified openocd and gdb runner for Cyclone V SoC DevKit.'''

import subprocess
import re
import os

from os import path
from pathlib import Path

try:
    from elftools.elf.elffile import ELFFile
except ImportError:
    ELFFile = None

from runners.core import ZephyrBinaryRunner, RunnerCaps

DEFAULT_OPENOCD_TCL_PORT = 6333
DEFAULT_OPENOCD_TELNET_PORT = 4444
DEFAULT_OPENOCD_GDB_PORT = 3333
DEFAULT_OPENOCD_RESET_HALT_CMD = 'reset halt'

class IntelCycloneVBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for openocd.'''

    def __init__(self, cfg, pre_init=None, reset_halt_cmd=DEFAULT_OPENOCD_RESET_HALT_CMD,
                 pre_load=None, load_cmd=None, verify_cmd=None, post_verify=None,
                 do_verify=False, do_verify_only=False,
                 tui=None, config=None, serial=None, use_elf=True,
                 no_halt=False, no_init=False, no_targets=False,
                 tcl_port=DEFAULT_OPENOCD_TCL_PORT,
                 telnet_port=DEFAULT_OPENOCD_TELNET_PORT,
                 gdb_port=DEFAULT_OPENOCD_GDB_PORT,
                 gdb_init=None, no_load=False):
        super().__init__(cfg)

        support = path.join(cfg.board_dir, 'support')

        if not config:
            default = path.join(support, 'openocd.cfg')
            default2 = path.join(support, 'download_all.gdb')
            default3 = path.join(support, 'appli_dl_cmd.gdb')
            default4 = path.join(support, 'appli_debug_cmd.gdb')
            if path.exists(default):
                config = [default]
                gdb_commands = [default2]
                gdb_commands2 = [default3]
                gdb_commands_deb = [default4]
        self.openocd_config = config
        self.gdb_cmds = gdb_commands
        self.gdb_cmds2 = gdb_commands2
        self.gdb_cmds_deb = gdb_commands_deb

        search_args = []
        if path.exists(support):
            search_args.append('-s')
            search_args.append(support)

        if self.openocd_config is not None:
            for i in self.openocd_config:
                if path.exists(i) and not path.samefile(path.dirname(i), support):
                    search_args.append('-s')
                    search_args.append(path.dirname(i))

        if cfg.openocd_search is not None:
            for p in cfg.openocd_search:
                search_args.extend(['-s', p])
        self.openocd_cmd = [cfg.openocd or 'openocd'] + search_args
        # openocd doesn't cope with Windows path names, so convert
        # them to POSIX style just to be sure.
        self.elf_name = Path(cfg.elf_file).as_posix()
        self.pre_init = pre_init or []
        self.reset_halt_cmd = reset_halt_cmd
        self.pre_load = pre_load or []
        self.load_cmd = load_cmd
        self.verify_cmd = verify_cmd
        self.post_verify = post_verify or []
        self.do_verify = do_verify or False
        self.do_verify_only = do_verify_only or False
        self.tcl_port = tcl_port
        self.telnet_port = telnet_port
        self.gdb_port = gdb_port
        self.gdb_cmd = [cfg.gdb] if cfg.gdb else None
        self.tui_arg = ['-tui'] if tui else []
        self.halt_arg = [] if no_halt else ['-c halt']
        self.init_arg = [] if no_init else ['-c init']
        self.targets_arg = [] if no_targets else ['-c targets']
        self.serial = ['-c set _ZEPHYR_BOARD_SERIAL ' + serial] if serial else []
        self.use_elf = use_elf
        self.gdb_init = gdb_init
        self.load_arg = [] if no_load else ['-ex', 'load']

    @classmethod
    def name(cls):
        return 'intel_cyclonev'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash', 'debug', 'attach'},
                          dev_id=False, flash_addr=False, erase=False)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--config', action='append',
                            help='''if given, override default config file;
                            may be given multiple times''')
        parser.add_argument('--serial', default="",
                            help='if given, selects FTDI instance by its serial number, defaults to empty')
        parser.add_argument('--use-elf', default=False, action='store_true',
                            help='if given, Elf file will be used for loading instead of HEX image')
        # Options for flashing:
        parser.add_argument('--cmd-pre-init', action='append',
                            help='''Command to run before calling init;
                            may be given multiple times''')
        parser.add_argument('--cmd-reset-halt', default=DEFAULT_OPENOCD_RESET_HALT_CMD,
                            help=f'''Command to run for resetting and halting the target,
                            defaults to "{DEFAULT_OPENOCD_RESET_HALT_CMD}"''')
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
        parser.add_argument('--verify', action='store_true',
                            help='if given, verify after flash')
        parser.add_argument('--verify-only', action='store_true',
                            help='if given, do verify and verify only. No flashing')

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
        parser.add_argument('--gdb-init', action='append',
                            help='if given, add GDB init commands')
        parser.add_argument('--no-halt', action='store_true',
                            help='if given, no halt issued in gdb server cmd')
        parser.add_argument('--no-init', action='store_true',
                            help='if given, no init issued in gdb server cmd')
        parser.add_argument('--no-targets', action='store_true',
                            help='if given, no target issued in gdb server cmd')
        parser.add_argument('--no-load', action='store_true',
                            help='if given, no load issued in gdb server cmd')

    @classmethod
    def do_create(cls, cfg, args):
        return IntelCycloneVBinaryRunner(
            cfg,
            pre_init=args.cmd_pre_init, reset_halt_cmd=args.cmd_reset_halt,
            pre_load=args.cmd_pre_load, load_cmd=args.cmd_load,
            verify_cmd=args.cmd_verify, post_verify=args.cmd_post_verify,
            do_verify=args.verify, do_verify_only=args.verify_only,
            tui=args.tui, config=args.config, serial=args.serial,
            use_elf=args.use_elf, no_halt=args.no_halt, no_init=args.no_init,
            no_targets=args.no_targets, tcl_port=args.tcl_port,
            telnet_port=args.telnet_port, gdb_port=args.gdb_port,
            gdb_init=args.gdb_init, no_load=args.no_load)

    def print_gdbserver_message(self):
        if not self.thread_info_enabled:
            thread_msg = '; no thread info available'
        elif self.supports_thread_info():
            thread_msg = '; thread info enabled'
        else:
            thread_msg = '; update OpenOCD software for thread info'
        self.logger.info('OpenOCD GDB server running on port '
                         f'{self.gdb_port}{thread_msg}')

    # pylint: disable=R0201
    def to_num(self, number):
        dev_match = re.search(r"^\d*\+dev", number)
        dev_version = not dev_match is None

        num_match = re.search(r"^\d*", number)
        num = int(num_match.group(0))

        if dev_version:
            num += 1

        return num

    def read_version(self):
        self.require(self.openocd_cmd[0])

	# OpenOCD prints in stderr, need redirect to get output
        out = self.check_output([self.openocd_cmd[0], '--version'],
                                stderr=subprocess.STDOUT).decode()

        return out.split('\n')[0]

    def supports_thread_info(self):
        # Zephyr rtos was introduced after 0.11.0
        version_str = self.read_version().split(' ')[3]
        version = version_str.split('.')
        (major, minor, rev) = [self.to_num(i) for i in version]
        return (major, minor, rev) > (0, 11, 0)

    def do_run(self, command, **kwargs):
        self.require(self.openocd_cmd[0])
        if ELFFile is None:
            raise RuntimeError(
                'elftools missing; please "pip3 install elftools"')

        self.cfg_cmd = []
        if self.openocd_config is not None:
            for i in self.openocd_config:
                self.cfg_cmd.append('-f')
                self.cfg_cmd.append(i)

        if command == 'flash' and self.use_elf:
            self.do_flash_elf(**kwargs)
        elif command == 'flash':
            self.do_flash_elf(**kwargs)
        elif command in ('attach', 'debug'):
            self.do_attach_debug(command, **kwargs)

    def do_flash_elf(self, **kwargs):
        if self.gdb_cmd is None:
            raise ValueError('Cannot debug; no gdb specified')
        if self.elf_name is None:
            raise ValueError('Cannot debug; no .elf specified')

        pre_init_cmd = []
        for i in self.pre_init:
            pre_init_cmd.append("-c")
            pre_init_cmd.append(i)
            pre_init_cmd.append("-q")

        if self.thread_info_enabled and self.supports_thread_info():
            pre_init_cmd.append("-c")
            pre_init_cmd.append("$_TARGETNAME configure -rtos Zephyr")

        server_cmd = (self.openocd_cmd + self.serial + self.cfg_cmd +        #added mevalver
                      pre_init_cmd)
        temp_str = '--cd=' + os.environ.get('ZEPHYR_BASE') #Go to Zephyr base Dir
        gdb_cmd = (self.gdb_cmd + self.tui_arg +
                   [temp_str,'-ex', 'target extended-remote localhost:{}'.format(self.gdb_port) , '-batch']) #Execute First Script in Zephyr Base Dir

        gdb_cmd2 = (self.gdb_cmd + self.tui_arg +
                   ['-ex', 'target extended-remote localhost:{}'.format(self.gdb_port) , '-batch'])	#Execute Second Script in Build Dir
        echo = ['echo']
        if self.gdb_init is not None:
            for i in self.gdb_init:
                gdb_cmd.append("-ex")
                gdb_cmd.append(i)
                gdb_cmd2.append("-ex")
                gdb_cmd2.append(i)

        if self.gdb_cmds is not None:
            for i in self.gdb_cmds:
                gdb_cmd.append("-x")
                gdb_cmd.append(i)

        if self.gdb_cmds2 is not None:
            for i in self.gdb_cmds2:
                gdb_cmd2.append("-x")
                gdb_cmd2.append(i)

        self.require(gdb_cmd[0])
        self.print_gdbserver_message()

        cmd1 = (echo + server_cmd)
        self.check_call(cmd1)
        cmd2 = (echo + gdb_cmd)
        self.check_call(cmd2)
        cmd3 = (echo + gdb_cmd2)
        self.check_call(cmd3)

        self.run_server_and_client(server_cmd, gdb_cmd)
        self.run_server_and_client(server_cmd, gdb_cmd2)

    def do_attach_debug(self, command, **kwargs):
        if self.gdb_cmd is None:
            raise ValueError('Cannot debug; no gdb specified')
        if self.elf_name is None:
            raise ValueError('Cannot debug; no .elf specified')

        pre_init_cmd = []
        for i in self.pre_init:
            pre_init_cmd.append("-c")
            pre_init_cmd.append(i)

        if self.thread_info_enabled and self.supports_thread_info():
            pre_init_cmd.append("-c")
            pre_init_cmd.append("$_TARGETNAME configure -rtos Zephyr")
            pre_init_cmd.append("-q")

        server_cmd = (self.openocd_cmd + self.serial + self.cfg_cmd +
                      pre_init_cmd)

        gdb_attach = (self.gdb_cmd + self.tui_arg +
                   ['-ex', 'target extended-remote :{}'.format(self.gdb_port),
                    self.elf_name, '-q'])

        temp_str = '--cd=' + os.environ.get('ZEPHYR_BASE') #Go to Zephyr base Dir

        gdb_cmd = (self.gdb_cmd + self.tui_arg +
                   [temp_str,'-ex', 'target extended-remote localhost:{}'.format(self.gdb_port) , '-batch']) #Execute First Script in Zephyr Base Dir

        gdb_cmd2 = (self.gdb_cmd + self.tui_arg +
                   ['-ex', 'target extended-remote :{}'.format(self.gdb_port) , '-batch'])	#Execute Second Script in Build Dir


        if self.gdb_init is not None:
            for i in self.gdb_init:
                gdb_cmd.append("-ex")
                gdb_cmd.append(i)
                gdb_cmd2.append("-ex")
                gdb_cmd2.append(i)

        if self.gdb_cmds is not None:
            for i in self.gdb_cmds:
                gdb_cmd.append("-x")
                gdb_cmd.append(i)

        if self.gdb_cmds_deb is not None:
            for i in self.gdb_cmds_deb:
                gdb_cmd2.append("-x")
                gdb_cmd2.append(i)

        self.require(gdb_cmd[0])
        self.print_gdbserver_message()

        if command == 'attach':
            self.run_server_and_client(server_cmd, gdb_attach)
        elif command == 'debug':
            self.run_server_and_client(server_cmd, gdb_cmd)
            self.run_server_and_client(server_cmd, gdb_cmd2)
            self.run_server_and_client(server_cmd, gdb_attach)
