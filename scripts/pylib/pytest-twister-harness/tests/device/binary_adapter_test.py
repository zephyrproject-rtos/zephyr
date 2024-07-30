# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import logging
import os
import subprocess
import time
from pathlib import Path
from typing import Generator
from unittest import mock

import pytest

from twister_harness.device.binary_adapter import (
    CustomSimulatorAdapter,
    NativeSimulatorAdapter,
    UnitSimulatorAdapter,
)
from twister_harness.exceptions import TwisterHarnessException, TwisterHarnessTimeoutException
from twister_harness.twister_harness_config import DeviceConfig


@pytest.fixture
def script_path(resources: Path) -> str:
    return str(resources.joinpath('mock_script.py'))


@pytest.fixture(name='device')
def fixture_device_adapter(tmp_path: Path) -> Generator[NativeSimulatorAdapter, None, None]:
    build_dir = tmp_path / 'build_dir'
    os.mkdir(build_dir)
    device = NativeSimulatorAdapter(DeviceConfig(build_dir=build_dir, type='native', base_timeout=5.0))
    try:
        yield device
    finally:
        device.close()  # to make sure all running processes are closed


@pytest.fixture(name='launched_device')
def fixture_launched_device_adapter(
    device: NativeSimulatorAdapter, script_path: str
) -> Generator[NativeSimulatorAdapter, None, None]:
    device.command = ['python3', script_path]
    try:
        device.launch()
        yield device
    finally:
        device.close()  # to make sure all running processes are closed


def test_if_binary_adapter_runs_without_errors(launched_device: NativeSimulatorAdapter) -> None:
    """
    Run script which prints text line by line and ends without errors.
    Verify if subprocess was ended without errors, and without timeout.
    """
    device = launched_device
    lines = device.readlines_until(regex='Returns with code')
    device.close()
    assert 'Readability counts.' in lines
    assert os.path.isfile(device.handler_log_path)
    with open(device.handler_log_path, 'r') as file:
        file_lines = [line.strip() for line in file.readlines()]
    assert file_lines[-2:] == lines[-2:]


def test_if_binary_adapter_finishes_after_timeout_while_there_is_no_data_from_subprocess(
    device: NativeSimulatorAdapter, script_path: str
) -> None:
    """Test if thread finishes after timeout when there is no data on stdout, but subprocess is still running"""
    device.base_timeout = 0.3
    device.command = ['python3', script_path, '--long-sleep', '--sleep=5']
    device.launch()
    with pytest.raises(TwisterHarnessTimeoutException, match='Read from device timeout occurred'):
        device.readlines_until(regex='Returns with code')
    device.close()
    assert device._process is None
    with open(device.handler_log_path, 'r') as file:
        file_lines = [line.strip() for line in file.readlines()]
    # this message should not be printed because script has been terminated due to timeout
    assert 'End of script' not in file_lines, 'Script has not been terminated before end'


def test_if_binary_adapter_raises_exception_empty_command(device: NativeSimulatorAdapter) -> None:
    device.command = []
    exception_msg = 'Run command is empty, please verify if it was generated properly.'
    with pytest.raises(TwisterHarnessException, match=exception_msg):
        device._flash_and_run()


@mock.patch('subprocess.Popen', side_effect=subprocess.SubprocessError(1, 'Exception message'))
def test_if_binary_adapter_raises_exception_when_subprocess_raised_subprocess_error(
    patched_popen, device: NativeSimulatorAdapter
) -> None:
    device.command = ['echo', 'TEST']
    with pytest.raises(TwisterHarnessException, match='Exception message'):
        device._flash_and_run()


@mock.patch('subprocess.Popen', side_effect=FileNotFoundError(1, 'File not found', 'fake_file.txt'))
def test_if_binary_adapter_raises_exception_file_not_found(
    patched_popen, device: NativeSimulatorAdapter
) -> None:
    device.command = ['echo', 'TEST']
    with pytest.raises(TwisterHarnessException, match='fake_file.txt'):
        device._flash_and_run()


@mock.patch('subprocess.Popen', side_effect=Exception(1, 'Raised other exception'))
def test_if_binary_adapter_raises_exception_when_subprocess_raised_an_error(
    patched_popen, device: NativeSimulatorAdapter
) -> None:
    device.command = ['echo', 'TEST']
    with pytest.raises(TwisterHarnessException, match='Raised other exception'):
        device._flash_and_run()


def test_if_binary_adapter_connect_disconnect_print_warnings_properly(
    caplog: pytest.LogCaptureFixture, launched_device: NativeSimulatorAdapter
) -> None:
    device = launched_device
    assert device._device_connected.is_set() and device.is_device_connected()
    caplog.set_level(logging.DEBUG)
    device.connect()
    warning_msg = 'Device already connected'
    assert warning_msg in caplog.text
    for record in caplog.records:
        if record.message == warning_msg:
            assert record.levelname == 'DEBUG'
            break
    device.disconnect()
    assert not device._device_connected.is_set() and not device.is_device_connected()
    device.disconnect()
    warning_msg = 'Device already disconnected'
    assert warning_msg in caplog.text
    for record in caplog.records:
        if record.message == warning_msg:
            assert record.levelname == 'DEBUG'
            break


def test_if_binary_adapter_raise_exc_during_connect_read_and_write_after_close(
    launched_device: NativeSimulatorAdapter
) -> None:
    device = launched_device
    assert device._device_run.is_set() and device.is_device_running()
    device.close()
    assert not device._device_run.is_set() and not device.is_device_running()
    with pytest.raises(TwisterHarnessException, match='Cannot connect to not working device'):
        device.connect()
    with pytest.raises(TwisterHarnessException, match='No connection to the device'):
        device.write(b'')
    device.clear_buffer()
    with pytest.raises(TwisterHarnessException, match='No connection to the device and no more data to read.'):
        device.readline()


def test_if_binary_adapter_raise_exc_during_read_and_write_after_close(
    launched_device: NativeSimulatorAdapter
) -> None:
    device = launched_device
    device.disconnect()
    with pytest.raises(TwisterHarnessException, match='No connection to the device'):
        device.write(b'')
    device.clear_buffer()
    with pytest.raises(TwisterHarnessException, match='No connection to the device and no more data to read.'):
        device.readline()


def test_if_binary_adapter_is_able_to_read_leftovers_after_disconnect_or_close(
    device: NativeSimulatorAdapter, script_path: str
) -> None:
    device.command = ['python3', script_path, '--sleep=0.05']
    device.launch()
    device.readlines_until(regex='Beautiful is better than ugly.')
    time.sleep(0.1)
    device.disconnect()
    assert len(device.readlines()) > 0
    device.connect()
    device.readlines_until(regex='Flat is better than nested.')
    time.sleep(0.1)
    device.close()
    assert len(device.readlines()) > 0


def test_if_binary_adapter_properly_send_data_to_subprocess(
    shell_simulator_adapter: NativeSimulatorAdapter
) -> None:
    """Run shell_simulator.py program, send "zen" command and verify output."""
    device = shell_simulator_adapter
    time.sleep(0.1)
    device.write(b'zen\n')
    time.sleep(1)
    lines = device.readlines_until(regex='Namespaces are one honking great idea')
    assert 'The Zen of Python, by Tim Peters' in lines


def test_if_native_binary_adapter_get_command_returns_proper_string(device: NativeSimulatorAdapter) -> None:
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [str(device.device_config.build_dir / 'zephyr' / 'zephyr.exe')]


@mock.patch('shutil.which', return_value='west')
def test_if_custom_binary_adapter_get_command_returns_proper_string(patched_which, tmp_path: Path) -> None:
    device = CustomSimulatorAdapter(DeviceConfig(build_dir=tmp_path, type='custom'))
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == ['west', 'build', '-d', str(tmp_path), '-t', 'run']


@mock.patch('shutil.which', return_value=None)
def test_if_custom_binary_adapter_raise_exception_when_west_not_found(patched_which, tmp_path: Path) -> None:
    device = CustomSimulatorAdapter(DeviceConfig(build_dir=tmp_path, type='custom'))
    with pytest.raises(TwisterHarnessException, match='west not found'):
        device.generate_command()


def test_if_unit_binary_adapter_get_command_returns_proper_string(tmp_path: Path) -> None:
    device = UnitSimulatorAdapter(DeviceConfig(build_dir=tmp_path, type='unit'))
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [str(tmp_path / 'testbinary')]
