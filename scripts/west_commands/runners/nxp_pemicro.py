# SPDX-FileCopyrightText: Copyright (c) 2026 Sachin Kumar
# SPDX-License-Identifier: Apache-2.0

"""Runner for PeMicro gdb server for OpenSDA Debug Probe."""

import os

from runners.core import RunnerCaps, ZephyrBinaryRunner


class NxPPemicroRunner(ZephyrBinaryRunner):
    """Runner for NXP PEmicro hardware using pegdbserver."""

    def __init__(self, cfg, device, gdb, dev_id):
        super().__init__(cfg)
        self.device = device
        self.gdb = gdb
        self.dev_id = dev_id

    @classmethod
    def name(cls):
        return "nxp_pemicro"

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={"debug", "flash"}, dev_id=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument(
            "--device",
            required=True,
            help="NXP device name for pegdbserver",
        )

        # Required for board_runner_args compatibility
        parser.add_argument("--core-name")
        parser.add_argument("--soc-name")
        parser.add_argument("--soc-family-name")
        parser.add_argument("--speed")

    @classmethod
    def do_create(cls, cfg, args):
        return cls(
            cfg=cfg,
            device=args.device,
            gdb=cfg.gdb,
            dev_id=args.dev_id,
        )

    def do_run(self, command, **kwargs):
        self.require("pegdbserver_console.exe")

        if self.gdb:
            self.require(
                os.path.basename(self.gdb),
                path=os.path.dirname(self.gdb),
            )

        server_cmd = [
            "pegdbserver_console.exe",
            "-startserver",
            f"-device={self.device}",
            "-interface=USBMULTILINK",
            f"-port={self.dev_id}",
        ]

        gdb_cmd = [
            self.gdb,
            self.cfg.elf_file,
            "-ex",
            "target remote :7224",
            "-ex",
            "load",
        ]

        if command == "flash":
            gdb_cmd.append("-batch")

        self.run_server_and_client(server_cmd, gdb_cmd)
