# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import logging
from dataclasses import dataclass, field
from pathlib import Path
from twister_harness.helpers.domains_helper import get_default_domain_name

import pytest

logger = logging.getLogger(__name__)


@dataclass
class DeviceConfig:
    type: str
    build_dir: Path
    base_timeout: float = 60.0  # [s]
    flash_timeout: float = 60.0  # [s]
    platform: str = ''
    serial: str = ''
    baud: int = 115200
    runner: str = ''
    runner_params: list[str] = field(default_factory=list, repr=False)
    id: str = ''
    product: str = ''
    serial_pty: str = ''
    flash_before: bool = False
    west_flash_extra_args: list[str] = field(default_factory=list, repr=False)
    name: str = ''
    pre_script: Path | None = None
    post_script: Path | None = None
    post_flash_script: Path | None = None
    fixtures: list[str] = None
    app_build_dir: Path | None = None

    def __post_init__(self):
        domains = self.build_dir / 'domains.yaml'
        if domains.exists():
            self.app_build_dir = self.build_dir / get_default_domain_name(domains)
        else:
            self.app_build_dir = self.build_dir


@dataclass
class TwisterHarnessConfig:
    """Store Twister harness configuration to have easy access in test."""
    devices: list[DeviceConfig] = field(default_factory=list, repr=False)

    @classmethod
    def create(cls, config: pytest.Config) -> TwisterHarnessConfig:
        """Create new instance from pytest.Config."""

        devices = []

        west_flash_extra_args: list[str] = []
        if config.option.west_flash_extra_args:
            west_flash_extra_args = [w.strip() for w in config.option.west_flash_extra_args.split(',')]
        runner_params: list[str] = []
        if config.option.runner_params:
            runner_params = [w.strip() for w in config.option.runner_params]
        device_from_cli = DeviceConfig(
            type=config.option.device_type,
            build_dir=_cast_to_path(config.option.build_dir),
            base_timeout=config.option.base_timeout,
            flash_timeout=config.option.flash_timeout,
            platform=config.option.platform,
            serial=config.option.device_serial,
            baud=config.option.device_serial_baud,
            runner=config.option.runner,
            runner_params=runner_params,
            id=config.option.device_id,
            product=config.option.device_product,
            serial_pty=config.option.device_serial_pty,
            flash_before=bool(config.option.flash_before),
            west_flash_extra_args=west_flash_extra_args,
            pre_script=_cast_to_path(config.option.pre_script),
            post_script=_cast_to_path(config.option.post_script),
            post_flash_script=_cast_to_path(config.option.post_flash_script),
            fixtures=config.option.fixtures,
        )

        devices.append(device_from_cli)

        return cls(
            devices=devices
        )


def _cast_to_path(path: str | None) -> Path | None:
    if path is None:
        return None
    return Path(path)
