# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=duplicate-code

"""Test fixtures and utilities for Bluetooth Classic FTP testing."""

import inspect
import logging
import re
import time

import pytest
from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)


class BaseBoard:
    """Base class for board-level test functionality."""

    def __init__(self, shell, dut):
        self.shell = shell
        self.dut = dut

    # zephyr sehll APIs
    def exec_command(self, *args, **kwargs):
        """Execute a shell command and return its output."""
        return self.shell.exec_command(*args, **kwargs)

    def readlines_until(self, *args, **kwargs):
        """Read lines from device until a condition is met, with optional timeout."""
        if 'timeout' not in kwargs:
            kwargs['timeout'] = 3
        return self.dut.readlines_until(*args, **kwargs)

    def check_shell_response(self, lines, regex: list[str] | str):
        parent = inspect.stack()[1]
        module = parent.frame.f_globals['__name__']
        lineno = parent.lineno
        found = False
        dct = {}
        logger.debug('check_shell_response')

        if isinstance(regex, str):
            messages = [regex]
        else:
            messages = regex

        for message in messages:
            dct[message] = False

        for line in lines:
            logger.debug(f'{module} {lineno}: DUT response: {str(line)}')

        for message in messages:
            for line in lines:
                if re.search(message, line):
                    dct[message] = True
                    for key in dct:
                        if dct[key] is False:
                            found = False
                            break
                    else:
                        found = True
                    break

        for key in dct:
            logger.debug(f'{module} {lineno}: Expected DUT response: "{key}", Matched: {dct[key]}')

        return found

    def wait_for_shell_response(self, regex: list[str] | str, timeout=10):
        parent = inspect.stack()[1]
        module = parent.frame.f_globals['__name__']
        lineno = parent.lineno
        found = False
        lines = []
        dct = {}

        logger.debug('wait_for_shell_response')

        if isinstance(regex, str):
            messages = [regex]
        else:
            messages = regex

        for message in messages:
            dct[message] = False

        try:
            for _ in range(0, timeout):
                read_lines = self.dut.readlines(print_output=False)
                for line in read_lines:
                    logger.debug(f'{module} {lineno}: DUT response: {str(line)}')
                lines += read_lines
                for message in messages:
                    for line in read_lines:
                        if re.search(message, line):
                            dct[message] = True
                            for key in dct:
                                if dct[key] is False:
                                    found = False
                                    break
                            else:
                                found = True
                            break
                if found is True:
                    break
                time.sleep(1)
        except Exception as e:
            logger.error(f'{e}!', exc_info=True)
            raise e

        for key in dct:
            logger.debug(f'{module} {lineno}: Expected DUT response: "{key}", Matched: {dct[key]}')

        return found, lines

    def iexpect(self, command, response: list[str] | str, wait=True, timeout=10):
        """send command and  return output matching an expected pattern."""
        lines = self.shell.exec_command(command)
        if wait:
            found, lines = self.wait_for_shell_response(response, timeout=timeout)
        else:
            found = self.check_shell_response(lines, response)

        assert found is not False
        return found, lines


class BluetoothBoard(BaseBoard):
    """Board implementation with Bluetooth Classic functionality."""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.pub_addr = None
        self.rnd_addr = None
        self.addr_type = None
        self.channel = None
        self.psm = None

    # BT test commands
    def bt_init(self, timeout=10):
        """Initialize Bluetooth on the board."""
        return self.iexpect('bt init\n', r'Bluetooth initialized[\s\S]*\n', timeout=timeout)

    def bt_connectable(self):
        """Set Bluetooth device connectable."""
        lines = self.exec_command("br pscan on")
        assert any([re.search(".*connectable done.*", line) for line in lines]), (
            'fail to set connectable'
        )

    def bt_discoverable(self):
        """Set Bluetooth device discoverable."""
        lines = self.exec_command("br iscan on")
        assert any([re.search(".*discoverable done.*", line) for line in lines]), (
            'fail to set discoverable'
        )

    def get_address(self):
        """Get the Bluetooth address of the board."""
        lines = self.exec_command('bt id-show')

        if not lines:
            return
        for line in lines:
            pattern = r"(([0-9A-Fa-f]{2}:?){6})\s\((\w+)\)"
            match = re.search(pattern, line)
            if match:
                mac_address = match.group(1)  # address
                addr_type = match.group(3)  # addr type (public/random)
                logging.info(f"MAC Address: {mac_address}")
                logging.info(f"Address Type: {addr_type}")
                if addr_type == 'public':
                    self.pub_addr = mac_address
                if addr_type == 'random':
                    self.rnd_addr = mac_address
                self.addr_type = addr_type
                return (mac_address, addr_type)

    def ftp_init(self):
        """FTP server initialization."""
        self.exec_command('bt auth none')

        # Register FTP server
        lines = self.exec_command('test_ftp server register')
        pattern = r"FTP server\(s\) registered"
        found = self.check_shell_response(lines, pattern)
        assert found is True

        # Register L2CAP server
        lines = self.exec_command('test_ftp server l2cap_register')
        pattern = r"FTP L2CAP server registered, psm (?P<psm>0x\w+)"
        found = self.check_shell_response(lines, pattern)
        assert found is True

        for line in lines:
            match = re.search(pattern, line)
            if match:
                psm = match.group('psm')
                break

        # Register RFCOMM server
        lines = self.exec_command('test_ftp server rfcomm_register')
        pattern = r"FTP RFCOMM server registered, channel (?P<channel>\w+)"
        found = self.check_shell_response(lines, pattern)
        assert found is True

        for line in lines:
            match = re.search(pattern, line)
            if match:
                channel = match.group('channel')
                break

        self.channel = int(channel, 10)
        self.psm = int(psm, 16)


def _setup_board(shell: Shell, dut: DeviceAdapter) -> BluetoothBoard:
    """Bring up Bluetooth Classic and the FTP servers on one board."""
    board = BluetoothBoard(shell, dut)
    board.exec_command('bt init')
    board.readlines_until(regex='Bluetooth initialized', timeout=10)
    board.get_address()
    board.bt_connectable()
    board.bt_discoverable()
    board.ftp_init()
    return board


@pytest.fixture(scope='session')
def server(duts: list[DeviceAdapter], shells: list[Shell]) -> BluetoothBoard:
    """First board, the device under test, answering as the FTP server."""
    assert len(shells) > 1, 'This test requires two DUTs to be configured.'
    logger.info('Fixture for Bluetooth FTP server setup.')
    return _setup_board(shells[0], duts[0])


@pytest.fixture(scope='session')
def client(duts: list[DeviceAdapter], shells: list[Shell]) -> BluetoothBoard:
    """Second board, driving the FTP client side of every test."""
    assert len(shells) > 1, 'This test requires two DUTs to be configured.'
    logger.info('Fixture for Bluetooth FTP client setup.')
    return _setup_board(shells[1], duts[1])
