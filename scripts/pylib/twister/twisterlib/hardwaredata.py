# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import logging
import os
import signal
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
    base_params: str | None = None
    runner_params: str | None = None
    hooks: dict[str, dict[str, Any]] = field(default_factory=dict)
    runner: str | None = None
    flash_timeout: int = 60
    flash_with_test: bool = False
    flash_before: bool = False
    fixtures: list[str] = field(default_factory=list)
    run_with_fixture_only: bool = False
    probe_id: str | None = None
    notes: str | None = None
    match: bool = False
    west_flash_cmd: str | None = None

    def to_dict(self) -> dict[str, Any]:
        """Convert DUT dataclass to dictionary for YAML serialization."""
        result = asdict(self)
        # Remove falsy values to keep config file clean
        return {k: v for k, v in result.items() if v}

    def _hook_env(self, hook_type, extra_env=None):
        env = os.environ.copy()
        env['TWISTER_HOOK'] = hook_type
        if self.platform:
            env['TWISTER_PLATFORM'] = str(self.platform)
        if self.id:
            env['TWISTER_DUT_ID'] = str(self.id)
        if self.serial:
            env['TWISTER_DUT_SERIAL'] = str(self.serial)
        if self.runner:
            env['TWISTER_RUNNER'] = str(self.runner)
        if extra_env:
            env.update(extra_env)
        return env

    def run_hook(self, hook_type, extra_env=None):
        """Run hook script. Returns True on success, False on failure, None if no hook."""
        hook = self.hooks.get(hook_type)
        if hook is None:
            return None

        if isinstance(hook, str):
            hook = {'script': hook}

        script = os.path.expandvars(hook.get('script'))
        args = [os.path.expandvars(str(a)) for a in hook.get('args', [])]
        cmd = [script] + args
        timeout = hook.get('timeout', 60)

        logger.info(f"Running command {cmd}....")
        env = self._hook_env(hook_type, extra_env)
        try:
            with subprocess.Popen(
                cmd, stderr=subprocess.PIPE, stdout=subprocess.PIPE, env=env, start_new_session=True
            ) as proc:
                try:
                    stdout, stderr = proc.communicate(timeout=timeout)
                    logger.debug(stdout.decode(errors='ignore'))
                    if proc.returncode != 0:
                        logger.error(f"Custom script failure: {stderr.decode(errors='ignore')}")
                        return False
                except subprocess.TimeoutExpired:
                    os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
                    proc.communicate()
                    logger.error(f"{script} timed out")
                    return False
        except FileNotFoundError:
            logger.error(f"Hook script not found: {script}")
            return False
        except OSError as e:
            logger.error(f"Hook script failed: {e}")
            return False
        except subprocess.SubprocessError as e:
            logger.error(f"Hook script failed: {e}")
            return False
        return True


@dataclass
class CompoundHardwareData(HardwareData):
    """DUT configuration with auxiliary connections from
    the same physical device (same hardware ID). Entries can represent
    other CPU cores or separate logging ports of the same device.
    The build_dir attribute specifies the directory where the build artifacts are located,
    it is optional and used in multi-dut configurations.
    """

    entries: list[HardwareData] = field(default_factory=list)
    build_dir: str | None = None

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
