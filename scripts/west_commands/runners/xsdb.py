# Copyright (c) 2024 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0

"""Runner for flashing with xsdb CLI, the official programming
   utility from AMD platforms.
"""
import argparse
import os

from runners.core import RunnerCaps, RunnerConfig, ZephyrBinaryRunner


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

    @classmethod
    def name(cls):
        return 'xsdb'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(flash_addr=True)

    @classmethod
    def do_add_parser(cls, parser):
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
        cmd = ['xsdb', self.xsdb_cfg_file, *self._boot_args()]
        self.check_call(cmd)
