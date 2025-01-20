# Copyright (c) 2017 Linaro Limited.
# Copyright (c) 2024 Tenstorrent AI ULC
#
# SPDX-License-Identifier: Apache-2.0
#
# pylint: disable=duplicate-code

'''Runner for openocd.'''

import re
import subprocess
from os import name as os_name
from os import path
from pathlib import Path

from zephyr_ext_common import ZEPHYR_BASE

if os_name != "nt":
    import sys
    import termios

try:  # noqa SIM105
    from elftools.elf.elffile import ELFFile
except ImportError:
    pass

from runners.core import RunnerCaps, ZephyrBinaryRunner

DEFAULT_OPENOCD_TCL_PORT = 6333
DEFAULT_OPENOCD_TELNET_PORT = 4444
DEFAULT_OPENOCD_GDB_PORT = 3333
DEFAULT_OPENOCD_RTT_PORT = 5555
DEFAULT_OPENOCD_RESET_HALT_CMD = 'reset init'
DEFAULT_OPENOCD_TARGET_HANDLE = "_TARGETNAME"

def to_num(number):
    dev_match = re.search(r"^\d*\+dev", number)
    dev_version = dev_match is not None

    num_match = re.search(r"^\d*", number)
    num = int(num_match.group(0))

    if dev_version:
        num += 1

    return num

class OpenOcdBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for openocd.'''

    def __init__(self, cfg, pre_init=None, reset_halt_cmd=DEFAULT_OPENOCD_RESET_HALT_CMD,
                 pre_load=None, erase_cmd=None, load_cmd=None, verify_cmd=None,
                 post_verify=None, do_verify=False, do_verify_only=False, do_erase=False,
                 tui=None, config=None, serial=None, use_elf=None,
                 no_halt=False, no_init=False, no_targets=False,
                 tcl_port=DEFAULT_OPENOCD_TCL_PORT,
                 telnet_port=DEFAULT_OPENOCD_TELNET_PORT,
                 gdb_port=DEFAULT_OPENOCD_GDB_PORT,
                 gdb_client_port=DEFAULT_OPENOCD_GDB_PORT,
                 gdb_init=None, no_load=False,
                 target_handle=DEFAULT_OPENOCD_TARGET_HANDLE,
                 rtt_port=DEFAULT_OPENOCD_RTT_PORT):
        super().__init__(cfg)

        if not path.exists(cfg.board_dir):
            # try to find the board support in-tree
            cfg_board_path = path.normpath(cfg.board_dir)
            _temp_path = cfg_board_path.split("boards/")[1]
            support = path.join(ZEPHYR_BASE, "boards", _temp_path, 'support')
        else:
            support = path.join(cfg.board_dir, 'support')


        if not config:
            default = path.join(support, 'openocd.cfg')
            if path.exists(default):
                config = [default]
        self.openocd_config = config

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
        self.elf_name = Path(cfg.elf_file).as_posix() if cfg.elf_file else None
        self.pre_init = pre_init or []
        self.reset_halt_cmd = reset_halt_cmd
        self.pre_load = pre_load or []
        self.erase_cmd = erase_cmd
        self.load_cmd = load_cmd
        self.verify_cmd = verify_cmd
        self.post_verify = post_verify or []
        self.do_verify = do_verify or False
        self.do_verify_only = do_verify_only or False
        self.do_erase = do_erase or False
        self.tcl_port = tcl_port
        self.telnet_port = telnet_port
        self.gdb_port = gdb_port
        self.gdb_client_port = gdb_client_port
        self.gdb_cmd = [cfg.gdb] if cfg.gdb else None
        self.tui_arg = ['-tui'] if tui else []
        self.halt_arg = [] if no_halt else ['-c halt']
        self.init_arg = [] if no_init else ['-c init']
        self.targets_arg = [] if no_targets else ['-c targets']
        self.serial = ['-c set _ZEPHYR_BOARD_SERIAL ' + serial] if serial else []
        self.use_elf = use_elf
        self.gdb_init = gdb_init
        self.load_arg = [] if no_load else ['-ex', 'load']
        self.target_handle = target_handle
        self.rtt_port = rtt_port

    @classmethod
    def name(cls):
        return 'openocd'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash', 'debug', 'debugserver', 'attach', 'rtt'},
                          rtt=True, erase=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--config', action='append',
                            help='''if given, override default config file;
                            may be given multiple times''')
        parser.add_argument('--serial', default="",
                            help='''if given, selects FTDI instance by its serial number,
                            defaults to empty''')
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
        parser.add_argument('--cmd-erase', action='append',
                            help='''Command to erase device; may be given multiple times''')
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
        parser.add_argument('--gdb-client-port', default=DEFAULT_OPENOCD_GDB_PORT,
                            help='''openocd gdb client port if multiple ports come
                            up, defaults to 3333''')
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
        parser.add_argument('--target-handle', default=DEFAULT_OPENOCD_TARGET_HANDLE,
                            help=f'''Internal handle used in openocd targets cfg
                            files, defaults to "{DEFAULT_OPENOCD_TARGET_HANDLE}".
                            ''')
        parser.add_argument('--rtt-port', default=DEFAULT_OPENOCD_RTT_PORT,
                            help='openocd rtt port, defaults to 5555')


    @classmethod
    def do_create(cls, cfg, args):
        return OpenOcdBinaryRunner(
            cfg,
            pre_init=args.cmd_pre_init, reset_halt_cmd=args.cmd_reset_halt,
            pre_load=args.cmd_pre_load, erase_cmd=args.cmd_erase, load_cmd=args.cmd_load,
            verify_cmd=args.cmd_verify, post_verify=args.cmd_post_verify,
            do_verify=args.verify, do_verify_only=args.verify_only, do_erase=args.erase,
            tui=args.tui, config=args.config, serial=args.serial,
            use_elf=args.use_elf, no_halt=args.no_halt, no_init=args.no_init,
            no_targets=args.no_targets, tcl_port=args.tcl_port,
            telnet_port=args.telnet_port, gdb_port=args.gdb_port,
            gdb_client_port=args.gdb_client_port, gdb_init=args.gdb_init,
            no_load=args.no_load, target_handle=args.target_handle,
            rtt_port=args.rtt_port)

    def print_gdbserver_message(self):
        if not self.thread_info_enabled:
            thread_msg = '; no thread info available'
        elif self.supports_thread_info():
            thread_msg = '; thread info enabled'
        else:
            thread_msg = '; update OpenOCD software for thread info'
        self.logger.info('OpenOCD GDB server running on port '
                         f'{self.gdb_port}{thread_msg}')

    def print_rttserver_message(self):
        self.logger.info(f'OpenOCD RTT server running on port {self.rtt_port}')

    def read_version(self):
        self.require(self.openocd_cmd[0])

        # OpenOCD prints in stderr, need redirect to get output
        out = self.check_output([self.openocd_cmd[0], '--version'],
                                stderr=subprocess.STDOUT).decode()

        version_match = re.search(r"Open On-Chip Debugger v?(\d+.\d+.\d+)", out)
        version = version_match.group(1).split('.')

        return [to_num(i) for i in version]

    def supports_thread_info(self):
        # Zephyr rtos was introduced after 0.11.0
        (major, minor, rev) = self.read_version()
        return (major, minor, rev) > (0, 11, 0)

    def do_run(self, command, **kwargs):
        self.require(self.openocd_cmd[0])
        if globals().get('ELFFile') is None:
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
            self.do_flash(**kwargs)
        elif command in ('attach', 'debug', 'rtt'):
            self.do_attach_debug_rtt(command, **kwargs)
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

        self.logger.info(f'Flashing file: {hex_name}')

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

        load_image = []
        if not self.do_verify_only:
            # Halt target
            load_image = ['-c', self.reset_halt_cmd]
            # Perform any erase operations
            if self.do_erase:
                if self.erase_cmd is None:
                    self.logger.error('--erase not supported for target without --cmd-erase')
                    return
                for erase_cmd in self.erase_cmd:
                    load_image += ["-c", erase_cmd]
                # Trim the "erase" from "flash write_image erase" since a mass erase is already done
                if self.load_cmd.endswith(' erase'):
                    self.load_cmd = self.load_cmd[:-6]
            # Load image
            load_image +=['-c', self.load_cmd + ' ' + hex_name]

        verify_image = []
        if self.do_verify or self.do_verify_only:
            verify_image = ['-c', self.reset_halt_cmd,
                            '-c', self.verify_cmd + ' ' + hex_name]

        cmd = (self.openocd_cmd + self.serial + self.cfg_cmd +
               pre_init_cmd + self.init_arg + self.targets_arg +
               pre_load_cmd + load_image +
               verify_image +
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

        pre_load_cmd = []
        load_image = []
        if not self.do_verify_only:
            for i in self.pre_load:
                pre_load_cmd.append("-c")
                pre_load_cmd.append(i)
            load_image = ['-c', 'load_image ' + self.elf_name]

        verify_image = []
        post_verify_cmd = []
        if self.do_verify or self.do_verify_only:
            verify_image = ['-c', 'verify_image ' + self.elf_name]
            for i in self.post_verify:
                post_verify_cmd.append("-c")
                post_verify_cmd.append(i)

        prologue = ['-c', 'resume ' + ep_addr,
                    '-c', 'shutdown']

        cmd = (self.openocd_cmd + self.serial + self.cfg_cmd +
               pre_init_cmd + self.init_arg + self.targets_arg +
               pre_load_cmd + ['-c', self.reset_halt_cmd] +
               load_image +
               verify_image + post_verify_cmd +
               prologue)

        self.check_call(cmd)

    def do_attach_debug_rtt(self, command, **kwargs):
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
            rtos_command = f'${self.target_handle} configure -rtos Zephyr'
            pre_init_cmd.append(rtos_command)

        server_cmd = (self.openocd_cmd + self.serial + self.cfg_cmd +
                      ['-c', f'tcl_port {self.tcl_port}',
                       '-c', f'telnet_port {self.telnet_port}',
                       '-c', f'gdb_port {self.gdb_port}'] +
                      pre_init_cmd + self.init_arg + self.targets_arg +
                      self.halt_arg)
        gdb_cmd = (self.gdb_cmd + self.tui_arg +
                   ['-ex', f'target extended-remote :{self.gdb_client_port}',
                    self.elf_name])
        if command == 'debug':
            gdb_cmd.extend(self.load_arg)
        if self.gdb_init is not None:
            for i in self.gdb_init:
                gdb_cmd.append("-ex")
                gdb_cmd.append(i)
        if command == 'rtt':
            rtt_address = self.get_rtt_address()
            if rtt_address is None:
                raise ValueError("RTT Control block not be found")

            # cannot prompt the user to press return for automation purposes
            gdb_cmd.extend(['-ex', 'set pagination off'])
            # start the internal openocd rtt service via gdb monitor commands
            gdb_cmd.extend(
                ['-ex', f'monitor rtt setup 0x{rtt_address:x} 0x10 "SEGGER RTT"'])
            gdb_cmd.extend(['-ex', 'monitor reset run'])
            gdb_cmd.extend(['-ex', 'monitor rtt start'])
            gdb_cmd.extend(
                ['-ex', f'monitor rtt server start {self.rtt_port} 0'])
            # detach from the target and quit the gdb client session
            gdb_cmd.extend(['-ex', 'detach', '-ex', 'quit'])

        self.require(gdb_cmd[0])
        self.print_gdbserver_message()

        if command in ('attach', 'debug'):
            server_proc = self.popen_ignore_int(server_cmd, stderr=subprocess.DEVNULL)
            try:
                self.run_client(gdb_cmd)
            finally:
                server_proc.terminate()
                server_proc.wait()
        elif command == 'rtt':
            self.print_rttserver_message()
            server_proc = self.popen_ignore_int(server_cmd)

            if os_name != 'nt':
                # Save the terminal settings
                fd = sys.stdin.fileno()
                new_term = termios.tcgetattr(fd)
                old_term = termios.tcgetattr(fd)

                # New terminal setting unbuffered
                new_term[3] = new_term[3] & ~termios.ICANON & ~termios.ECHO
                termios.tcsetattr(fd, termios.TCSAFLUSH, new_term)
            else:
                fd = None
                old_term = None

            try:
                # run the binary with gdb, set up the rtt server (runs to completion)
                subprocess.run(gdb_cmd)
                # run the rtt client in the foreground
                self.run_telnet_client('localhost', self.rtt_port)
            finally:
                if old_term is not None and fd is not None:
                    termios.tcsetattr(fd, termios.TCSAFLUSH, old_term)

                server_proc.terminate()
                server_proc.wait()

    def do_debugserver(self, **kwargs):
        pre_init_cmd = []
        for i in self.pre_init:
            pre_init_cmd.append("-c")
            pre_init_cmd.append(i)

        if self.thread_info_enabled and self.supports_thread_info():
            pre_init_cmd.append("-c")
            rtos_command = f'${self.target_handle} configure -rtos Zephyr'
            pre_init_cmd.append(rtos_command)

        cmd = (self.openocd_cmd + self.cfg_cmd +
               ['-c', f'tcl_port {self.tcl_port}',
                '-c', f'telnet_port {self.telnet_port}',
                '-c', f'gdb_port {self.gdb_port}'] +
               pre_init_cmd + self.init_arg + self.targets_arg +
               ['-c', self.reset_halt_cmd])
        self.print_gdbserver_message()
        self.check_call(cmd)
