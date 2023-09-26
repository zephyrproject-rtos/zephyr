# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import os
from pathlib import Path
from typing import Generator

import pytest

from twister_harness.device.binary_adapter import NativeSimulatorAdapter
from twister_harness.twister_harness_config import DeviceConfig


@pytest.fixture
def resources() -> Path:
    """Return path to `resources` folder"""
    return Path(__file__).parent.joinpath('resources')


@pytest.fixture
def zephyr_base() -> str:
    zephyr_base_path = os.getenv('ZEPHYR_BASE')
    if zephyr_base_path is None:
        pytest.fail('Environmental variable ZEPHYR_BASE has to be set.')
    else:
        return zephyr_base_path


@pytest.fixture
def twister_harness(zephyr_base) -> str:
    """Retrun path to pytest-twister-harness src directory"""
    pytest_twister_harness_path = str(Path(zephyr_base) / 'scripts' / 'pylib' / 'pytest-twister-harness' / 'src')
    return pytest_twister_harness_path


@pytest.fixture
def shell_simulator_path(resources: Path) -> str:
    return str(resources / 'shell_simulator.py')


@pytest.fixture
def shell_simulator_adapter(
    tmp_path: Path, shell_simulator_path: str
) -> Generator[NativeSimulatorAdapter, None, None]:
    build_dir = tmp_path / 'build_dir'
    os.mkdir(build_dir)
    device = NativeSimulatorAdapter(DeviceConfig(build_dir=build_dir, type='native', base_timeout=5.0))
    try:
        device.command = ['python3', shell_simulator_path]
        device.launch()
        yield device
    finally:
        device.write(b'quit\n')
        device.close()
