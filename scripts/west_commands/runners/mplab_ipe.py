# Copyright (c) 2017 Linaro Limited.
# Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
# Copyright (c) 2026 Muhammed Asif P
# SPDX-License-Identifier: Apache-2.0

import os
import shlex

from runners.core import RunnerCaps, RunnerConfig, ZephyrBinaryRunner


class MPLABIPEBinaryRunner(ZephyrBinaryRunner):
    """Runner for Flashing Application using Microchip MPLAB IPE"""

    def __init__(
        self, cfg: RunnerConfig, tool: str, device: str, erase: bool = False, verify: bool = False
    ):
        super().__init__(cfg)
        self.tool = tool
        self.device = device
        self.erase = erase
        self.verify = verify

    @classmethod
    def name(cls):
        return "mplab_ipe"

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={"flash"}, erase=True, verify=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument("--tool", required=True, help="Programmer tool name, e.g. PKOB4")
        parser.add_argument(
            "--device", required=True, help="Target device name, e.g. 32CX1025SG61128"
        )

    @classmethod
    def do_create(cls, cfg: RunnerConfig, args):
        return MPLABIPEBinaryRunner(
            cfg, tool=args.tool, device=args.device, erase=args.erase, verify=args.verify
        )

    def do_run(self, command: str, **kwargs):
        if command == "flash":
            self.flash()

    def flash(self):
        hex_file = self.cfg.hex_file

        if not hex_file or not os.path.isfile(hex_file):
            raise RuntimeError(f"Hex file not found: {hex_file}")

        exe = "ipecmd.exe" if os.name == "nt" else "ipecmd.sh"

        cmd = [
            exe,
            f"-TP{self.tool}",
            f"-P{self.device}",
            f"-F{hex_file}",
            "-M",
            "-OL",
            "-OK",
        ]

        if self.erase:
            cmd.append("-E")

        if self.verify:
            cmd.append("-V")
            # cmd.append("-YP")

        self.logger.info("Running: %s", " ".join(shlex.quote(p) for p in cmd))
        self.check_call(cmd)
