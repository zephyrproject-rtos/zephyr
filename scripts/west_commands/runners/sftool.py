# Copyright (c) 2025 Core Devices LLC
# SPDX-License-Identifier: Apache-2.0

import argparse
import shlex

from runners.core import RunnerCaps, RunnerConfig, ZephyrBinaryRunner


class SftoolRunner(ZephyrBinaryRunner):
    """Runner front-end for sftool CLI."""

    def __init__(
        self,
        cfg: RunnerConfig,
        chip: str,
        port: str,
        erase: bool,
        dt_flash: bool,
        tool_opt: list[str],
    ) -> None:
        super().__init__(cfg)

        self._chip = chip
        self._port = port
        self._erase = erase
        self._dt_flash = dt_flash

        self._tool_opt: list[str] = []
        for opts in [shlex.split(opt) for opt in tool_opt]:
            self._tool_opt += opts

    @classmethod
    def name(cls):
        return "sftool"

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={"flash"}, erase=True, flash_addr=True, file=True, tool_opt=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument(
            "--chip",
            type=str,
            required=True,
            help="Target chip, e.g. SF32LB52",
        )

        parser.add_argument(
            "--port",
            type=str,
            required=True,
            help="Serial port device, e.g. /dev/ttyUSB0",
        )

    @classmethod
    def do_create(cls, cfg: RunnerConfig, args: argparse.Namespace) -> "SftoolRunner":
        return SftoolRunner(
            cfg,
            chip=args.chip,
            port=args.port,
            erase=args.erase,
            dt_flash=args.dt_flash,
            tool_opt=args.tool_opt,
        )

    def do_run(self, command: str, **kwargs):
        sftool = self.require("sftool")

        cmd = [sftool, "--chip", self._chip, "--port", self._port]
        cmd += self._tool_opt

        if self._erase:
            self.check_call(cmd + ["erase_flash"])

        if self.cfg.file:
            self.check_call(cmd + ["write_flash", self.cfg.file])
        else:
            if self.cfg.bin_file and self._dt_flash:
                addr = self.flash_address_from_build_conf(self.build_conf)
                self.check_call(cmd + ["write_flash", f"{self.cfg.bin_file}@0x{addr:08x}"])
            elif self.cfg.hex_file:
                self.check_call(cmd + ["write_flash", self.cfg.hex_file])
            else:
                raise RuntimeError("No file available for flashing")
