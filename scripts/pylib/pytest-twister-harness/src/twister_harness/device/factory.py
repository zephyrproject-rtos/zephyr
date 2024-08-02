# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import logging
from typing import Type

from twister_harness.device.device_adapter import DeviceAdapter
from twister_harness.device.hardware_adapter import HardwareAdapter
from twister_harness.device.qemu_adapter import QemuAdapter
from twister_harness.device.binary_adapter import (
    CustomSimulatorAdapter,
    NativeSimulatorAdapter,
    UnitSimulatorAdapter,
)
from twister_harness.exceptions import TwisterHarnessException

logger = logging.getLogger(__name__)


class DeviceFactory:
    _devices: dict[str, Type[DeviceAdapter]] = {}

    @classmethod
    def discover(cls):
        """Return available devices."""

    @classmethod
    def register_device_class(cls, name: str, klass: Type[DeviceAdapter]):
        if name not in cls._devices:
            cls._devices[name] = klass

    @classmethod
    def get_device(cls, name: str) -> Type[DeviceAdapter]:
        logger.debug('Get device type "%s"', name)
        try:
            return cls._devices[name]
        except KeyError as exc:
            logger.error('There is no device with name "%s"', name)
            raise TwisterHarnessException(f'There is no device with name "{name}"') from exc


DeviceFactory.register_device_class('custom', CustomSimulatorAdapter)
DeviceFactory.register_device_class('native', NativeSimulatorAdapter)
DeviceFactory.register_device_class('unit', UnitSimulatorAdapter)
DeviceFactory.register_device_class('hardware', HardwareAdapter)
DeviceFactory.register_device_class('qemu', QemuAdapter)
