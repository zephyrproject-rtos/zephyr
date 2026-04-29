# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import logging
import os
import subprocess
from dataclasses import asdict, dataclass, field
from typing import Any

logger = logging.getLogger('twister')


@dataclass
class HardwareData:
    """Device Under Test configuration."""

    id: str | None = None
    serial: str | None = None
    serial_baud: int = 115200
    platform: str | None = None
    product: str | None = None
    serial_pty: str | None = None
    connected: bool = False
    runner_params: str | None = None
    hooks: list[dict[str, Any]] = field(default_factory=list)
    runner: str | None = None
    flash_timeout: int = 60
    flash_with_test: bool = False
    flash_before: bool = False
    fixtures: list[str] = field(default_factory=list)
    probe_id: str | None = None
    notes: str | None = None
    match: bool = False
    west_flash_cmd: str | None = None

    def to_dict(self) -> dict[str, Any]:
        """Convert DUT dataclass to dictionary for YAML serialization."""
        result = asdict(self)
        # Remove falsy values to keep config file clean
        return {k: v for k, v in result.items() if v}

    def run_hook(self, hook_type):
        hooks = (h for h in self.hooks if h.get("type") == hook_type)
        if (hook := next(hooks, None)) is None:
            return

        script = hook.get('script')
        args = hook.get('args', [])
        cmd = [script] + args
        timeout = hook.get('timeout', 60)

        logger.info(f"Running command {cmd}....")
        env = os.environ.copy()
        try:
            with subprocess.Popen(
                cmd, stderr=subprocess.PIPE, stdout=subprocess.PIPE, env=env
            ) as proc:
                try:
                    stdout, stderr = proc.communicate(timeout=timeout)
                    logger.debug(stdout.decode())
                    if proc.returncode != 0:
                        logger.error(f"Custom script failure: {stderr.decode(errors='ignore')}")
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.communicate()
                    logger.error(f"{script} timed out")
        except FileNotFoundError:
            logger.error(f"Hook script not found: {script}")
        except subprocess.SubprocessError as e:
            logger.error(f"Hook script failed: {e}")


@dataclass
class CompoundHardwareData(HardwareData):
    """DUT configuration with auxiliary connections from
    the same physical device (same hardware ID). Entries can represent
    other CPU cores or separate logging ports of the same device.
    """

    entries: list[HardwareData] = field(default_factory=list)

    def to_dict(self) -> dict[str, Any]:
        """Convert DUT dataclass to dictionary for YAML serialization."""
        result = asdict(self)
        result['entries'] = [entry.to_dict() for entry in self.entries]
        # Remove falsy values to keep config file clean
        return {k: v for k, v in result.items() if v}

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> CompoundHardwareData:
        """Create CompoundHardwareData from dictionary loaded from YAML."""
        if 'entries' in data:
            data['entries'] = [HardwareData(**entry) for entry in data['entries']]
        return cls(**data)
