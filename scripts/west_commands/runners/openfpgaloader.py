# SPDX-FileCopyrightText: Copyright (c) 2026 TOKITA Hiroshi
# SPDX-License-Identifier: Apache-2.0

"""Runner for the openFPGALoader programming tool."""

from runners.core import RunnerCaps, ZephyrBinaryRunner


class OpenFPGALoaderBinaryRunner(ZephyrBinaryRunner):
    """Runner front-end for openFPGALoader."""

    def __init__(self, cfg, board, external_flash=False, verify=False, offset=None):
        super().__init__(cfg)
        self.board = board
        self.external_flash = external_flash
        self.verify = verify
        self.offset = offset

    @classmethod
    def name(cls):
        return "openfpgaloader"

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={"flash"})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument("--board", required=True, help="openFPGALoader board name")
        parser.add_argument(
            "--external-flash", action="store_true", help="select the external flash"
        )
        parser.add_argument("--verify", action="store_true", help="verify the flash contents")
        parser.add_argument("--offset", help="flash offset")

    @classmethod
    def do_create(cls, cfg, args):
        return OpenFPGALoaderBinaryRunner(
            cfg,
            args.board,
            external_flash=args.external_flash,
            verify=args.verify,
            offset=args.offset,
        )

    def do_run(self, command, **kwargs):
        self.require("openFPGALoader")
        self.ensure_output("bin")

        command = ["openFPGALoader", "--board", self.board]
        if self.external_flash:
            command.append("--external-flash")
        command.append("--write-flash")
        if self.offset:
            command.extend(("--offset", self.offset))
        if self.verify:
            command.append("--verify")
        command.append(self.cfg.bin_file)

        self.check_call(command)
