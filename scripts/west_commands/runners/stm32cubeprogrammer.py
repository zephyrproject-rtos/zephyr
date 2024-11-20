# Copyright (c) 2020 Teslabs Engineering S.L.
#
# SPDX-License-Identifier: Apache-2.0

"""Runner for flashing with STM32CubeProgrammer CLI, the official programming
   utility from ST Microelectronics.
"""

import argparse
from pathlib import Path
import platform
import os
import shlex
import shutil
from typing import List, Optional, ClassVar, Dict

from runners.core import ZephyrBinaryRunner, RunnerCaps, RunnerConfig


class STM32CubeProgrammerBinaryRunner(ZephyrBinaryRunner):
    """Runner front-end for STM32CubeProgrammer CLI."""

    _RESET_MODES: ClassVar[dict[str, str]] = {
        "sw": "SWrst",
        "hw": "HWrst",
        "core": "Crst",
    }
    """Reset mode argument mappings."""

    def __init__(
        self,
        cfg: RunnerConfig,
        port: str,
        frequency: int | None,
        reset_mode: str | None,
        conn_modifiers: str | None,
        cli: Path | None,
        use_elf: bool,
        erase: bool,
        extload: str | None,
        tool_opt: list[str],
    ) -> None:
        super().__init__(cfg)

        self._port = port
        self._frequency = frequency
        self._reset_mode = reset_mode
        self._conn_modifiers = conn_modifiers
        self._cli = (
            cli or STM32CubeProgrammerBinaryRunner._get_stm32cubeprogrammer_path()
        )
        self._use_elf = use_elf
        self._erase = erase

        if extload:
            p = STM32CubeProgrammerBinaryRunner._get_stm32cubeprogrammer_path().parent.resolve() / 'ExternalLoader'
            self._extload = ['-el', str(p / extload)]
        else:
            self._extload = []

        self._tool_opt: list[str] = list()
        for opts in [shlex.split(opt) for opt in tool_opt]:
            self._tool_opt += opts

        # add required library loader path to the environment (Linux only)
        if platform.system() == "Linux":
            os.environ["LD_LIBRARY_PATH"] = str(self._cli.parent / ".." / "lib")

    @staticmethod
    def _get_stm32cubeprogrammer_path() -> Path:
        """Obtain path of the STM32CubeProgrammer CLI tool."""

        if platform.system() == "Linux":
            cmd = shutil.which("STM32_Programmer_CLI")
            if cmd is not None:
                return Path(cmd)

            return (
                Path.home()
                / "STMicroelectronics"
                / "STM32Cube"
                / "STM32CubeProgrammer"
                / "bin"
                / "STM32_Programmer_CLI"
            )

        if platform.system() == "Windows":
            cmd = shutil.which("STM32_Programmer_CLI")
            if cmd is not None:
                return Path(cmd)

            cli = (
                Path("STMicroelectronics")
                / "STM32Cube"
                / "STM32CubeProgrammer"
                / "bin"
                / "STM32_Programmer_CLI.exe"
            )
            x86_path = Path(os.environ["PROGRAMFILES(X86)"]) / cli
            if x86_path.exists():
                return x86_path

            return Path(os.environ["PROGRAMW6432"]) / cli

        if platform.system() == "Darwin":
            return (
                Path("/Applications")
                / "STMicroelectronics"
                / "STM32Cube"
                / "STM32CubeProgrammer"
                / "STM32CubeProgrammer.app"
                / "Contents"
                / "MacOs"
                / "bin"
                / "STM32_Programmer_CLI"
            )

        raise NotImplementedError("Could not determine STM32_Programmer_CLI path")

    @classmethod
    def name(cls):
        return "stm32cubeprogrammer"

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={"flash"}, erase=True, extload=True, tool_opt=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument(
            "--port",
            type=str,
            required=True,
            help="Interface identifier, e.g. swd, jtag, /dev/ttyS0...",
        )
        parser.add_argument(
            "--frequency", type=int, required=False, help="Programmer frequency in KHz"
        )
        parser.add_argument(
            "--reset-mode",
            type=str,
            required=False,
            choices=["sw", "hw", "core"],
            help="Reset mode",
        )
        parser.add_argument(
            "--conn-modifiers",
            type=str,
            required=False,
            help="Additional options for the --connect argument",
        )
        parser.add_argument(
            "--cli",
            type=Path,
            required=False,
            help="STM32CubeProgrammer CLI tool path",
        )
        parser.add_argument(
            "--use-elf",
            action="store_true",
            required=False,
            help="Use ELF file when flashing instead of HEX file",
        )

    @classmethod
    def extload_help(cls) -> str:
        return "External Loader for STM32_Programmer_CLI"

    @classmethod
    def tool_opt_help(cls) -> str:
        return "Additional options for STM32_Programmer_CLI"

    @classmethod
    def do_create(
        cls, cfg: RunnerConfig, args: argparse.Namespace
    ) -> "STM32CubeProgrammerBinaryRunner":
        return STM32CubeProgrammerBinaryRunner(
            cfg,
            port=args.port,
            frequency=args.frequency,
            reset_mode=args.reset_mode,
            conn_modifiers=args.conn_modifiers,
            cli=args.cli,
            use_elf=args.use_elf,
            erase=args.erase,
            extload=args.extload,
            tool_opt=args.tool_opt,
        )

    def do_run(self, command: str, **kwargs):
        if command == "flash":
            self.flash(**kwargs)

    def flash(self, **kwargs) -> None:
        self.require(str(self._cli))

        # prepare base command
        cmd = [str(self._cli)]

        connect_opts = f"port={self._port}"
        if self._frequency:
            connect_opts += f" freq={self._frequency}"
        if self._reset_mode:
            reset_mode = STM32CubeProgrammerBinaryRunner._RESET_MODES[self._reset_mode]
            connect_opts += f" reset={reset_mode}"
        if self._conn_modifiers:
            connect_opts += f" {self._conn_modifiers}"

        cmd += ["--connect", connect_opts]
        cmd += self._tool_opt
        if self._extload:
            # external loader to come after the tool option in STM32CubeProgrammer
            cmd += self._extload

        # erase first if requested
        if self._erase:
            self.check_call(cmd + ["--erase", "all"])

        # flash image and run application
        dl_file = self.cfg.elf_file if self._use_elf else self.cfg.hex_file
        if dl_file is None:
            raise RuntimeError('cannot flash; no download file was specified')
        elif not os.path.isfile(dl_file):
            raise RuntimeError(f'download file {dl_file} does not exist')
        self.check_call(cmd + ["--download", dl_file, "--start"])
