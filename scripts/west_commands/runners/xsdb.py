# Copyright (c) 2024 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0

"""Runner for flashing and debugging with xsdb CLI, the official programming
utility from AMD platforms.
"""

import argparse
import os

from runners.core import RunnerCaps, RunnerConfig, ZephyrBinaryRunner

_XSDB_INTERACTIVE_TCL = os.path.join(os.path.dirname(__file__), 'xsdb_interactive.tcl')

_DEBUG_SESSION_HELP = """
XSDB Quick Reference (type help in XSDB for the full command list):
  con, stp, nxt, stpi - execution control
  bpadd, bplist       - breakpoints
  reload, restart     - reload helpers defined by the west xsdb runner
  exit                - quit
"""


class XSDBBinaryRunner(ZephyrBinaryRunner):
    def __init__(
        self,
        cfg: RunnerConfig,
        config=None,
        bitstream=None,
        fsbl=None,
        pdi=None,
        bl31=None,
        dtb=None,
        pmufw=None,
        params=None,
        hw_server_url=None,
    ):
        super().__init__(cfg)
        self.elf_file = cfg.elf_file

        if not config:
            cfgfile_path = os.path.join(cfg.board_dir, 'support')
            for name in ('xsdb_cfg.tcl', 'xsdb.cfg'):
                candidate = os.path.join(cfgfile_path, name)
                if os.path.exists(candidate):
                    config = candidate
                    break
        self.xsdb_cfg_file = config
        self.bitstream = bitstream
        self.fsbl = fsbl
        self.pdi = pdi
        self.bl31 = bl31
        self.dtb = dtb
        self.pmufw = pmufw
        # Generic name/value pass-through for anything a board's xsdb.cfg
        # accepts that isn't one of the fixed options above (e.g. "pl_pdi"
        # for partial reconfiguration, or a one-off board-specific value).
        self.params = [tuple(param) for param in params] if params else []

        # Debug configuration (--hw-server only; otherwise inherit board cfg behavior)
        self.hw_server_url = hw_server_url

    @classmethod
    def name(cls):
        return 'xsdb'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash', 'debug', 'debugserver'}, flash_addr=True)

    @classmethod
    def do_add_parser(cls, parser):
        # Flash options
        parser.add_argument('--config', help='if given, override default config file')
        parser.add_argument('--bitstream', help='path to the bitstream file')
        parser.add_argument('--fsbl', help='path to the fsbl elf file')
        parser.add_argument('--pdi', help='path to the base/boot pdi file')
        parser.add_argument('--bl31', help='path to the bl31(ATF) elf file')
        parser.add_argument('--system-dtb', help='path to the system.dtb file')
        parser.add_argument('--pmufw', help='path to the PMU firmware elf file (ZynqMP)')
        parser.add_argument(
            '--param',
            dest='params',
            action='append',
            nargs=2,
            metavar=('NAME', 'VALUE'),
            help="""
            name/value pair for a parameter not covered by the options
            above, forwarded to the board xsdb.cfg the same way, e.g.
            "--param pl_pdi /path/to/rp0.pdi" for a PL PDI (programs FPGA
            fabric). May be repeated, including with the same NAME multiple
            times to load several PL PDIs in order for partial
            reconfiguration. Only takes effect on boards whose xsdb.cfg
            declares that parameter name.
            """,
        )

        # Debug options
        parser.add_argument(
            '--hw-server',
            help='hw_server URL (format: TCP:<host>:<port>). When omitted, the '
            'board xsdb.cfg uses connect without -url, or HW_SERVER_URL from the '
            'environment if set.',
        )

    @classmethod
    def do_create(cls, cfg: RunnerConfig, args: argparse.Namespace) -> "XSDBBinaryRunner":
        return XSDBBinaryRunner(
            cfg,
            config=args.config,
            bitstream=args.bitstream,
            fsbl=args.fsbl,
            pdi=args.pdi,
            bl31=args.bl31,
            dtb=args.system_dtb,
            pmufw=args.pmufw,
            params=args.params,
            hw_server_url=args.hw_server,
        )

    def _boot_args(self):
        """Build the flat "key value key value ..." argument list consumed by
        the board xsdb.cfg's parse_args helper.

        Board cfg scripts populate an associative array from these pairs, so
        unlike a positional argument list, order does not matter here and
        each board only has to declare the keys it actually understands.
        Boards that do not recognize a given key reject it with a clear
        Tcl-level error instead of silently misreading a positional slot.
        """
        args = ['elf', self.elf_file]
        for key, value in (
            ('bitstream', self.bitstream),
            ('fsbl', self.fsbl),
            ('pdi', self.pdi),
            ('bl31', self.bl31),
            ('dtb', self.dtb),
            ('pmufw', self.pmufw),
        ):
            if value is not None:
                args.extend((key, value))

        # Generic --param pass-through. Tcl's "array set" keeps only the
        # last value for a duplicate key, so repeated --param calls sharing
        # a NAME (e.g. multiple PL PDIs for partial reconfiguration) are
        # merged into one space-joined Tcl list value under that key;
        # boards read it back with "foreach v $args_arr(name) { ... }".
        merged = {}
        for name, value in self.params:
            merged[name] = f'{merged[name]} {value}' if name in merged else value
        for name, value in merged.items():
            args.extend((name, value))
        return args

    def do_run(self, command, **kwargs):
        if command == 'flash':
            self.do_flash(**kwargs)
        elif command in ('debug', 'debugserver'):
            # Both debug and debugserver use native XSDB
            self.do_debug_native(**kwargs)
        else:
            raise ValueError(f'Unsupported command: {command}')

    def do_flash(self, **kwargs):
        """Flash the target using XSDB."""
        self.require('xsdb')

        cmd = ['xsdb', self.xsdb_cfg_file, *self._boot_args()]
        self.check_call(cmd)

    def _write_xsdb_debug_params(self) -> str:
        """Write the debug parameter file consumed by the interactive script.

        Parameters are emitted as a simple ``key = value`` data file (not
        environment variables and not executable Tcl). The interactive Tcl
        script parses them into a local dictionary, so no state leaks into
        global variables and the script can be run standalone for
        troubleshooting:

            xsdb -interactive xsdb_interactive.tcl <build_dir>/xsdb_debug_params.ini

        ``boot_arg`` may repeat; the ordered values are the same flat
        "key value key value ..." tokens passed to the board's
        ``load_image`` on a flash, so a debug session boots identically.
        """
        if not self.xsdb_cfg_file or not self.elf_file:
            raise RuntimeError('XSDB debug requires board config and ELF file')

        lines = [
            '# Auto-generated by the west xsdb runner (west debug/debugserver).',
            '# Regenerated on every debug session; do not edit by hand.',
            f'board_cfg = {self.xsdb_cfg_file}',
            f'elf = {self.elf_file}',
        ]
        if self.hw_server_url is not None:
            lines.append(f'hw_server = {self.hw_server_url}')
        for token in self._boot_args():
            lines.append(f'boot_arg = {token}')

        params_path = os.path.join(self.cfg.build_dir, 'xsdb_debug_params.ini')
        with open(params_path, 'w') as params_file:
            params_file.write('\n'.join(lines) + '\n')

        return params_path

    def do_debug_native(self, **kwargs):
        """Start native XSDB interactive debugging session.

        This is used for both 'debug' and 'debugserver' commands.
        Uses the board's xsdb.cfg which contains the proper target selection
        and platform initialization specific to each board.
        """
        self.require('xsdb')

        if not self.xsdb_cfg_file or not os.path.exists(self.xsdb_cfg_file):
            raise ValueError(
                f'Board configuration file not found: {self.xsdb_cfg_file}\n'
                'XSDB debugging requires a board-specific xsdb.cfg file.\n'
                'Please ensure your board has support/xsdb.cfg in its directory.'
            )

        if not os.path.isfile(_XSDB_INTERACTIVE_TCL):
            raise RuntimeError(f'XSDB interactive script not found: {_XSDB_INTERACTIVE_TCL}')

        params_file = self._write_xsdb_debug_params()

        self.logger.info('Starting XSDB native debug session')
        self.logger.info(f'Using board config: {self.xsdb_cfg_file}')
        self.logger.info(f'Debug parameters: {params_file}')
        if self.hw_server_url is not None:
            hw_server_msg = self.hw_server_url
        else:
            hw_server_msg = 'local default (board xsdb.cfg connect without -url)'
        self.logger.info(f'hw_server: {hw_server_msg}')
        self.logger.info(_DEBUG_SESSION_HELP.strip())

        cmd = ['xsdb', '-interactive', _XSDB_INTERACTIVE_TCL, params_file]
        self.check_call(cmd)
