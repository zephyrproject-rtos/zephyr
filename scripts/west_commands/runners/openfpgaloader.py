# SPDX-FileCopyrightText: Copyright (c) 2026 TOKITA Hiroshi
# SPDX-License-Identifier: Apache-2.0

"""Runner for the openFPGALoader programming tool."""

import argparse

from runners.core import RunnerCaps, ZephyrBinaryRunner


class OpenFPGALoaderBinaryRunner(ZephyrBinaryRunner):
    """Runner front-end for openFPGALoader."""

    def __init__(
        self, cfg, board, external_flash=False, verify=False, offset=None, usb_serial=None
    ):
        super().__init__(cfg)
        self.board = board
        self.external_flash = external_flash
        self.verify = verify
        self.offset = offset
        self.usb_serial = usb_serial

    @classmethod
    def name(cls):
        return "openfpgaloader"

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={"flash"})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument("--board", required=True, help="openFPGALoader board name")
        parser.add_argument("--usb-serial-num", help="USB serial number of the programmer")
        parser.add_argument("--offset", help="flash offset")
        parser.add_argument(
            "--external-flash",
            action=argparse.BooleanOptionalAction,
            help="select the external flash",
        )
        parser.add_argument(
            "--verify", action=argparse.BooleanOptionalAction, help="verify the flash contents"
        )

    @classmethod
    def do_create(cls, cfg, args):
        return OpenFPGALoaderBinaryRunner(
            cfg,
            args.board,
            external_flash=args.external_flash,
            verify=args.verify,
            offset=args.offset,
            usb_serial=args.usb_serial_num,
        )

    def do_run(self, command, **kwargs):
        self.require("openFPGALoader")
        self.ensure_output("bin")

        command = ["openFPGALoader", "--board", self.board]
        if self.usb_serial:
            command.extend(("--usb-serial-num", self.usb_serial))
        if self.external_flash:
            command.append("--external-flash")
        command.append("--write-flash")
        if self.offset:
            command.extend(("--offset", self.offset))
        if self.verify:
            command.append("--verify")
        command.append(self.cfg.bin_file)

        self.check_call(command)
