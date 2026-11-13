# Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import subprocess
import sys
from types import SimpleNamespace
from unittest.mock import patch

import pytest
from conftest import RC_KERNEL_BIN

from runners.bkflash import BKFIL, BkflashBinaryRunner
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


def bkfil(port, *args):
    return [sys.executable, BKFIL, '--port', port, *args]


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
    '''Stand in for check_output running "bkfil.py probe" on each port.'''

    def check_output(cmd, **kwargs):
        port = cmd[cmd.index('--port') + 1]
        if port in answering:
            return b'chip id 0x72360001\n'
        why = DENIED if port == '/dev/ttyUSB0' else SILENT
        raise subprocess.CalledProcessError(1, cmd, output=f'trace\n{why}\n'.encode())

    return check_output


def probed_ports(check_output):
    return [c.args[0][c.args[0].index('--port') + 1] for c in check_output.call_args_list]


@patch('serial.tools.list_ports.comports', return_value=PORTS)
@patch('runners.bkflash.BkflashBinaryRunner.check_output', side_effect=probe_patch([PROBED_PORT]))
@patch('runners.bkflash.BkflashBinaryRunner.check_call')
def test_bkflash_init(cc, co, cp, cfg):
    runner = BkflashBinaryRunner(cfg)
    with patch('os.path.isfile', side_effect=os_path_isfile_patch):
        runner.run('flash')
    # USB adapters in device order, stopping at the first that answers.
    assert probed_ports(co) == ['/dev/ttyUSB0', PROBED_PORT]
    assert co.call_args.args[0] == bkfil(PROBED_PORT, 'probe')
    assert cc.call_args.args[0] == bkfil(PROBED_PORT, 'write', RC_KERNEL_BIN)


@patch('serial.tools.list_ports.comports', return_value=PORTS)
@patch('runners.bkflash.BkflashBinaryRunner.check_output', side_effect=probe_patch([]))
@patch('runners.bkflash.BkflashBinaryRunner.check_call')
def test_bkflash_no_board(cc, co, cp, cfg):
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
    assert probed_ports(co) == ['/dev/ttyUSB0', PROBED_PORT]
    cc.assert_not_called()


@patch('serial.tools.list_ports.comports', return_value=PORTS[1:2])
@patch('runners.bkflash.BkflashBinaryRunner.check_output')
@patch('runners.bkflash.BkflashBinaryRunner.check_call')
def test_bkflash_no_usb_port(cc, co, cp, cfg):
    runner = BkflashBinaryRunner(cfg)
    with (
        patch('os.path.isfile', side_effect=os_path_isfile_patch),
        pytest.raises(RuntimeError, match='no USB serial port found'),
    ):
        runner.run('flash')
    co.assert_not_called()
    cc.assert_not_called()


@patch('runners.bkflash.BkflashBinaryRunner.check_output')
@patch('runners.bkflash.BkflashBinaryRunner.check_call')
def test_bkflash_create(cc, co, cfg):
    args = ['--port', TEST_PORT, '--baud-rate', '1500000', '--flash-offset', '0x1000']
    runner = create(cfg, args + ['--monitor', '--monitor-baud', '250000'])
    with patch('os.path.isfile', side_effect=os_path_isfile_patch):
        runner.run('flash')
    co.assert_not_called()
    assert cc.call_args.args[0] == bkfil(
        TEST_PORT,
        'write',
        RC_KERNEL_BIN,
        '--offset',
        '0x1000',
        '--baud',
        '1500000',
        '--monitor',
        '250000',
    )


@patch('runners.bkflash.BkflashBinaryRunner.check_call')
def test_bkflash_file(cc, cfg):
    runner = create(cfg._replace(file=TEST_FILE, file_type=FileType.BIN), ['--port', TEST_PORT])
    runner.run('flash')
    assert cc.call_args.args[0] == bkfil(TEST_PORT, 'write', TEST_FILE)


@patch('runners.bkflash.BkflashBinaryRunner.check_call')
def test_bkflash_file_not_bin(cc, cfg):
    runner = create(cfg._replace(file='zephyr.hex', file_type=FileType.HEX), [])
    with pytest.raises(ValueError):
        runner.run('flash')
    cc.assert_not_called()


@patch('serial.tools.list_ports.comports')
@patch('runners.core.subprocess.check_output')
@patch('runners.core.subprocess.check_call')
def test_bkflash_dry_run(cc, co, cp, cfg):
    runner = create(cfg, ['--dry-run'])
    with patch('os.path.isfile', side_effect=os_path_isfile_patch):
        runner.run('flash')
    # No probing either: it resets boards.
    cp.assert_not_called()
    co.assert_not_called()
    cc.assert_not_called()
