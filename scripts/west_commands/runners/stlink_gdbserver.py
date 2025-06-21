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

from runners.core import RunnerCaps, RunnerConfig, ZephyrBinaryRunner

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
            CUBECLT_FLDR_RE = re.compile(r"stm32cubeclt_([1-9]).(\d+).(\d+)", re.IGNORECASE)

            def path2version(path: Path):
                m = CUBECLT_FLDR_RE.match(path.name)
                if m is None:
                    # We should never be calling this function for
                    # a path that did not match() the regexp, so
                    # this should never happen...
                    raise RuntimeError("Invalid candidate path?!")

                major = int(m[1])
                minor = int(m[2])
                revis = int(m[3])
                return major * 1000000 + minor * 1000 + revis

            # List all possible CubeCLT installations in tools folder
            candidates = [f for f in tools_folder.iterdir() if CUBECLT_FLDR_RE.match(f.name)]

            if len(candidates) == 0:
                return None

            # Sort candidates and return the most recent version
            return sorted(candidates, key=path2version, reverse=True)[0]

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
            raise NotImplementedError("Unsupported OS")

        clt = find_highest_clt_version(Path(search_path))
        if clt is None:
            raise RuntimeError("Could not locate STM32CubeCLT installation!")

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
            "--swd", action='store_true', default=True, help="Enable SWD debug mode"
        )
        parser.add_argument("--apid", type=int, default=0, help="Target DAP ID")
        parser.add_argument(
            "--port-number",
            type=int,
            default=STLINK_GDB_SERVER_DEFAULT_PORT,
            help="Port number for GDB client",
        )

    @classmethod
    def do_create(cls, cfg: RunnerConfig, args: argparse.Namespace) -> "STLinkGDBServerRunner":
        return STLinkGDBServerRunner(
            cfg, args.swd, args.apid, args.dev_id, args.port_number, args.extload
        )

    def __init__(
        self,
        cfg: RunnerConfig,
        swd: bool,
        ap_id: int | None,
        stlink_serial: str | None,
        gdb_port: int,
        external_loader: str | None,
    ):
        super().__init__(cfg)
        self.elf_name = Path(cfg.elf_file).as_posix() if cfg.elf_file else None
        self.gdb_cmd = [cfg.gdb] if cfg.gdb else None

        self._swd = swd
        self._gdb_port = gdb_port
        self._stlink_serial = stlink_serial
        self._ap_id = ap_id
        self._external_loader = external_loader

    def do_run(self, command: str, **kwargs):
        if command in ["attach", "debug", "debugserver"]:
            self.do_attach_debug_debugserver(command)
        else:
            raise NotImplementedError(f"{command} not supported")

    def do_attach_debug_debugserver(self, command: str):
        if self.elf_name is None:
            raise RuntimeError("ELF is required for this runner")

        gdb_args = ["-ex", f"target remote :{self._gdb_port}", self.elf_name]

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
            gdb_args += ["-ex", f"load {self.elf_name}"]

        if self._stlink_serial:
            gdbserver_cmd += ["--serial-number", self._stlink_serial]

        if self._external_loader:
            extldr_path = cubeprg_path / "ExternalLoader" / self._external_loader
            if not extldr_path.exists():
                raise RuntimeError(f"External loader {self._external_loader} does not exist")
            gdbserver_cmd += ["--extload", str(extldr_path)]

        self.require(gdbserver_cmd[0])

        if command == "debugserver":
            self.check_call(gdbserver_cmd)
        elif self.gdb_cmd is None:  # attach/debug
            raise RuntimeError("GDB is required for attach/debug")
        else:  # attach/debug
            gdb_cmd = self.gdb_cmd + gdb_args
            self.require(gdb_cmd[0])
            self.run_server_and_client(gdbserver_cmd, gdb_cmd)
