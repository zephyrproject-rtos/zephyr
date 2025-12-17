# Copyright (c) 2026 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
'''
Common code shared by the STMicroelectronics runners
(stlink_gdbserver, stm32cubeprogrammer)
'''

import json
import shutil
import subprocess
from pathlib import Path


class STM32CubeCLI:
    def __init__(self) -> None:
        self._exe = shutil.which("cube")

    def available(self) -> bool:
        return self._exe is not None

    def _call_exe(self, *args) -> subprocess.CompletedProcess[str]:
        if not self._exe:
            raise FileNotFoundError("STM32CubeCLI is not available")

        return subprocess.run(
            [self._exe, "--json", *args],
            capture_output=True,
            text=True,
            encoding="utf-8",
        )

    def search_tool(self, tool_name: str) -> Path | None:
        res = self._call_exe("--resolve", tool_name)
        if res.returncode != 0:
            return None

        try:
            data = json.loads(res.stdout.strip())
        except json.JSONDecodeError:
            return None

        if (command := data.get("command")) is None:
            return None

        if not isinstance(command, list) or len(command) == 0:
            return None

        return Path(command[0])
