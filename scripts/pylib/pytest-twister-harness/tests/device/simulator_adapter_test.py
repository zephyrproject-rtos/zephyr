# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import os
import subprocess
from pathlib import Path
from unittest import mock

import pytest

from twister_harness.device.simulator_adapter import (
    CustomSimulatorAdapter,
    NativeSimulatorAdapter,
    UnitSimulatorAdapter,
)
from twister_harness.exceptions import TwisterHarnessException
from twister_harness.log_files.log_file import HandlerLogFile, NullLogFile
from twister_harness.twister_harness_config import DeviceConfig


@pytest.fixture(name='device')
def fixture_adapter(tmp_path) -> NativeSimulatorAdapter:
    return NativeSimulatorAdapter(DeviceConfig(build_dir=tmp_path))


def test_if_native_simulator_adapter_get_command_returns_proper_string(
        device: NativeSimulatorAdapter, resources: Path
) -> None:
    device.device_config.build_dir = resources
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [str(resources.joinpath('zephyr', 'zephyr.exe'))]


def test_if_native_simulator_adapter_runs_without_errors(
        resources: Path, device: NativeSimulatorAdapter
) -> None:
    """
    Run script which prints text line by line and ends without errors.
    Verify if subprocess was ended without errors, and without timeout.
    """
    script_path = resources.joinpath('mock_script.py')
    # patching original command by mock_script.py to simulate same behaviour as zephyr.exe
    device.command = ['python3', str(script_path)]
    device.initialize_log_files()
    device.flash_and_run(timeout=4)
    lines = list(device.iter_stdout)  # give it time before close thread
    device.stop()
    assert device._process_ended_with_timeout is False
    assert 'Readability counts.' in lines
    assert os.path.isfile(device.handler_log_file.filename)
    with open(device.handler_log_file.filename, 'r') as file:
        file_lines = [line.strip() for line in file.readlines()]
    assert file_lines[-2:] == lines[-2:]


def test_if_native_simulator_adapter_finishes_after_timeout_while_there_is_no_data_from_subprocess(
        resources: Path, device: NativeSimulatorAdapter
) -> None:
    """Test if thread finishes after timeout when there is no data on stdout, but subprocess is still running"""
    script_path = resources.joinpath('mock_script.py')
    device.command = ['python3', str(script_path), '--long-sleep', '--sleep=5']
    device.initialize_log_files()
    device.flash_and_run(timeout=0.5)
    lines = list(device.iter_stdout)
    device.stop()
    assert device._process_ended_with_timeout is True
    assert device._exc is None
    # this message should not be printed because script has been terminated due to timeout
    assert 'End of script' not in lines, 'Script has not been terminated before end'


def test_if_native_simulator_adapter_raises_exception_file_not_found(device: NativeSimulatorAdapter) -> None:
    device.command = ['dummy']
    with pytest.raises(TwisterHarnessException, match='File not found: dummy'):
        device.flash_and_run(timeout=0.1)
        device.stop()
    assert device._exc is not None
    assert isinstance(device._exc, TwisterHarnessException)


def test_if_simulator_adapter_raises_exception_empty_command(device: NativeSimulatorAdapter) -> None:
    device.command = []
    exception_msg = 'Run simulation command is empty, please verify if it was generated properly.'
    with pytest.raises(TwisterHarnessException, match=exception_msg):
        device.flash_and_run(timeout=0.1)


def test_handler_and_device_log_correct_initialized_on_simulators(device: NativeSimulatorAdapter) -> None:
    device.initialize_log_files()
    assert isinstance(device.handler_log_file, HandlerLogFile)
    assert isinstance(device.device_log_file, NullLogFile)
    assert device.handler_log_file.filename.endswith('handler.log')  # type: ignore[union-attr]


@mock.patch('asyncio.run', side_effect=subprocess.SubprocessError(1, 'Exception message'))
def test_if_simulator_adapter_raises_exception_when_subprocess_raised_subprocess_error(
    patched_run, device: NativeSimulatorAdapter
):
    device.command = ['echo', 'TEST']
    with pytest.raises(TwisterHarnessException, match='Exception message'):
        device.flash_and_run(timeout=0.1)
        device.stop()


@mock.patch('asyncio.run', side_effect=Exception(1, 'Raised other exception'))
def test_if_simulator_adapter_raises_exception_when_subprocess_raised_an_error(
    patched_run, device: NativeSimulatorAdapter
):
    device.command = ['echo', 'TEST']
    with pytest.raises(TwisterHarnessException, match='Raised other exception'):
        device.flash_and_run(timeout=0.1)
        device.stop()


@mock.patch('shutil.which', return_value='west')
def test_if_custom_simulator_adapter_get_command_returns_proper_string(patched_which) -> None:
    device = CustomSimulatorAdapter(DeviceConfig(build_dir='build_dir'))
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == ['west', 'build', '-d', 'build_dir', '-t', 'run']


@mock.patch('shutil.which', return_value=None)
def test_if_custom_simulator_adapter_get_command_returns_empty_string(patched_which) -> None:
    device = CustomSimulatorAdapter(DeviceConfig(build_dir='build_dir'))
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == []


def test_if_unit_simulator_adapter_get_command_returns_proper_string(resources: Path) -> None:
    device = UnitSimulatorAdapter(DeviceConfig(build_dir=resources))
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [str(resources.joinpath('testbinary'))]
