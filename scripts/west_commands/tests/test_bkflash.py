# Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import logging
import os
from types import SimpleNamespace
from unittest.mock import ANY, call, patch

import pytest
from conftest import RC_KERNEL_BIN
from runners.bkflash import SECTOR, BkflashBinaryRunner, BootRom, FlashError, pad_to_sectors
from runners.core import FileType

TEST_PORT = '/dev/ttyACM7'
PROBED_PORT = '/dev/ttyUSB1'
TEST_FILE = 'other.crc.bin'

# A legacy UART and two USB adapters, the board on the second, listed out of order.
PORTS = [
    SimpleNamespace(device=PROBED_PORT, vid=0x1A86),
    SimpleNamespace(device='/dev/ttyS0', vid=None),
    SimpleNamespace(device='/dev/ttyUSB0', vid=0x0403),
]

DENIED = 'could not open port /dev/ttyUSB0: [Errno 13] Permission denied'
SILENT = 'BootROM did not answer the link check after reset'

os_path_isfile = os.path.isfile


def os_path_isfile_patch(filename):
    if filename == RC_KERNEL_BIN:
        return True
    return os_path_isfile(filename)


@pytest.fixture
def cfg(runner_config):
    # The shared fixture fills RunnerConfig positionally without uf2_file, which
    # leaves "file" set; this runner reads it, so start from no file given.
    return runner_config._replace(file=None, file_type=FileType.OTHER)


def create(cfg, args):
    parser = argparse.ArgumentParser(allow_abbrev=False)
    BkflashBinaryRunner.add_parser(parser)
    return BkflashBinaryRunner.create(cfg, parser.parse_args(args))


def probe_patch(answering):
    def probe(port, logger):
        if port not in answering:
            raise FlashError(DENIED if port == '/dev/ttyUSB0' else SILENT)

    return probe


@patch('serial.tools.list_ports.comports', return_value=PORTS)
@patch('runners.bkflash.probe', side_effect=probe_patch([PROBED_PORT]))
@patch('runners.bkflash.flash')
def test_bkflash_init(fl, pr, cp, cfg):
    runner = BkflashBinaryRunner(cfg)
    with patch('os.path.isfile', side_effect=os_path_isfile_patch):
        runner.run('flash')
    # USB adapters in device order, stopping at the first that answers.
    assert [c.args[0] for c in pr.call_args_list] == ['/dev/ttyUSB0', PROBED_PORT]
    assert fl.call_args_list == [
        call(PROBED_PORT, RC_KERNEL_BIN, ANY, offset=0, baud=1000000, monitor_baud=None)
    ]


@patch('serial.tools.list_ports.comports', return_value=PORTS)
@patch('runners.bkflash.probe', side_effect=probe_patch([]))
@patch('runners.bkflash.flash')
def test_bkflash_no_board(fl, pr, cp, cfg):
    runner = BkflashBinaryRunner(cfg)
    with (
        patch('os.path.isfile', side_effect=os_path_isfile_patch),
        pytest.raises(RuntimeError) as err,
    ):
        runner.run('flash')
    # Each port's own reason is reported, not a guess.
    assert f'/dev/ttyUSB0: {DENIED}' in str(err.value)
    assert f'{PROBED_PORT}: {SILENT}' in str(err.value)
    # Every USB adapter is tried; the legacy UART never is.
    assert [c.args[0] for c in pr.call_args_list] == ['/dev/ttyUSB0', PROBED_PORT]
    fl.assert_not_called()


@patch('serial.tools.list_ports.comports', return_value=PORTS[1:2])
@patch('runners.bkflash.probe')
@patch('runners.bkflash.flash')
def test_bkflash_no_usb_port(fl, pr, cp, cfg):
    runner = BkflashBinaryRunner(cfg)
    with (
        patch('os.path.isfile', side_effect=os_path_isfile_patch),
        pytest.raises(RuntimeError, match='no USB serial port found'),
    ):
        runner.run('flash')
    pr.assert_not_called()
    fl.assert_not_called()


@patch('runners.bkflash.probe')
@patch('runners.bkflash.flash')
def test_bkflash_create(fl, pr, cfg):
    args = ['--port', TEST_PORT, '--baud-rate', '1500000', '--flash-offset', '0x1000']
    runner = create(cfg, args + ['--monitor', '--monitor-baud', '250000'])
    with patch('os.path.isfile', side_effect=os_path_isfile_patch):
        runner.run('flash')
    pr.assert_not_called()
    assert fl.call_args_list == [
        call(TEST_PORT, RC_KERNEL_BIN, ANY, offset=0x1000, baud=1500000, monitor_baud=250000)
    ]


@patch('runners.bkflash.flash')
def test_bkflash_file(fl, cfg):
    runner = create(cfg._replace(file=TEST_FILE, file_type=FileType.BIN), ['--port', TEST_PORT])
    runner.run('flash')
    assert fl.call_args_list == [
        call(TEST_PORT, TEST_FILE, ANY, offset=0, baud=1000000, monitor_baud=None)
    ]


@patch('runners.bkflash.flash')
def test_bkflash_file_not_bin(fl, cfg):
    runner = create(cfg._replace(file='zephyr.hex', file_type=FileType.HEX), [])
    with pytest.raises(ValueError):
        runner.run('flash')
    fl.assert_not_called()


def test_bkflash_negative_offset(cfg):
    with pytest.raises(ValueError):
        create(cfg, ['--flash-offset', '-1'])


@patch('serial.tools.list_ports.comports')
@patch('runners.bkflash.probe')
@patch('runners.bkflash.flash')
def test_bkflash_dry_run(fl, pr, cp, cfg):
    runner = create(cfg, ['--dry-run'])
    with patch('os.path.isfile', side_effect=os_path_isfile_patch):
        runner.run('flash')
    # No probing either: it resets boards.
    cp.assert_not_called()
    pr.assert_not_called()
    fl.assert_not_called()


class FakeSerial:
    '''Serial port double that returns canned bytes and records what is written.'''

    def __init__(self, data=b''):
        self.data = bytearray(data)
        self.timeout = None
        self.written = b''

    @property
    def in_waiting(self):
        return len(self.data)

    def read(self, n):
        chunk = bytes(self.data[:n])
        del self.data[:n]
        return chunk

    def write(self, frame):
        self.written += frame


def test_bkflash_recv_frames():
    # Noise, a short response to another command, then a short and a long response.
    short = bytes([0x04, 0x0E, 0x06, 0x01, 0xE0, 0xFC, 0x03, 0xAA, 0xBB])
    long = bytes([0x04, 0x0E, 0xFF, 0x01, 0xE0, 0xFC, 0xF4, 0x03, 0x00, 0x0C, 0x00, 0x1C])
    other = bytes([0x04, 0x0E, 0x05, 0x01, 0xE0, 0xFC, 0x01, 0x00])
    rom = BootRom(FakeSerial(b'boot noise\r\n' + other + short + long), logging.getLogger())
    assert rom.recv(0x03, 1.0) == b'\xaa\xbb'
    assert rom.recv_long_ok(0x0C, 1.0) == b'\x1c'
    with pytest.raises(FlashError, match='no response'):
        rom.recv(0x03, 0.01)


class FakeRom:
    '''Flash of three sectors, each filled with its own index.'''

    def read_sector(self, addr):
        return bytes([addr // SECTOR]) * SECTOR


def test_bkflash_pad_to_sectors():
    # 16 bytes at 0x1ff8 straddle sectors 1 and 2; both are kept around the image.
    start, padded = pad_to_sectors(FakeRom(), 0x1FF8, b'\xee' * 16)
    assert start == SECTOR
    assert padded == b'\x01' * (SECTOR - 8) + b'\xee' * 16 + b'\x02' * (SECTOR - 8)
