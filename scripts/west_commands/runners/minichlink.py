# Copyright (c) 2024 Google LLC.
#
# SPDX-License-Identifier: Apache-2.0

"""WCH CH32V00x specific runner."""

import argparse

from runners.core import RunnerCaps, RunnerConfig, ZephyrBinaryRunner


class MiniChLinkBinaryRunner(ZephyrBinaryRunner):
    """Runner for CH32V00x based devices using minichlink."""

    def __init__(
        self,
        cfg: RunnerConfig,
        minichlink: str,
        erase: bool,
        reset: bool,
        dt_flash: bool,
        terminal: bool,
    ):
        super().__init__(cfg)

        self.minichlink = minichlink
        self.erase = erase
        self.reset = reset
        self.dt_flash = dt_flash
        self.terminal = terminal

    @classmethod
    def name(cls):
        return "minichlink"

    @classmethod
    def capabilities(cls) -> RunnerCaps:
        return RunnerCaps(commands={"flash"}, flash_addr=True, erase=True, reset=True)

    @classmethod
    def do_add_parser(cls, parser: argparse.ArgumentParser):
        parser.add_argument(
            "--minichlink", default="minichlink", help="path to the minichlink binary"
        )
        parser.add_argument(
            "--terminal",
            default=False,
            action=argparse.BooleanOptionalAction,
            help="open the terminal after flashing. Implies --reset.",
        )
        parser.set_defaults(reset=True)

    @classmethod
    def do_create(cls, cfg: RunnerConfig, args: argparse.Namespace):
        return MiniChLinkBinaryRunner(
            cfg,
            minichlink=args.minichlink,
            erase=args.erase,
            reset=args.reset,
            dt_flash=(args.dt_flash == "y"),
            terminal=args.terminal,
        )

    def do_run(self, command: str, **kwargs):
        self.require(self.minichlink)

        if command == "flash":
            self.flash()
        else:
            raise ValueError("BUG: unhandled command f{command}")

    def flash(self):
        self.ensure_output("bin")

        cmd = [self.minichlink, "-a"]

        if self.erase:
            cmd.append("-E")

        flash_addr = 0
        if self.dt_flash:
            flash_addr = self.flash_address_from_build_conf(self.build_conf)

        cmd.extend(["-w", self.cfg.bin_file or "", f"0x{flash_addr:x}"])

        if self.reset or self.terminal:
            cmd.append("-b")

        if self.terminal:
            cmd.append("-T")

        if self.terminal:
            self.check_call(cmd)
        else:
            self.check_output(cmd)
