# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

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
from twister_harness.exceptions import TwisterHarnessException
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
    assert os.path.isfile(device.connections[0].log_path)
    with open(device.connections[0].log_path, 'r') as file:
        file_lines = [line.strip() for line in file.readlines()]
    assert file_lines[-2:] == lines[-2:]


def test_if_binary_adapter_finishes_after_timeout_while_there_is_no_data_from_subprocess(
    device: NativeSimulatorAdapter, script_path: str
) -> None:
    """Test if thread finishes after timeout when there is no data on stdout, but subprocess is still running"""
    device.connections[0].timeout = 0.3
    device.command = ['python3', script_path, '--long-sleep', '--sleep=5']
    device.launch()
    with pytest.raises(AssertionError, match='Did not find line "Returns with code" within 0.3 seconds'):
        device.readlines_until(regex='Returns with code')
    device.close()
    assert device._process is None
    with open(device.connections[0].log_path, 'r') as file:
        file_lines = [line.strip() for line in file.readlines()]
    # this message should not be printed because script has been terminated due to timeout
    assert 'End of script' not in file_lines, 'Script has not been terminated before end'


def test_if_binary_adapter_raises_exception_empty_command(device: NativeSimulatorAdapter) -> None:
    device.command = []
    exception_msg = 'Run command is empty, please verify if it was generated properly.'
    with pytest.raises(TwisterHarnessException, match=exception_msg):
        device._device_launch()


@mock.patch('subprocess.Popen', side_effect=subprocess.SubprocessError(1, 'Exception message'))
def test_if_binary_adapter_raises_exception_when_subprocess_raised_subprocess_error(
    patched_popen, device: NativeSimulatorAdapter
) -> None:
    device.command = ['echo', 'TEST']
    with pytest.raises(TwisterHarnessException, match='Exception message'):
        device._device_launch()


@mock.patch('subprocess.Popen', side_effect=FileNotFoundError(1, 'File not found', 'fake_file.txt'))
def test_if_binary_adapter_raises_exception_file_not_found(
    patched_popen, device: NativeSimulatorAdapter
) -> None:
    device.command = ['echo', 'TEST']
    with pytest.raises(TwisterHarnessException, match='fake_file.txt'):
        device._device_launch()


@mock.patch('subprocess.Popen', side_effect=Exception(1, 'Raised other exception'))
def test_if_binary_adapter_raises_exception_when_subprocess_raised_an_error(
    patched_popen, device: NativeSimulatorAdapter
) -> None:
    device.command = ['echo', 'TEST']
    with pytest.raises(TwisterHarnessException, match='Raised other exception'):
        device._device_launch()


def test_if_binary_adapter_raise_exc_during_connect_read_and_write_after_close(
    launched_device: NativeSimulatorAdapter
) -> None:
    device = launched_device
    assert device._reader_started.is_set() and device.connections[0]._is_binary_running()
    device.close()
    assert not device._reader_started.is_set() and not device.connections[0]._is_binary_running()
    with pytest.raises(TwisterHarnessException, match='Cannot write to not connected device'):
        device.write(b'')
    device.clear_buffer()
    with pytest.raises(TwisterHarnessException, match='No connection to the device and no more data to read.'):
        device.readline()


def test_if_binary_adapter_raise_exc_during_read_and_write_after_close(
    launched_device: NativeSimulatorAdapter
) -> None:
    device = launched_device
    device.close()
    with pytest.raises(TwisterHarnessException, match='Cannot write to not connected device'):
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
    assert len(device.readlines()) > 0
    device.close()


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


@mock.patch('subprocess.Popen')
def test_set_extra_args_appends_to_command_on_launch(patched_popen, device: NativeSimulatorAdapter) -> None:
    """``set_extra_args`` should append its args to the auto-generated
    command at launch time, taking precedence over ``extra_test_args``."""
    device.device_config.extra_test_args = '--static-arg'
    device.set_extra_args(['--per-test', '--eeprom=/tmp/eep.bin'])
    device.launch()
    binary = str(device.device_config.build_dir / 'zephyr' / 'zephyr.exe')
    assert device.command == [binary, '--per-test', '--eeprom=/tmp/eep.bin']
    device.close()


@mock.patch('subprocess.Popen')
def test_set_extra_args_overrides_take_effect_across_launches(
    patched_popen, device: NativeSimulatorAdapter
) -> None:
    """A second ``set_extra_args`` call must replace the first — without
    this, the cached ``command`` from a previous launch would leak."""
    device.set_extra_args(['--first'])
    device.launch()
    device.close()
    device.set_extra_args(['--second'])
    device.launch()
    binary = str(device.device_config.build_dir / 'zephyr' / 'zephyr.exe')
    assert device.command == [binary, '--second']
    device.close()


@mock.patch('subprocess.Popen')
def test_extra_test_args_static_fallback_preserved(
    patched_popen, device: NativeSimulatorAdapter
) -> None:
    """When no per-test override is set, the static ``extra_test_args``
    CLI value flows through unchanged — backward compat for callers that
    have never used ``set_extra_args``."""
    device.device_config.extra_test_args = '--a --b=c'
    device.launch()
    binary = str(device.device_config.build_dir / 'zephyr' / 'zephyr.exe')
    assert device.command == [binary, '--a', '--b=c']
    device.close()


@mock.patch('subprocess.Popen')
def test_set_extra_args_none_preserves_static(
    patched_popen, device: NativeSimulatorAdapter
) -> None:
    """``set_extra_args(None)`` means 'no override' — the static
    ``extra_test_args`` flows through unchanged. The ``dut`` fixture
    relies on this when the user-overridable ``device_extra_args``
    fixture returns its default of ``None``."""
    device.device_config.extra_test_args = '--static-arg'
    device.set_extra_args(None)
    device.launch()
    binary = str(device.device_config.build_dir / 'zephyr' / 'zephyr.exe')
    assert device.command == [binary, '--static-arg']
    device.close()


@mock.patch('subprocess.Popen')
def test_set_extra_args_empty_list_suppresses_static(
    patched_popen, device: NativeSimulatorAdapter
) -> None:
    """``set_extra_args([])`` is an *explicit* empty override — distinct
    from ``None``. It launches the binary with no extra args at all,
    suppressing the static value. Lets a test opt out of CLI-supplied
    ``--extra-test-args`` for that one launch."""
    device.device_config.extra_test_args = '--static-arg'
    device.set_extra_args([])
    device.launch()
    binary = str(device.device_config.build_dir / 'zephyr' / 'zephyr.exe')
    assert device.command == [binary]
    device.close()


@mock.patch('subprocess.Popen')
def test_set_extra_args_can_be_cleared_back_to_static(
    patched_popen, device: NativeSimulatorAdapter
) -> None:
    """A test that opts into the override must be able to opt back out
    by passing ``None`` — what makes the dut fixture's 'always call
    set_extra_args' pattern safe across mixed test suites."""
    device.device_config.extra_test_args = '--static-arg'
    device.set_extra_args(['--per-test'])
    device.launch()
    device.close()
    device.set_extra_args(None)  # clear the override
    device.launch()
    binary = str(device.device_config.build_dir / 'zephyr' / 'zephyr.exe')
    assert device.command == [binary, '--static-arg']
    device.close()


@mock.patch('subprocess.Popen')
def test_externally_set_command_still_honored(
    patched_popen, device: NativeSimulatorAdapter
) -> None:
    """Tests that manipulate ``device.command`` directly (pre-existing
    pattern used by this very file's fixtures) must still work — only
    explicit ``set_extra_args`` calls trigger regeneration."""
    device.command = ['python3', '/path/to/script.py']
    device.launch()
    assert device.command == ['python3', '/path/to/script.py']
    device.close()
