# Copyright (c) 2017 Linaro Limited.
# Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
# Copyright (c) 2026 Muhammed Asif P
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import shlex

from runners.core import RunnerCaps, RunnerConfig, ZephyrBinaryRunner


class MPLABIPEBinaryRunner(ZephyrBinaryRunner):
    """Runner for Flashing Application using Microchip MPLAB IPE"""

    def __init__(
        self,
        cfg: RunnerConfig,
        tool: str,
        part: str,
        dev_id: str | None = None,
        erase: bool = False,
        verify: bool = False,
    ):
        super().__init__(cfg)
        self.tool = tool
        self.part = part
        self.dev_id = dev_id
        self.erase = erase
        self.verify = verify

    @classmethod
    def name(cls):
        return "mplab_ipe"

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={"flash"}, erase=True, dev_id=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument(
            "--tool",
            required=True,
            help="Programming probe model, e.g. PKOB4. Passed unchanged to the MLAB IPE",
        )
        parser.add_argument(
            "--part", required=True, help="Target device name, e.g. 32CX1025SG61128"
        )
        parser.add_argument(
            "--verify", action=argparse.BooleanOptionalAction, help="Verify after programming"
        )

    @classmethod
    def do_create(cls, cfg: RunnerConfig, args):
        return MPLABIPEBinaryRunner(
            cfg,
            tool=args.tool,  # probe type
            part=args.part,  # part number of the SoC
            dev_id=args.dev_id,
            erase=args.erase,
            verify=args.verify,
        )

    def do_run(self, command: str, **kwargs):
        if command == "flash":
            self.flash()
        else:
            raise ValueError(f"Unsupported command: {command}")

    def flash(self):
        self.ensure_output('hex')
        hex_file = self.cfg.hex_file

        exe = self.require("ipecmd.exe" if os.name == "nt" else "ipecmd.sh")

        cmd = [
            exe,
            f"-TP{self.tool}",  # probe type
            f"-P{self.part}",  # part number of the SoC
            f"-F{hex_file}",
            "-M",  # Program all memories
            "-OL",  # Release from reset
            "-OK",  # Silent connect with the tool and hardware
        ]

        if self.dev_id:
            cmd.append(f"-TS{self.dev_id}")  # Select programmer by serial number

        if self.erase:
            cmd.append("-E")

        if self.verify:
            cmd.append("-V")

        self.logger.info("Running: %s", " ".join(shlex.quote(p) for p in cmd))
        self.check_call(cmd)
