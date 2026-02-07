# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import logging
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Any

import yaml
from twisterlib.hardwaredata import HardwareData

TWISTER_PYTEST_CONFIG_FILE = 'twister_pytest_config.yaml'
logger = logging.getLogger('twister')


@dataclass
class HardwareDataWithCores(HardwareData):
    """Device Under Test configuration."""

    cores: list[HardwareData] = field(default_factory=list)

    def to_dict(self) -> dict[str, Any]:
        """Convert DUT dataclass to dictionary for YAML serialization."""
        result = asdict(self)
        result['cores'] = [core.to_dict() for core in self.cores]
        # Remove falsy values to keep config file clean
        return {k: v for k, v in result.items() if v}

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> HardwareDataWithCores:
        """Create HardwareDataWithCores from dictionary loaded from YAML."""
        if 'cores' in data:
            data['cores'] = [HardwareData(**core) for core in data['cores']]
        return cls(**data)


@dataclass
class HarnessPytestConfig:
    """Test parameters to serialize for pytest-harness."""

    platform: str
    device_type: str = ''
    base_timeout: float = 60.0
    extra_test_args: str = ''
    west_runner: str = ''
    west_flash_extra_args: str = ''
    flash_command: str = ''
    flash_before: bool = False
    twister_fixtures: list[str] = field(default_factory=list)
    required_builds: list[str] = field(default_factory=list)
    duts: list[HardwareDataWithCores] = field(default_factory=list)

    def to_dict(self) -> dict[str, Any]:
        """Convert dataclass to dictionary for YAML serialization."""
        result = asdict(self)
        result['duts'] = [dut.to_dict() for dut in self.duts]
        # Remove falsy values to keep config file clean
        return {k: v for k, v in result.items() if v}

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> HarnessPytestConfig:
        """Create TestParameters from dictionary loaded from YAML."""
        # Handle nested DUT objects
        if 'duts' in data:
            data['duts'] = [HardwareDataWithCores.from_dict(dut_data) for dut_data in data['duts']]
        return cls(**data)

    def save_to_yaml(self, filepath: Path) -> None:
        """Save configuration to YAML file."""
        with open(filepath, 'w') as f:
            yaml.dump(self.to_dict(), f, default_flow_style=False)
        logger.debug(f"Configuration saved to {filepath}")

    @classmethod
    def load_from_yaml(cls, filepath: Path) -> HarnessPytestConfig:
        """Load parameters from YAML file."""
        with open(filepath) as f:
            data = yaml.safe_load(f)
        logger.debug(f"Configuration loaded from {filepath}")
        return cls.from_dict(data)
