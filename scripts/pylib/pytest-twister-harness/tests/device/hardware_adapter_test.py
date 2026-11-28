# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import os
import time
from pathlib import Path
from unittest import mock

import pytest

from twister_harness.device.device_connection import create_device_connections
from twister_harness.device.hardware_adapter import HardwareAdapter
from twister_harness.exceptions import TwisterHarnessException
from twister_harness.twister_harness_config import DeviceConfig, DeviceSerialConfig


@pytest.fixture(name='device')
def fixture_adapter(tmp_path) -> HardwareAdapter:
    build_dir = tmp_path / 'build_dir'
    os.mkdir(build_dir)
    serial_configs = [DeviceSerialConfig(port='')]
    device_config = DeviceConfig(
        type='hardware',
        build_dir=build_dir,
        runner='runner',
        platform='platform',
        id='p_id',
        base_timeout=5.0,
        flash_command='',
        serial_configs=serial_configs,
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
    assert device.command == ['west', 'flash', '--no-rebuild', '--build-dir', 'build', '--runner', 'runner']


@mock.patch('shutil.which', return_value='west')
def test_if_get_command_returns_proper_string_2(patched_which, device: HardwareAdapter) -> None:
    device.device_config.build_dir = Path('build')
    device.device_config.runner = 'pyocd'
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [
        'west', 'flash', '--no-rebuild', '--build-dir', 'build', '--runner', 'pyocd', '--', '--board-id', 'p_id'
    ]


@mock.patch('shutil.which', return_value='west')
def test_if_get_command_returns_proper_string_3(patched_which, device: HardwareAdapter) -> None:
    device.device_config.build_dir = Path('build')
    device.device_config.runner = 'nrfjprog'
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [
        'west', 'flash', '--no-rebuild', '--build-dir', 'build', '--runner', 'nrfjprog', '--', '--dev-id', 'p_id'
    ]


@mock.patch('shutil.which', return_value='west')
def test_if_get_command_returns_proper_string_4(patched_which, device: HardwareAdapter) -> None:
    device.device_config.build_dir = Path('build')
    device.device_config.runner = 'openocd'
    device.device_config.product = 'STM32 STLink'
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [
        'west', 'flash', '--no-rebuild', '--build-dir', 'build', '--runner', 'openocd',
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
        'west', 'flash', '--no-rebuild', '--build-dir', 'build', '--runner', 'openocd',
        '--', '--cmd-pre-init', 'adapter serial p_id'
    ]


@mock.patch('shutil.which', return_value='west')
def test_if_get_command_returns_proper_string_6(patched_which, device: HardwareAdapter) -> None:
    device.device_config.build_dir = Path('build')
    device.device_config.runner = 'jlink'
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [
        'west', 'flash', '--no-rebuild', '--build-dir', 'build', '--runner', 'jlink',
        '--dev-id', 'p_id'
    ]


@mock.patch('shutil.which', return_value='west')
def test_if_get_command_returns_proper_string_7(patched_which, device: HardwareAdapter) -> None:
    device.device_config.build_dir = Path('build')
    device.device_config.runner = 'stm32cubeprogrammer'
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [
        'west', 'flash', '--no-rebuild', '--build-dir', 'build', '--runner', 'stm32cubeprogrammer',
        '--dev-id', 'p_id'
    ]


@mock.patch('shutil.which', return_value='west')
def test_if_get_command_returns_proper_string_8(patched_which, device: HardwareAdapter) -> None:
    device.device_config.build_dir = Path('build')
    device.device_config.runner = 'openocd'
    device.device_config.product = 'STLINK-V3'
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [
        'west', 'flash', '--no-rebuild', '--build-dir', 'build',
        '--runner', 'openocd', '--', '--cmd-pre-init', 'hla_serial p_id'
    ]


@mock.patch('shutil.which', return_value='west')
def test_if_get_command_returns_proper_string_with_base_params(patched_which, device: HardwareAdapter) -> None:
    device.device_config.build_dir = Path('build')
    device.device_config.base_params = ['--base-param1', '--base-param2']
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [
        'west', 'flash', '--no-rebuild', '--build-dir', 'build',
        '--runner', 'runner', '--base-param1', '--base-param2'
    ]


@mock.patch('shutil.which', return_value='west')
def test_if_get_command_returns_proper_string_with_runner_params_1(patched_which, device: HardwareAdapter) -> None:
    device.device_config.build_dir = Path('build')
    device.device_config.runner_params = ['--runner-param1', 'runner-param2']
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [
        'west', 'flash', '--no-rebuild', '--build-dir', 'build',
        '--runner', 'runner', '--', '--runner-param1', 'runner-param2'
    ]


@mock.patch('shutil.which', return_value='west')
def test_if_get_command_returns_proper_string_with_runner_params_2(patched_which, device: HardwareAdapter) -> None:
    device.device_config.build_dir = Path('build')
    device.device_config.runner = 'openocd'
    device.device_config.runner_params = [
        '--cmd-pre-init', 'adapter serial FT1LRSRD',
        '--cmd-pre-init', 'source [find interface/ftdi/jtag-lock-pick_tiny_2.cfg]',
        '--cmd-pre-init', 'transport select swd',
        '--cmd-pre-init', 'source [find target/nrf52.cfg]',
        '--cmd-pre-init', 'adapter speed 10000',
    ]
    device.device_config.product = 'JTAG-lock-pick Tiny 2'
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == [
        'west', 'flash', '--no-rebuild', '--build-dir', 'build',
        '--runner', 'openocd', '--',
        '--cmd-pre-init', 'adapter serial FT1LRSRD',
        '--cmd-pre-init', 'source [find interface/ftdi/jtag-lock-pick_tiny_2.cfg]',
        '--cmd-pre-init', 'transport select swd',
        '--cmd-pre-init', 'source [find target/nrf52.cfg]',
        '--cmd-pre-init', 'adapter speed 10000',
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
        'west', 'flash', '--no-rebuild', '--build-dir', 'build', '--runner', 'pyocd',
        '--', '--board-id=foobar', '--erase'
    ]


def test_if_get_command_flash_command(device: HardwareAdapter) -> None:
    device.device_config.build_dir = Path('build')
    device.device_config.flash_command = ['flash_command', '--with-arg']
    device.generate_command()
    assert isinstance(device.command, list)
    assert device.command == ['flash_command', '--build-dir', 'build', '--board-id', 'p_id', '--with-arg']


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
        'west', 'flash', '--no-rebuild', '--build-dir', str(tmp_path),
        '--runner', 'nrfjprog', '--', '--dev-id', 'p_id'
    ]
    with pytest.raises(expected_exception=TwisterHarnessException, match='Could not flash device p_id'):
        device._flash_and_run()
    assert os.path.isfile(device.device_log_path)
    with open(device.device_log_path, 'r') as file:
        assert 'flashing error' in file.readlines()


@mock.patch('twister_harness.device.device_connection.subprocess.Popen')
@mock.patch('twister_harness.device.device_connection.serial.Serial')
def test_if_hardware_adapter_uses_serial_pty(
    patched_serial, patched_popen, device: HardwareAdapter, monkeypatch: pytest.MonkeyPatch
):
    device.device_config.serial_configs[0].serial_pty = 'script.py'

    popen_mock = mock.Mock()
    popen_mock.communicate.return_value = (b'output', b'error')
    patched_popen.return_value = popen_mock

    monkeypatch.setattr('twister_harness.device.device_connection.pty.openpty', lambda: (123, 456))
    monkeypatch.setattr('twister_harness.device.device_connection.os.ttyname', lambda x: f'/pty/ttytest/{x}')

    serial_mock = mock.Mock()
    serial_mock.port = '/pty/ttytest/456'
    patched_serial.return_value = serial_mock

    device._reader_started.set()
    device.connect()
    assert device.connections[0]._serial_connection.port == '/pty/ttytest/456'  # type: ignore[union-attr]
    assert device.connections[0]._serial_pty_proc
    patched_popen.assert_called_with(
        ['script.py'],
        stdout=123,
        stdin=123,
        stderr=123
    )

    device.disconnect()
    assert not device.connections[0]._serial_pty_proc


def test_if_hardware_adapter_properly_send_data_to_subprocess(
    device: HardwareAdapter, shell_simulator_path: str
) -> None:
    """
    Run shell_simulator.py under serial_pty, send "zen" command and verify
    output. Flashing command is mocked by "dummy" echo command.
    """
    device.command = ['echo', 'TEST']  # only to mock flashing command
    device.device_config.serial_configs[0].serial_pty = f'python3 {shell_simulator_path}'
    device.launch()
    time.sleep(0.1)
    device.write(b'zen\n')
    time.sleep(1)
    lines = device.readlines_until(regex='Namespaces are one honking great idea')
    assert 'The Zen of Python, by Tim Peters' in lines
    device.write(b'quit\n')


@mock.patch('shutil.which', return_value='west')
def test_if_get_command_adds_no_reset_for_post_connect_reset(patched_which, device: HardwareAdapter) -> None:
    device.device_config.build_dir = Path('build')
    device.device_config.post_connect_reset = {'method': 'west'}
    device.device_config.west_flash_extra_args = ['--erase']
    device.generate_command()
    assert device.command == [
        'west', 'flash', '--no-rebuild', '--build-dir', 'build', '--runner', 'runner', '--', '--erase', '--no-reset'
    ]


@mock.patch('shutil.which', return_value='west')
def test_if_get_command_does_not_repeat_no_reset(patched_which, device: HardwareAdapter) -> None:
    device.device_config.build_dir = Path('build')
    device.device_config.post_connect_reset = {'method': 'west'}
    device.device_config.west_flash_extra_args = ['--no-reset']
    device.generate_command()
    assert device.command.count('--no-reset') == 1


@mock.patch('shutil.which', return_value='west')
def test_if_west_reset_command_skips_flash_args(patched_which, device: HardwareAdapter) -> None:
    device.device_config.runner = 'pyocd'
    device.device_config.post_connect_reset = {'method': 'west'}
    device.device_config.west_flash_extra_args = ['--erase']
    command = device._build_west_command('reset', Path('build'))
    assert command == [
        'west', 'reset', '--no-rebuild', '--build-dir', 'build', '--runner', 'pyocd', '--', '--board-id', 'p_id'
    ]


@pytest.mark.parametrize('flash_before', [True, False], ids=['flash before', 'connect before'])
def test_if_serial_post_connect_reset_runs_after_flash_and_connect(device: HardwareAdapter, flash_before) -> None:
    config = {'method': 'serial', 'sequence': [{'rts': True}]}
    device.device_config.flash_before = flash_before
    device.device_config.post_connect_reset = config
    calls = mock.Mock()
    device._flash_and_run = calls.flash
    device.connect = calls.connect
    device.connections[0].run_post_connect_reset = calls.reset

    device._device_launch()

    expected = [mock.call.flash(), mock.call.connect()] if flash_before else \
        [mock.call.connect(), mock.call.flash()]
    assert calls.mock_calls == expected + [mock.call.reset(config)]


@mock.patch('shutil.which', return_value='west')
@mock.patch('twister_harness.device.hardware_adapter.post_connect_reset.run')
def test_if_west_post_connect_reset_uses_west_reset(patched_run, patched_which, device: HardwareAdapter) -> None:
    config = {'method': 'west'}
    device.device_config.post_connect_reset = config
    device._flash_and_run = mock.Mock()
    device.connect = mock.Mock()

    device._device_launch()

    patched_run.assert_called_once_with(
        config,
        west_reset_command=device._build_west_command('reset', device.device_config.build_dir),
    )


@mock.patch('twister_harness.device.hardware_adapter.post_connect_reset.run')
def test_if_post_connect_reset_failure_raises(patched_run, device: HardwareAdapter) -> None:
    from twisterlib.post_connect_reset import PostConnectResetError

    patched_run.side_effect = PostConnectResetError('dummy failure')
    device.device_config.post_connect_reset = {'method': 'west'}
    device._build_west_command = mock.Mock(return_value=['west', 'reset'])
    device._flash_and_run = mock.Mock()
    device.connect = mock.Mock()

    with pytest.raises(TwisterHarnessException, match='Post-connect reset failed: dummy failure'):
        device._device_launch()


@pytest.mark.parametrize(
    'flash_command, west_flash_cmd, expect_warning',
    [('', '', False), ('', 'flash', False), ('custom', '', True), ('', 'debug', True)],
    ids=['default', 'west flash', 'flash command', 'west debug'],
)
@mock.patch('twister_harness.device.hardware_adapter.post_connect_reset.run')
def test_if_post_connect_reset_warns_when_device_is_not_stopped(
    patched_run, device: HardwareAdapter, caplog, flash_command, west_flash_cmd, expect_warning
) -> None:
    device.device_config.post_connect_reset = {'method': 'west'}
    device.device_config.flash_command = flash_command
    device.device_config.west_flash_cmd = west_flash_cmd
    device._build_west_command = mock.Mock(return_value=['west', 'reset'])
    device._flash_and_run = mock.Mock()
    device.connect = mock.Mock()

    device._device_launch()

    assert ('does not keep the device stopped' in caplog.text) == expect_warning
    device._build_west_command.assert_called_once_with('reset', mock.ANY)
    patched_run.assert_called_once_with(
        device.device_config.post_connect_reset, west_reset_command=['west', 'reset']
    )


@mock.patch('twister_harness.device.device_connection.serial.Serial')
def test_if_serial_lines_are_set_before_port_is_opened(patched_serial, device: HardwareAdapter) -> None:
    device.device_config.post_connect_reset = {
        'method': 'serial',
        'open_state': {'dtr': True, 'rts': False},
        'sequence': [],
    }
    device.connections = create_device_connections(device.device_config)
    events = []
    serial_mock = mock.Mock()
    type(serial_mock).dtr = mock.PropertyMock(side_effect=lambda v: events.append(('dtr', v)))
    type(serial_mock).rts = mock.PropertyMock(side_effect=lambda v: events.append(('rts', v)))
    serial_mock.open.side_effect = lambda: events.append('open')
    patched_serial.return_value = serial_mock

    device.connections[0]._connect_device()

    patched_serial.assert_called_once()
    assert patched_serial.call_args.args == (None,)
    assert events == [('dtr', True), ('rts', False), 'open']


def test_if_open_state_is_only_given_to_main_serial_port(device: HardwareAdapter) -> None:
    device.device_config.serial_configs.append(DeviceSerialConfig(port='/dev/second'))
    device.device_config.post_connect_reset = {
        'method': 'serial',
        'open_state': {'dtr': True},
        'sequence': [],
    }
    connections = create_device_connections(device.device_config)
    assert connections[0].open_state == {'dtr': True}
    assert connections[1].open_state is None


def test_if_serial_post_connect_reset_rejects_serial_pty(device: HardwareAdapter) -> None:
    from twisterlib.post_connect_reset import PostConnectResetError

    device.device_config.serial_configs[0].serial_pty = 'script.py'
    with pytest.raises(PostConnectResetError, match='serial_pty'):
        device.connections[0].run_post_connect_reset({'method': 'serial', 'sequence': []})


def test_if_reconnect_state_follows_the_lines_after_post_connect_reset(device: HardwareAdapter) -> None:
    connection = device.connections[0]
    connection.open_state = {'dtr': True, 'rts': False}
    connection._serial_connection = mock.Mock(dtr=True, rts=False)

    def mock_run(config, ser):
        ser.dtr = False
        ser.rts = False

    with mock.patch('twister_harness.device.device_connection.post_connect_reset.run', mock_run):
        connection.run_post_connect_reset({'method': 'serial', 'sequence': []})

    assert connection.reconnect_state == {'dtr': False, 'rts': False}
    assert connection.open_state == {'dtr': True, 'rts': False}


@mock.patch('twister_harness.device.device_connection.serial.Serial')
def test_if_each_launch_opens_port_with_configured_open_state(patched_serial, device: HardwareAdapter) -> None:
    device.device_config.post_connect_reset = {
        'method': 'serial',
        'open_state': {'rts': True},
        'sequence': [{'rts': False}],
    }
    device.connections = create_device_connections(device.device_config)
    device._flash_and_run = mock.Mock()
    rts_at_open = []

    def mock_serial(*args, **kwargs):
        ser = mock.Mock(is_open=False, dtr=False, rts=False)

        def mock_open():
            rts_at_open.append(ser.rts)
            ser.is_open = True

        ser.open.side_effect = mock_open
        ser.close.side_effect = lambda: setattr(ser, 'is_open', False)
        return ser

    patched_serial.side_effect = mock_serial

    device._device_launch()
    device.disconnect()
    device.connect()
    device.disconnect()
    device._device_launch()

    assert rts_at_open == [True, False, True]
