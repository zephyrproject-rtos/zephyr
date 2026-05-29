# Copyright (c) 2025 Core Devices LLC
# Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
# SPDX-License-Identifier: Apache-2.0

import argparse
import shlex
import subprocess

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
        flash_files: list[str],
    ) -> None:
        super().__init__(cfg)

        self._chip = chip
        self._port = port
        self._erase = erase
        self._dt_flash = dt_flash
        self._flash_files = flash_files

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
        parser.add_argument(
            "--flash-file",
            dest="flash_files",
            action="append",
            default=[],
            help="Additional file@address entries that must be flashed before the Zephyr image",
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
            flash_files=args.flash_files,
        )

    def do_run(self, command: str, **kwargs):
        sftool = self.require("sftool")

        cmd = [sftool, "--chip", self._chip, "--port", self._port]
        cmd += self._tool_opt

        flash_targets: list[str] = []
        flash_targets.extend(self._flash_files)

        if self.cfg.file:
            flash_targets.append(self.cfg.file)
        else:
            if self.cfg.bin_file and self._dt_flash:
                addr = self.flash_address_from_build_conf(self.build_conf)
                flash_targets.append(f"{self.cfg.bin_file}@0x{addr:08x}")
            elif self.cfg.hex_file:
                flash_targets.append(self.cfg.hex_file)
            else:
                raise RuntimeError("No file available for flashing")

        write_args = ["write_flash"]
        if self._erase:
            write_args.append("-e")

        full_cmd = cmd + write_args + flash_targets
        self._log_cmd(full_cmd)
        if not self.dry_run:
            subprocess.run(full_cmd, check=True)
