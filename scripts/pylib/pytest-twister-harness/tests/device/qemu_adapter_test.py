# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import os
import sys
import threading
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from typing import Generator
from unittest.mock import patch

from unittest import mock
import pytest

from twister_harness.device.binary_adapter import QemuAdapter
from twister_harness.exceptions import TwisterHarnessException, TwisterHarnessTimeoutException
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
    device.connections[0]._write_to_device = mock.Mock()
    device.launch()
    lines = device.readlines_until(regex='Namespaces are one honking great idea')
    device.close()
    assert 'Readability counts.' in lines
    assert os.path.isfile(device.connections[0].log_path)
    with open(device.connections[0].log_path, 'r') as file:
        file_lines = [line.strip() for line in file.readlines()]
    assert file_lines[-2:] == lines[-2:]


def test_qemu_adapter_stops_process_before_closing_fifo(device: QemuAdapter) -> None:
    fifo = device.connections[0]._fifo_connection
    events = []

    def stop_process() -> None:
        events.append('process')

    def stop_reader() -> None:
        events.append('reader')

    with (
        mock.patch.object(device, '_stop_subprocess', side_effect=stop_process),
        mock.patch.object(device, 'stop_reader', side_effect=stop_reader),
        mock.patch.object(fifo, 'disconnect', side_effect=lambda: events.append('fifo')),
        mock.patch.object(fifo, 'cleanup', side_effect=lambda: events.append('cleanup')),
    ):
        device.close()

    assert events == ['process', 'reader', 'fifo', 'cleanup']


def test_if_qemu_adapter_raise_exception_due_to_no_fifo_connection(device: QemuAdapter) -> None:
    device.connections[0].timeout = 0.3
    device.command = ['sleep', '1']
    with pytest.raises(TwisterHarnessException, match='Cannot establish communication with QEMU device.'):
        device.launch()
    device.close()
    assert not os.path.exists(device.connections[0]._fifo_connection._fifo_out_path)
    assert not os.path.exists(device.connections[0]._fifo_connection._fifo_in_path)


@pytest.fixture
def coverage_device(resources, device: QemuAdapter):
    device.command = [
        sys.executable,
        str(resources / 'coverage_fifo.py'),
        str(device.device_config.build_dir / 'qemu-fifo'),
    ]
    device.launch()
    device.write(b'ready\n')
    device.readlines_until(regex='READY')
    return device


def test_close_waits_for_coverage_before_disconnecting(coverage_device):
    device = coverage_device
    connection = device.connections[0]
    device.write(b'start\n')
    device.readlines_until(regex='GCOV_COVERAGE_DUMP_START')
    waiting = threading.Event()
    wait_for_coverage = connection.wait_for_coverage

    def wait(timeout):
        waiting.set()
        return wait_for_coverage(timeout)

    with (
        patch.object(connection, 'wait_for_coverage', side_effect=wait),
        ThreadPoolExecutor(max_workers=1) as executor,
    ):
        closing = executor.submit(device.close)
        assert waiting.wait(2)
        assert not closing.done()
        assert device.is_device_connected()
        assert device._process.poll() is None
        device.write(b'end\n')
        closing.result(timeout=5)

    assert 'GCOV_COVERAGE_DUMP_END' in connection.log_path.read_text()
    assert device._process is None
    assert not device.is_reader_started()


def test_close_preserves_delayed_coverage_end_in_log(coverage_device):
    device = coverage_device
    device.write(b'delayed-end\n')
    device.readlines_until(regex='GCOV_COVERAGE_DUMP_START')
    device.close()
    assert 'GCOV_COVERAGE_DUMP_END' in device.connections[0].log_path.read_text()


def test_incomplete_coverage_reports_timeout_and_cleans_up(coverage_device, caplog):
    device = coverage_device
    connection = device.connections[0]
    process = device._process
    reader_thread = connection._reader_thread
    device.write(b'start\n')
    device.readlines_until(regex='GCOV_COVERAGE_DUMP_START')
    device.base_timeout = 0.05

    with pytest.raises(TwisterHarnessTimeoutException, match='GCOV_COVERAGE_DUMP_END'):
        device.close()

    assert str(connection.log_path) in caplog.text
    assert 'incomplete coverage' in caplog.text
    assert process.poll() is not None
    assert device._process is None
    assert not device.is_reader_started()
    assert not os.path.exists(connection._fifo_connection._fifo_out_path)
    assert not os.path.exists(connection._fifo_connection._fifo_in_path)
    reader_thread.join(timeout=1)
    assert not reader_thread.is_alive()
    device.close()  # Repeated teardown must not wait or raise again.


@pytest.mark.parametrize('completed', [False, True])
def test_close_without_pending_coverage_does_not_block(coverage_device, completed):
    device = coverage_device
    connection = device.connections[0]
    if completed:
        device.write(b'start\nend\n')
        device.readlines_until(regex='GCOV_COVERAGE_DUMP_END')
    assert connection.wait_for_coverage(0)
    with ThreadPoolExecutor(max_workers=1) as executor:
        closing = executor.submit(device.close)
        closing.result(timeout=2)
    assert device._process is None


def test_coverage_state_is_reset_when_device_is_relaunched(coverage_device):
    device = coverage_device
    connection = device.connections[0]
    device.write(b'start\n')
    device.readlines_until(regex='GCOV_COVERAGE_DUMP_START')
    device.base_timeout = 0.2
    with pytest.raises(TwisterHarnessTimeoutException, match='GCOV_COVERAGE_DUMP_END'):
        device.close()

    device.base_timeout = 5.0
    device.launch()
    device.write(b'ready\n')
    device.readlines_until(regex='READY')
    assert connection.wait_for_coverage(0)
    device.write(b'start\n')
    device.readlines_until(regex='GCOV_COVERAGE_DUMP_START')
    assert not connection.wait_for_coverage(0)
    device.write(b'end\n')
    device.readlines_until(regex='GCOV_COVERAGE_DUMP_END')


def test_each_coverage_dump_requires_its_own_end_marker(coverage_device):
    device = coverage_device
    connection = device.connections[0]
    for _ in range(2):
        device.write(b'start\n')
        device.readlines_until(regex='GCOV_COVERAGE_DUMP_START')
        assert not connection.wait_for_coverage(0)
        device.write(b'end\n')
        device.readlines_until(regex='GCOV_COVERAGE_DUMP_END')
        assert connection.wait_for_coverage(0)
