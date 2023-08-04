# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import os
import subprocess
from pathlib import Path
from unittest import mock

import pytest

from conftest import readlines_until
from twister_harness.device.binary_adapter import (
    CustomSimulatorAdapter,
    NativeSimulatorAdapter,
    UnitSimulatorAdapter,
)
from twister_harness.exceptions import TwisterHarnessException, TwisterHarnessTimeoutException
from twister_harness.twister_harness_config import DeviceConfig


@pytest.fixture(name='device')
def fixture_adapter(tmp_path) -> NativeSimulatorAdapter:
    return NativeSimulatorAdapter(DeviceConfig(build_dir=tmp_path))


def test_if_simulator_adapter_runs_without_errors(
        resources: Path, device: NativeSimulatorAdapter
) -> None:
    """
    Run script which prints text line by line and ends without errors.
    Verify if subprocess was ended without errors, and without timeout.
    """
    script_path = resources.joinpath('mock_script.py')
    # patching original command by mock_script.py to simulate same behaviour as zephyr.exe
    device.command = ['python3', str(script_path)]
    device.launch()
    lines = readlines_until(device=device, line_pattern='Returns with code')
    device.close()
    assert 'Readability counts.' in lines
    assert os.path.isfile(device.handler_log_path)
    with open(device.handler_log_path, 'r') as file:
        file_lines = [line.strip() for line in file.readlines()]
    assert file_lines[-2:] == lines[-2:]


def test_if_simulator_adapter_finishes_after_timeout_while_there_is_no_data_from_subprocess(
        resources: Path, device: NativeSimulatorAdapter
) -> None:
    """Test if thread finishes after timeout when there is no data on stdout, but subprocess is still running"""
    script_path = resources.joinpath('mock_script.py')
    device.base_timeout = 1.0
    device.command = ['python3', str(script_path), '--long-sleep', '--sleep=5']
    device.launch()
    with pytest.raises(TwisterHarnessTimeoutException, match='Read from device timeout occurred'):
        readlines_until(device=device, line_pattern='Returns with code')
    device.close()
    assert device._process is None
    with open(device.handler_log_path, 'r') as file:
        file_lines = [line.strip() for line in file.readlines()]
    # this message should not be printed because script has been terminated due to timeout
    assert 'End of script' not in file_lines, 'Script has not been terminated before end'


def test_if_simulator_adapter_raises_exception_empty_command(device: NativeSimulatorAdapter) -> None:
    device.command = []
    exception_msg = 'Run command is empty, please verify if it was generated properly.'
    with pytest.raises(TwisterHarnessException, match=exception_msg):
        device._flash_and_run()


@mock.patch('subprocess.Popen', side_effect=subprocess.SubprocessError(1, 'Exception message'))
def test_if_simulator_adapter_raises_exception_when_subprocess_raised_subprocess_error(
    patched_popen, device: NativeSimulatorAdapter
) -> None:
    device.command = ['echo', 'TEST']
    with pytest.raises(TwisterHarnessException, match='Exception message'):
        device._flash_and_run()


@mock.patch('subprocess.Popen', side_effect=FileNotFoundError(1, 'File not found', 'fake_file.txt'))
def test_if_simulator_adapter_raises_exception_file_not_found(
    patched_popen, device: NativeSimulatorAdapter
) -> None:
    device.command = ['echo', 'TEST']
    with pytest.raises(TwisterHarnessException, match='fake_file.txt'):
        device._flash_and_run()


@mock.patch('subprocess.Popen', side_effect=Exception(1, 'Raised other exception'))
def test_if_simulator_adapter_raises_exception_when_subprocess_raised_an_error(
    patched_run, device: NativeSimulatorAdapter
) -> None:
    device.command = ['echo', 'TEST']
    with pytest.raises(TwisterHarnessException, match='Raised other exception'):
        device._flash_and_run()


def test_if_native_simulator_adapter_get_command_returns_proper_string(
        device: NativeSimulatorAdapter, resources: Path
) -> None:
    device.device_config.build_dir = resources
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [str(resources.joinpath('zephyr', 'zephyr.exe'))]


@mock.patch('shutil.which', return_value='west')
def test_if_custom_simulator_adapter_get_command_returns_proper_string(patched_which, tmp_path: Path) -> None:
    device = CustomSimulatorAdapter(DeviceConfig(build_dir=tmp_path))
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == ['west', 'build', '-d', str(tmp_path), '-t', 'run']


@mock.patch('shutil.which', return_value=None)
def test_if_custom_simulator_adapter_raise_exception_when_west_not_found(patched_which, tmp_path: Path) -> None:
    device = CustomSimulatorAdapter(DeviceConfig(build_dir=tmp_path))
    with pytest.raises(TwisterHarnessException, match='west not found'):
        device.generate_command()


def test_if_unit_simulator_adapter_get_command_returns_proper_string(tmp_path: Path) -> None:
    device = UnitSimulatorAdapter(DeviceConfig(build_dir=tmp_path))
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [str(tmp_path / 'testbinary')]
