# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import os
import time
from pathlib import Path
from unittest import mock

import pytest

from twister_harness.device.hardware_adapter import HardwareAdapter
from twister_harness.exceptions import TwisterHarnessException
from twister_harness.twister_harness_config import DeviceConfig


@pytest.fixture(name='device')
def fixture_adapter(tmp_path) -> HardwareAdapter:
    build_dir = tmp_path / 'build_dir'
    os.mkdir(build_dir)
    device_config = DeviceConfig(
        type='hardware',
        build_dir=build_dir,
        runner='runner',
        platform='platform',
        id='p_id',
        base_timeout=5.0,
    )
    return HardwareAdapter(device_config)


@mock.patch('shutil.which', return_value=None)
def test_if_hardware_adapter_raise_exception_when_west_not_found(patched_which, device: HardwareAdapter) -> None:
    with pytest.raises(TwisterHarnessException, match='west not found'):
        device.generate_command()


@mock.patch('shutil.which', return_value='west')
def test_if_get_command_returns_proper_string_1(patched_which, device: HardwareAdapter) -> None:
    device.device_config.build_dir = Path('build')
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == ['west', 'flash', '--skip-rebuild', '--build-dir', 'build', '--runner', 'runner']


@mock.patch('shutil.which', return_value='west')
def test_if_get_command_returns_proper_string_2(patched_which, device: HardwareAdapter) -> None:
    device.device_config.build_dir = Path('build')
    device.device_config.runner = 'pyocd'
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [
        'west', 'flash', '--skip-rebuild', '--build-dir', 'build', '--runner', 'pyocd', '--', '--board-id', 'p_id'
    ]


@mock.patch('shutil.which', return_value='west')
def test_if_get_command_returns_proper_string_3(patched_which, device: HardwareAdapter) -> None:
    device.device_config.build_dir = Path('build')
    device.device_config.runner = 'nrfjprog'
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [
        'west', 'flash', '--skip-rebuild', '--build-dir', 'build', '--runner', 'nrfjprog', '--', '--dev-id', 'p_id'
    ]


@mock.patch('shutil.which', return_value='west')
def test_if_get_command_returns_proper_string_4(patched_which, device: HardwareAdapter) -> None:
    device.device_config.build_dir = Path('build')
    device.device_config.runner = 'openocd'
    device.device_config.product = 'STM32 STLink'
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [
        'west', 'flash', '--skip-rebuild', '--build-dir', 'build', '--runner', 'openocd',
        '--', '--cmd-pre-init', 'hla_serial p_id'
    ]


@mock.patch('shutil.which', return_value='west')
def test_if_get_command_returns_proper_string_5(patched_which, device: HardwareAdapter) -> None:
    device.device_config.build_dir = Path('build')
    device.device_config.runner = 'openocd'
    device.device_config.product = 'EDBG CMSIS-DAP'
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [
        'west', 'flash', '--skip-rebuild', '--build-dir', 'build', '--runner', 'openocd',
        '--', '--cmd-pre-init', 'cmsis_dap_serial p_id'
    ]


@mock.patch('shutil.which', return_value='west')
def test_if_get_command_returns_proper_string_6(patched_which, device: HardwareAdapter) -> None:
    device.device_config.build_dir = Path('build')
    device.device_config.runner = 'jlink'
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [
        'west', 'flash', '--skip-rebuild', '--build-dir', 'build', '--runner', 'jlink',
        '--tool-opt=-SelectEmuBySN p_id'
    ]


@mock.patch('shutil.which', return_value='west')
def test_if_get_command_returns_proper_string_7(patched_which, device: HardwareAdapter) -> None:
    device.device_config.build_dir = Path('build')
    device.device_config.runner = 'stm32cubeprogrammer'
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [
        'west', 'flash', '--skip-rebuild', '--build-dir', 'build', '--runner', 'stm32cubeprogrammer',
        '--tool-opt=sn=p_id'
    ]


@mock.patch('shutil.which', return_value='west')
def test_if_get_command_returns_proper_string_8(patched_which, device: HardwareAdapter) -> None:
    device.device_config.build_dir = Path('build')
    device.device_config.runner = 'openocd'
    device.device_config.product = 'STLINK-V3'
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [
        'west', 'flash', '--skip-rebuild', '--build-dir', 'build',
        '--runner', 'openocd', '--', '--cmd-pre-init', 'hla_serial p_id'
    ]


@mock.patch('shutil.which', return_value='west')
def test_if_get_command_returns_proper_string_with_west_flash_extra_args(
    patched_which, device: HardwareAdapter
) -> None:
    device.device_config.build_dir = Path('build')
    device.device_config.west_flash_extra_args = ['--board-id=foobar', '--erase']
    device.device_config.runner = 'pyocd'
    device.device_config.id = ''
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [
        'west', 'flash', '--skip-rebuild', '--build-dir', 'build', '--runner', 'pyocd',
        '--', '--board-id=foobar', '--erase'
    ]


def test_if_hardware_adapter_raises_exception_empty_command(device: HardwareAdapter) -> None:
    device.command = []
    exception_msg = 'Flash command is empty, please verify if it was generated properly.'
    with pytest.raises(TwisterHarnessException, match=exception_msg):
        device._flash_and_run()


@mock.patch('twister_harness.device.hardware_adapter.subprocess.Popen')
def test_device_log_correct_error_handle(patched_popen, device: HardwareAdapter, tmp_path: Path) -> None:
    popen_mock = mock.Mock()
    popen_mock.communicate.return_value = (b'flashing error', b'')
    patched_popen.return_value = popen_mock
    device.device_config.build_dir = tmp_path
    device.command = [
        'west', 'flash', '--skip-rebuild', '--build-dir', str(tmp_path),
        '--runner', 'nrfjprog', '--', '--dev-id', 'p_id'
    ]
    with pytest.raises(expected_exception=TwisterHarnessException, match='Could not flash device p_id'):
        device._flash_and_run()
    assert os.path.isfile(device.device_log_path)
    with open(device.device_log_path, 'r') as file:
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

    device._device_run.set()
    device.connect()
    assert device._serial_connection.port == '/pty/ttytest/456'  # type: ignore[union-attr]
    assert device._serial_pty_proc
    patched_popen.assert_called_with(
        ['script.py'],
        stdout=123,
        stdin=123,
        stderr=123
    )

    device.disconnect()
    assert not device._serial_pty_proc


def test_if_hardware_adapter_properly_send_data_to_subprocess(
    device: HardwareAdapter, shell_simulator_path: str
) -> None:
    """
    Run shell_simulator.py under serial_pty, send "zen" command and verify
    output. Flashing command is mocked by "dummy" echo command.
    """
    device.command = ['echo', 'TEST']  # only to mock flashing command
    device.device_config.serial_pty = f'python3 {shell_simulator_path}'
    device.launch()
    time.sleep(0.1)
    device.write(b'zen\n')
    time.sleep(1)
    lines = device.readlines_until(regex='Namespaces are one honking great idea')
    assert 'The Zen of Python, by Tim Peters' in lines
    device.write(b'quit\n')
