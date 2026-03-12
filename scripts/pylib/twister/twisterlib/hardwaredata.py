# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from dataclasses import asdict, dataclass, field
from typing import Any


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
    pre_script: str | None = None
    post_script: str | None = None
    post_flash_script: str | None = None
    script_param: str | None = None
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
