# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import os
from pathlib import Path
from unittest import mock

import pytest

from twister_harness.device.hardware_adapter import HardwareAdapter
from twister_harness.exceptions import TwisterHarnessException
from twister_harness.log_files.log_file import DeviceLogFile, HandlerLogFile
from twister_harness.twister_harness_config import DeviceConfig


@pytest.fixture(name='device')
def fixture_adapter() -> HardwareAdapter:
    device_config = DeviceConfig(
        runner='runner',
        build_dir=Path('build'),
        platform='platform',
        id='p_id',
    )
    return HardwareAdapter(device_config)


@mock.patch('twister_harness.device.hardware_adapter.shutil.which')
def test_if_get_command_returns_proper_string_1(patched_which, device: HardwareAdapter) -> None:
    patched_which.return_value = 'west'
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == ['west', 'flash', '--skip-rebuild', '--build-dir', 'build', '--runner', 'runner']


@mock.patch('twister_harness.device.hardware_adapter.shutil.which')
def test_if_get_command_returns_proper_string_2(patched_which, device: HardwareAdapter) -> None:
    patched_which.return_value = 'west'
    device.device_config.runner = 'pyocd'
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [
        'west', 'flash', '--skip-rebuild', '--build-dir', 'build', '--runner', 'pyocd', '--', '--board-id', 'p_id'
    ]


@mock.patch('twister_harness.device.hardware_adapter.shutil.which')
def test_if_get_command_raise_exception_if_west_is_not_installed(patched_which, device: HardwareAdapter) -> None:
    patched_which.return_value = None
    with pytest.raises(TwisterHarnessException, match='west not found'):
        device.generate_command()


@mock.patch('twister_harness.device.hardware_adapter.shutil.which')
def test_if_get_command_returns_proper_string_3(patched_which, device: HardwareAdapter) -> None:
    patched_which.return_value = 'west'
    device.device_config.runner = 'nrfjprog'
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [
        'west', 'flash', '--skip-rebuild', '--build-dir', 'build', '--runner', 'nrfjprog', '--', '--dev-id', 'p_id'
    ]


@mock.patch('twister_harness.device.hardware_adapter.shutil.which')
def test_if_get_command_returns_proper_string_4(patched_which, device: HardwareAdapter) -> None:
    patched_which.return_value = 'west'
    device.device_config.runner = 'openocd'
    device.device_config.product = 'STM32 STLink'
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [
        'west', 'flash', '--skip-rebuild', '--build-dir', 'build', '--runner', 'openocd',
        '--', '--cmd-pre-init', 'hla_serial p_id'
    ]


@mock.patch('twister_harness.device.hardware_adapter.shutil.which')
def test_if_get_command_returns_proper_string_5(patched_which, device: HardwareAdapter) -> None:
    patched_which.return_value = 'west'
    device.device_config.runner = 'openocd'
    device.device_config.product = 'EDBG CMSIS-DAP'
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [
        'west', 'flash', '--skip-rebuild', '--build-dir', 'build', '--runner', 'openocd',
        '--', '--cmd-pre-init', 'cmsis_dap_serial p_id'
    ]


@mock.patch('twister_harness.device.hardware_adapter.shutil.which')
def test_if_get_command_returns_proper_string_6(patched_which, device: HardwareAdapter) -> None:
    patched_which.return_value = 'west'
    device.device_config.runner = 'jlink'
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [
        'west', 'flash', '--skip-rebuild', '--build-dir', 'build', '--runner', 'jlink',
        '--tool-opt=-SelectEmuBySN p_id'
    ]


@mock.patch('twister_harness.device.hardware_adapter.shutil.which')
def test_if_get_command_returns_proper_string_7(patched_which, device: HardwareAdapter) -> None:
    patched_which.return_value = 'west'
    device.device_config.runner = 'stm32cubeprogrammer'
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [
        'west', 'flash', '--skip-rebuild', '--build-dir', 'build', '--runner', 'stm32cubeprogrammer',
        '--tool-opt=sn=p_id'
    ]


@mock.patch('twister_harness.device.hardware_adapter.shutil.which')
def test_if_get_command_returns_proper_string_8(patched_which, device: HardwareAdapter) -> None:
    patched_which.return_value = 'west'
    device.device_config.runner = 'openocd'
    device.device_config.product = 'STLINK-V3'
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [
        'west', 'flash', '--skip-rebuild', '--build-dir', 'build',
        '--runner', 'openocd', '--', '--cmd-pre-init', 'hla_serial p_id'
    ]


def test_if_hardware_adapter_raises_exception_empty_command(device: HardwareAdapter) -> None:
    device.command = []
    exception_msg = 'Flash command is empty, please verify if it was generated properly.'
    with pytest.raises(TwisterHarnessException, match=exception_msg):
        device.flash_and_run()


def test_handler_and_device_log_correct_initialized_on_hardware(device: HardwareAdapter, tmp_path: Path) -> None:
    device.device_config.build_dir = tmp_path
    device.initialize_log_files()
    assert isinstance(device.handler_log_file, HandlerLogFile)
    assert isinstance(device.device_log_file, DeviceLogFile)
    assert device.handler_log_file.filename.endswith('handler.log')  # type: ignore[union-attr]
    assert device.device_log_file.filename.endswith('device.log')  # type: ignore[union-attr]


@mock.patch('twister_harness.device.hardware_adapter.subprocess.Popen')
def test_device_log_correct_error_handle(patched_popen, device: HardwareAdapter, tmp_path: Path) -> None:
    popen_mock = mock.Mock()
    popen_mock.communicate.return_value = (b'', b'flashing error')
    patched_popen.return_value = popen_mock
    device.device_config.build_dir = tmp_path
    device.initialize_log_files()
    device.command = [
        'west', 'flash', '--skip-rebuild', '--build-dir', str(tmp_path),
        '--runner', 'nrfjprog', '--', '--dev-id', 'p_id'
    ]
    with pytest.raises(expected_exception=TwisterHarnessException, match='Could not flash device p_id'):
        device.flash_and_run()
    assert os.path.isfile(device.device_log_file.filename)
    with open(device.device_log_file.filename, 'r') as file:
        assert 'flashing error' in file.readlines()


@mock.patch('twister_harness.device.hardware_adapter.subprocess.Popen')
@mock.patch('twister_harness.device.hardware_adapter.serial.Serial')
def test_if_hardware_adapter_uses_serial_pty(
    patched_serial, patched_popen, device: HardwareAdapter, monkeypatch: pytest.MonkeyPatch
):
    device.device_config.serial_pty = 'script.py'

    popen_mock = mock.Mock()
    popen_mock.communicate.return_value = (b'output', b'error')
    patched_popen.return_value = popen_mock

    monkeypatch.setattr('twister_harness.device.hardware_adapter.pty.openpty', lambda: (123, 456))
    monkeypatch.setattr('twister_harness.device.hardware_adapter.os.ttyname', lambda x: f'/pty/ttytest/{x}')

    serial_mock = mock.Mock()
    serial_mock.port = '/pty/ttytest/456'
    patched_serial.return_value = serial_mock

    device.connect()
    assert device.connection.port == '/pty/ttytest/456'  # type: ignore[union-attr]
    assert device.serial_pty_proc
    patched_popen.assert_called_with(
        ['script.py'],
        stdout=123,
        stdin=123,
        stderr=123
    )

    device.disconnect()
    assert not device.connection
    assert not device.serial_pty_proc


@mock.patch('twister_harness.device.hardware_adapter.shutil.which')
def test_if_get_command_returns_proper_string_with_west_flash(patched_which, device: HardwareAdapter) -> None:
    patched_which.return_value = 'west'
    device.device_config.west_flash_extra_args = ['--board-id=foobar', '--erase']
    device.device_config.runner = 'pyocd'
    device.device_config.id = ''
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [
        'west', 'flash', '--skip-rebuild', '--build-dir', 'build', '--runner', 'pyocd',
        '--', '--board-id=foobar', '--erase'
    ]
