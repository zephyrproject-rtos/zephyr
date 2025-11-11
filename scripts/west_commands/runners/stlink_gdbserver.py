# Copyright (c) 2025 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0

"""
Runner for debugging applications using the ST-LINK GDB server
from STMicroelectronics, provided as part of the STM32CubeCLT.
"""

import argparse
import platform
import re
import shutil
from pathlib import Path

from runners.core import MissingProgram, RunnerCaps, RunnerConfig, ZephyrBinaryRunner

STLINK_GDB_SERVER_DEFAULT_PORT = 61234


class STLinkGDBServerRunner(ZephyrBinaryRunner):
    @classmethod
    def _get_stm32cubeclt_paths(cls) -> tuple[Path, Path]:
        """
        Returns a tuple of two elements of class pathlib.Path:
            [0]: path to the ST-LINK_gdbserver executable
            [1]: path to the "STM32CubeProgrammer/bin" folder
        """

        def find_highest_clt_version(tools_folder: Path) -> Path | None:
            if not tools_folder.is_dir():
                return None

            # List all CubeCLT installations present in tools folder
            CUBECLT_FLDR_RE = re.compile(r"stm32cubeclt_([1-9]).(\d+).(\d+)", re.IGNORECASE)
            installations: list[tuple[int, Path]] = []
            for f in tools_folder.iterdir():
                m = CUBECLT_FLDR_RE.match(f.name)
                if m is not None:
                    # Compute a number that can be easily compared
                    # from the STM32CubeCLT version number
                    major, minor, revis = int(m[1]), int(m[2]), int(m[3])
                    ver_num = major * 1000000 + minor * 1000 + revis
                    installations.append((ver_num, f))

            if len(installations) == 0:
                return None

            # Sort candidates and return the path to the most recent version
            most_recent_install = sorted(installations, key=lambda e: e[0], reverse=True)[0]
            return most_recent_install[1]

        cur_platform = platform.system()

        # Attempt to find via shutil.which()
        if cur_platform in ["Linux", "Windows"]:
            gdbserv = shutil.which("ST-LINK_gdbserver")
            cubeprg = shutil.which("STM32_Programmer_CLI")
            if gdbserv and cubeprg:
                # Return the parent of cubeprg as [1] should be the path
                # to the folder containing STM32_Programmer_CLI, not the
                # path to the executable itself
                return (Path(gdbserv), Path(cubeprg).parent)

        # Search in OS-specific paths
        search_path: str
        tool_suffix = ""
        if cur_platform == "Linux":
            search_path = "/opt/st/"
        elif cur_platform == "Windows":
            search_path = "C:\\ST\\"
            tool_suffix = ".exe"
        elif cur_platform == "Darwin":
            search_path = "/opt/ST/"
        else:
            raise RuntimeError("Unsupported OS")

        clt = find_highest_clt_version(Path(search_path))
        if clt is None:
            raise MissingProgram("ST-LINK_gdbserver (from STM32CubeCLT)")

        gdbserver_path = clt / "STLink-gdb-server" / "bin" / f"ST-LINK_gdbserver{tool_suffix}"
        cubeprg_bin_path = clt / "STM32CubeProgrammer" / "bin"

        return (gdbserver_path, cubeprg_bin_path)

    @classmethod
    def name(cls) -> str:
        return "stlink_gdbserver"

    @classmethod
    def capabilities(cls) -> RunnerCaps:
        return RunnerCaps(commands={"attach", "debug", "debugserver"}, dev_id=True, extload=True)

    @classmethod
    def extload_help(cls) -> str:
        return "External Loader for ST-Link GDB server"

    @classmethod
    def do_add_parser(cls, parser: argparse.ArgumentParser):
        # Expose a subset of the ST-LINK GDB server arguments
        parser.add_argument(
            "--swd",
            default=True,
            action=argparse.BooleanOptionalAction,
            help="Enable SWD debug mode (default: %(default)s)\nUse --no-swd to disable.",
        )
        parser.add_argument("--apid", type=int, default=0, help="Target DAP ID")
        parser.add_argument(
            "--port-number",
            type=int,
            default=STLINK_GDB_SERVER_DEFAULT_PORT,
            help="Port number for GDB client",
        )
        parser.add_argument(
            "--external-init",
            action='store_true',
            help="Run Init() from external loader after reset",
        )

    @classmethod
    def do_create(cls, cfg: RunnerConfig, args: argparse.Namespace) -> "STLinkGDBServerRunner":
        return STLinkGDBServerRunner(
            cfg,
            args.swd,
            args.apid,
            args.dev_id,
            args.port_number,
            args.extload,
            args.external_init,
        )

    def __init__(
        self,
        cfg: RunnerConfig,
        swd: bool,
        ap_id: int | None,
        stlink_serial: str | None,
        gdb_port: int,
        external_loader: str | None,
        external_init: bool,
    ):
        super().__init__(cfg)
        self.ensure_output('elf')

        self._swd = swd
        self._gdb_port = gdb_port
        self._stlink_serial = stlink_serial
        self._ap_id = ap_id
        self._external_loader = external_loader
        self._do_external_init = external_init

    def do_run(self, command: str, **kwargs):
        if command in ["attach", "debug", "debugserver"]:
            self.do_attach_debug_debugserver(command)
        else:
            raise ValueError(f"{command} not supported")

    def do_attach_debug_debugserver(self, command: str):
        # self.ensure_output('elf') is called in constructor
        # and validated that self.cfg.elf_file is non-null.
        # This assertion is required for the test framework,
        # which doesn't have this insight - it should never
        # trigger in real-world scenarios.
        assert self.cfg.elf_file is not None
        elf_path = Path(self.cfg.elf_file).as_posix()

        gdb_args = ["-ex", f"target remote :{self._gdb_port}", elf_path]

        (gdbserver_path, cubeprg_path) = STLinkGDBServerRunner._get_stm32cubeclt_paths()
        gdbserver_cmd = [gdbserver_path.as_posix()]
        gdbserver_cmd += ["--stm32cubeprogrammer-path", str(cubeprg_path.absolute())]
        gdbserver_cmd += ["--port-number", str(self._gdb_port)]
        gdbserver_cmd += ["--apid", str(self._ap_id)]
        gdbserver_cmd += ["--halt"]

        if self._swd:
            gdbserver_cmd.append("--swd")

        if command == "attach":
            gdbserver_cmd += ["--attach"]
        else:  # debug/debugserver
            gdbserver_cmd += ["--initialize-reset"]
            gdb_args += ["-ex", f"load {elf_path}"]

        if self._stlink_serial:
            gdbserver_cmd += ["--serial-number", self._stlink_serial]

        if self._external_loader:
            extldr_path = cubeprg_path / "ExternalLoader" / self._external_loader
            if not extldr_path.exists():
                raise RuntimeError(f"External loader {self._external_loader} does not exist")

            if self._do_external_init:
                gdbserver_cmd += ["--external-init"]
            gdbserver_cmd += ["--extload", str(extldr_path)]

        self.require(gdbserver_cmd[0])

        if command == "debugserver":
            self.check_call(gdbserver_cmd)
        elif self.cfg.gdb is None:  # attach/debug
            raise RuntimeError("GDB is required for attach/debug")
        else:  # attach/debug
            gdb_cmd = [self.cfg.gdb] + gdb_args
            self.require(gdb_cmd[0])
            self.run_server_and_client(gdbserver_cmd, gdb_cmd)
