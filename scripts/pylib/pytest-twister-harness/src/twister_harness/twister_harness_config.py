# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import logging
from dataclasses import dataclass, field
from pathlib import Path

import pytest

logger = logging.getLogger(__name__)


@dataclass
class DeviceConfig:
    platform: str = ''
    type: str = ''
    serial: str = ''
    baud: int = 115200
    runner: str = ''
    id: str = ''
    product: str = ''
    serial_pty: str = ''
    west_flash_extra_args: list[str] = field(default_factory=list, repr=False)
    flashing_timeout: int = 60  # [s]
    build_dir: Path | str = ''
    binary_file: Path | str = ''
    name: str = ''
    pre_script: str = ''
    post_script: str = ''
    post_flash_script: str = ''


@dataclass
class TwisterHarnessConfig:
    """Store Twister harness configuration to have easy access in test."""
    output_dir: Path = Path('twister_harness_out')
    devices: list[DeviceConfig] = field(default_factory=list, repr=False)

    @classmethod
    def create(cls, config: pytest.Config) -> TwisterHarnessConfig:
        """Create new instance from pytest.Config."""
        output_dir: Path = config.option.output_dir

        devices = []

        west_flash_extra_args: list[str] = []
        if config.option.west_flash_extra_args:
            west_flash_extra_args = [w.strip() for w in config.option.west_flash_extra_args.split(',')]
        device_from_cli = DeviceConfig(
            platform=config.option.platform,
            type=config.option.device_type,
            serial=config.option.device_serial,
            baud=config.option.device_serial_baud,
            runner=config.option.runner,
            id=config.option.device_id,
            product=config.option.device_product,
            serial_pty=config.option.device_serial_pty,
            west_flash_extra_args=west_flash_extra_args,
            flashing_timeout=config.option.flashing_timeout,
            build_dir=config.option.build_dir,
            binary_file=config.option.binary_file,
            pre_script=config.option.pre_script,
            post_script=config.option.post_script,
            post_flash_script=config.option.post_flash_script,
        )

        devices.append(device_from_cli)

        return cls(
            output_dir=output_dir,
            devices=devices
        )
