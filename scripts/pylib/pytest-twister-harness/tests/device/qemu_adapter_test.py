# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import os
from pathlib import Path
from typing import Generator
from unittest.mock import patch

import pytest

from twister_harness.device.qemu_adapter import QemuAdapter
from twister_harness.exceptions import TwisterHarnessException
from twister_harness.twister_harness_config import DeviceConfig


@pytest.fixture(name='device')
def fixture_device_adapter(tmp_path) -> Generator[QemuAdapter, None, None]:
    build_dir = tmp_path / 'build_dir'
    os.mkdir(build_dir)
    device = QemuAdapter(DeviceConfig(build_dir=build_dir, type='qemu', base_timeout=5.0))
    try:
        yield device
    finally:
        device.close()  # to make sure all running processes are closed


@patch('shutil.which', return_value='west')
def test_if_generate_command_creates_proper_command(patched_which, device: QemuAdapter):
    device.device_config.app_build_dir = Path('build_dir')
    device.generate_command()
    assert device.command == ['west', 'build', '-d', 'build_dir', '-t', 'run']


def test_if_qemu_adapter_runs_without_errors(resources, device: QemuAdapter) -> None:
    fifo_file_path = str(device.device_config.build_dir / 'qemu-fifo')
    script_path = resources.joinpath('fifo_mock.py')
    device.command = ['python', str(script_path), fifo_file_path]
    device.launch()
    lines = device.readlines_until(regex='Namespaces are one honking great idea')
    device.close()
    assert 'Readability counts.' in lines
    assert os.path.isfile(device.handler_log_path)
    with open(device.handler_log_path, 'r') as file:
        file_lines = [line.strip() for line in file.readlines()]
    assert file_lines[-2:] == lines[-2:]


def test_if_qemu_adapter_raise_exception_due_to_no_fifo_connection(device: QemuAdapter) -> None:
    device.base_timeout = 0.3
    device.command = ['sleep', '1']
    with pytest.raises(TwisterHarnessException, match='Cannot establish communication with QEMU device.'):
        device._flash_and_run()
    device._close_device()
    assert not os.path.exists(device._fifo_connection._fifo_out_path)
    assert not os.path.exists(device._fifo_connection._fifo_in_path)
