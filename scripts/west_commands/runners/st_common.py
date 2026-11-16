# Copyright (c) 2026 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0

"""Common code shared by the STMicroelectronics runners
(stlink_gdbserver, stm32cubeprogrammer)
"""

import json
import shutil
import subprocess
from logging import Logger
from pathlib import Path


class STM32CubeCLI:
    @staticmethod
    def getTool(logger: Logger) -> "STM32CubeCLI|None":
        exe_loc = shutil.which("cube")

        return STM32CubeCLI(exe_loc, logger) if exe_loc else None

    def __init__(self, exe: str, logger: Logger) -> None:
        assert exe is not None, "Valid executable path is required"
        self._exe = Path(exe)
        self.logger = logger

    def _call_exe(self, *args) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [self._exe, "--json", *args],
            capture_output=True,
            text=True,
            encoding="utf-8",
        )

    def search_tool(self, tool_name: str) -> Path | None:
        self.logger.debug(f"Querying STM32CubeCLI in search of tool '{tool_name}'")
        res = self._call_exe("--resolve", tool_name)

        # If resolution fails, the tool will return a non-zero exit code.
        # This happens if the requested tool is not installed, so log the
        # event at a low severity. Processing must be stopped immediately
        # because stdout/stderr will contain non-JSON messages; parsing
        # below would fail catastrophically if we continued.
        if res.returncode != 0:
            self.logger.info(f"STM32CubeCLI resolve failed (exit code {res.returncode})")
            return None

        data = json.loads(res.stdout.strip())

        # STM32CubeCLI is still in beta, so the JSON schema might change.
        # Process the tool's output in a safe manner and merely log an error if parsing
        # fails instead of letting the exception propagate. This ensures the presence of
        # an unsupported version of STM32CubeCLI (which uses a schema we don't recognize)
        # doesn't prevent usage of the ST runners entirely.
        try:
            command: list = data["command"]
            return Path(command[0])
        except (KeyError, IndexError, TypeError):
            self.logger.error(
                f"Failed to process STM32CubeCLI resolve output while searching for '{tool_name}'",
                exc_info=True,
            )
            self.logger.debug("(Has the STM32CubeCLI schema changed?)")
            self.logger.debug(f"Raw output JSON:\n{data}")
            return None
