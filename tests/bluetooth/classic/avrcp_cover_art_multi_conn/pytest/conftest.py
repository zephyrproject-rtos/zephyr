# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=duplicate-code

"""Test fixtures and utilities for Bluetooth Classic AVRCP Cover Art testing."""

import inspect
import logging
import re
import time

import pytest
from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)

# LE address as printed by the shell, e.g. P:XX:XX:XX:XX:XX:XX or R:XX:XX:XX:XX:XX:XX
LE_ADDR_PATTERN = r'\b(?P<type>[PR]):(?P<addr>(?:[0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2})'


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
        self.avrcp_info = {}  # {bd_addr: avrcp_cover_art_addr}

    def reset(self):
        """Reset the board to its initial state."""
        command = "test_smp reboot"
        self.dut.write(command.encode())
        if not self.shell.wait_for_prompt():
            logger.error('Prompt not found')
        if self.dut.device_config.type == 'hardware':
            # after booting up the device, there might appear additional logs
            # after first prompt, so we need to wait and clear the buffer
            time.sleep(0.5)
            self.dut.clear_buffer()

    # BT test commands
    def bt_init(self, timeout=10):
        """Initialize Bluetooth on the board."""
        return self.iexpect('bt init', r'Bluetooth initialized', timeout=timeout)

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

    def _parse_le_address(self, command):
        """Run a shell command and parse the LE address it prints."""
        lines = self.exec_command(command)
        for line in lines:
            match = re.search(LE_ADDR_PATTERN, line)
            if match:
                addr_type = 'public' if match.group('type').upper() == 'P' else 'random'
                mac_address = match.group('addr')
                logger.info(f"addr:{mac_address}, type:{addr_type}")
                if addr_type == 'public':
                    self.pub_addr = mac_address
                else:
                    self.rnd_addr = mac_address
                return (mac_address, addr_type)
        pytest.fail(f'No LE address found in the output of "{command}"')

    def get_address(self):
        """Get the Bluetooth address of the board."""
        return self._parse_le_address('bt id-show')

    def clear(self):
        """Clear the board console's output."""
        return self.exec_command('clear\n')

    def avrcp_init(self):
        """avrcp initialization."""
        self.exec_command('bt auth none')
        self.exec_command('test_avrcp ct register_cb')
        self.exec_command('test_avrcp tg register_cb')
        self.exec_command('test_avrcp_cover_art ct reg')
        self.exec_command('test_avrcp_cover_art tg reg')


def _setup_board(shell: Shell, dut: DeviceAdapter) -> BluetoothBoard:
    """Bring up Bluetooth Classic and the AVRCP Cover Art servers on one board."""
    board = BluetoothBoard(shell, dut)
    board.exec_command('bt init')
    board.readlines_until(regex='Bluetooth initialized', timeout=10)
    board.get_address()
    board.bt_connectable()
    board.bt_discoverable()
    board.avrcp_init()
    return board


@pytest.fixture(scope='session')
def client(duts: list[DeviceAdapter], shells: list[Shell]) -> BluetoothBoard:
    """First board, the client side of the tests."""
    assert len(shells) >= 3, 'This test requires three DUTs to be configured.'
    logger.info('Fixture for Bluetooth client setup.')
    return _setup_board(shells[0], duts[0])


@pytest.fixture(scope='session')
def server_1(duts: list[DeviceAdapter], shells: list[Shell]) -> BluetoothBoard:
    """Second board, the first server."""
    assert len(shells) >= 3, 'This test requires three DUTs to be configured.'
    logger.info('Fixture for Bluetooth server_1 setup.')
    return _setup_board(shells[1], duts[1])


@pytest.fixture(scope='session')
def server_2(duts: list[DeviceAdapter], shells: list[Shell]) -> BluetoothBoard:
    """Third board, the second server."""
    assert len(shells) >= 3, 'This test requires three DUTs to be configured.'
    logger.info('Fixture for Bluetooth server_2 setup.')
    return _setup_board(shells[2], duts[2])
