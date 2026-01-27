# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import csv
import logging
import pytest
from dataclasses import dataclass, field
from pathlib import Path
from twister_harness.exceptions import TwisterHarnessException
from twister_harness.helpers.domains_helper import get_default_domain_name
from twisterlib.hardwaredata import HardwareData
from twisterlib.harnessconfig import (
    HardwareDataWithCores,
    HarnessPytestConfig,
    TWISTER_PYTEST_CONFIG_FILE,
)

logger = logging.getLogger(__name__)


@dataclass
class DeviceSerialConfig:
    port: str
    baud: int = 115200
    serial_pty: str = ''


@dataclass
class DeviceConfig:
    type: str
    build_dir: Path
    base_timeout: float = 60.0  # [s]
    flash_timeout: float = 60.0  # [s]
    platform: str = ''
    serial_configs: list[DeviceSerialConfig] = field(default_factory=list)
    runner: str = ''
    runner_params: list[str] = field(default_factory=list, repr=False)
    id: str = ''
    product: str = ''
    flash_before: bool = False
    west_flash_extra_args: list[str] = field(default_factory=list, repr=False)
    flash_command: str = ''
    name: str = ''
    pre_script: Path | None = None
    post_script: Path | None = None
    post_flash_script: Path | None = None
    fixtures: list[str] = None
    app_build_dir: Path | None = None
    extra_test_args: str = ''

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
    test_params: HarnessPytestConfig | None = None

    @classmethod
    def create(cls, config: pytest.Config) -> TwisterHarnessConfig:
        """Create new instance from pytest.Config."""
        if test_params_file := get_test_param_file(config):
            test_params = HarnessPytestConfig.load_from_yaml(test_params_file)
        else:
            test_params = HarnessPytestConfig(platform='undefined')

        build_dir = _cast_to_path(config.option.build_dir)
        if not build_dir and test_params_file:
            build_dir = test_params_file.parent
        if not build_dir:
            raise TwisterHarnessException('--build-dir or --test-config has to be provided')

        device_type = config.option.device_type or test_params.device_type
        if not device_type:
            raise TwisterHarnessException('--device-type has to be provided')

        devices = []

        west_flash_extra_args: list[str] = []
        if config.option.west_flash_extra_args:
            west_flash_extra_args = [
                w.strip() for w in next(csv.reader([config.option.west_flash_extra_args]))
            ]
        elif test_params.west_flash_extra_args:
            west_flash_extra_args = [
                w.strip() for w in next(csv.reader([test_params.west_flash_extra_args]))
            ]

        flash_command: list[str] = []
        if config.option.flash_command:
            flash_command = [w.strip() for w in next(csv.reader([config.option.flash_command]))]
        elif test_params.flash_command:
            flash_command = [w.strip() for w in next(csv.reader([test_params.flash_command]))]

        runner_params: list[str] = []
        if config.option.runner_params:
            runner_params = [w.strip() for w in config.option.runner_params]

        # for native, qemu and backwards compatibility with DUT defined from command line
        if not test_params.duts:
            dut = HardwareDataWithCores()
            dut.serial = config.option.device_serial[0] if config.option.device_serial else None
            dut.serial_baud = config.option.device_serial_baud
            dut.serial_pty = config.option.device_serial_pty
            if config.option.device_serial and len(config.option.device_serial) > 1:
                for core_serial in config.option.device_serial[1:]:
                    core_dut = HardwareData(
                        serial=core_serial,
                        serial_baud=config.option.device_serial_baud,
                        serial_pty=config.option.device_serial_pty,
                    )
                    dut.cores.append(core_dut)
            test_params.duts.append(dut)

        for dut in test_params.duts:
            serial_configs: list[DeviceSerialConfig] = []
            for _dut in [dut] + dut.cores:
                serial_configs.append(
                    DeviceSerialConfig(
                        port=_dut.serial, baud=_dut.serial_baud, serial_pty=_dut.serial_pty
                    )
                )

            device = DeviceConfig(
                type=device_type,
                build_dir=build_dir,
                base_timeout=config.option.base_timeout or test_params.base_timeout,
                flash_timeout=config.option.flash_timeout or dut.flash_timeout,
                platform=dut.platform or config.option.platform or test_params.platform,
                serial_configs=serial_configs,
                runner=config.option.runner or dut.runner,
                runner_params=dut.runner_params or runner_params,  # order should be changed to prefer CLI
                id=config.option.device_id or dut.id,
                product=config.option.device_product or dut.product,
                flash_before=config.option.flash_before or dut.flash_before or test_params.flash_before,
                west_flash_extra_args=west_flash_extra_args,
                flash_command=flash_command,
                pre_script=_cast_to_path(config.option.pre_script) or _cast_to_path(dut.pre_script),
                post_script=_cast_to_path(config.option.post_script) or _cast_to_path(dut.post_script),
                post_flash_script=_cast_to_path(config.option.post_flash_script) or _cast_to_path(dut.post_flash_script),
                fixtures=config.option.fixtures or dut.fixtures or test_params.twister_fixtures,
                extra_test_args=config.option.extra_test_args,
            )
            devices.append(device)

        return cls(devices=devices, test_params=test_params)


def get_test_param_file(config: pytest.Config) -> Path | None:
    if test_params_file := _cast_to_path(config.option.twister_config):
        return test_params_file
    if build_dir := _cast_to_path(config.option.build_dir):
        test_params_file = build_dir / TWISTER_PYTEST_CONFIG_FILE
        if test_params_file.exists():
            return test_params_file
    return None


def _cast_to_path(path: str | None) -> Path | None:
    if path is None:
        return None
    return Path(path)
