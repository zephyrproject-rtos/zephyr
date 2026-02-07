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

    def to_dict(self) -> dict[str, Any]:
        """Convert DUT dataclass to dictionary for YAML serialization."""
        result = asdict(self)
        # Remove falsy values to keep config file clean
        return {k: v for k, v in result.items() if v}
