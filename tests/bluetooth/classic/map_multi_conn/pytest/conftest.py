# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=duplicate-code

"""Test fixtures and utilities for Bluetooth Classic MAP testing."""

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
        self.addr_type = None
        self.mas_conn = {}  # {addr: {instance_id: mas_conn, }}
        self.mns_conn = {}  # {addr: mns_conn}
        # MAS server list: [{'psm': 0x1001, 'instance_id': 0,
        #                    'supported_features': 0x0000001F, 'channel': 0x06}]
        self.mas_server = []
        # MNS server dict: {'psm': 0x1001, 'supported_features': 0x0000001F, 'channel': 0x06}
        self.mns_server = {}

    def reset(self):
        """Reset the board to its initial state."""
        self.board.reset()

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
                self.addr_type = addr_type
                return (mac_address, addr_type)
        pytest.fail(f'No LE address found in the output of "{command}"')

    def get_address(self):
        """Get the Bluetooth address of the board."""
        return self._parse_le_address('bt id-show')

    def clear(self):
        """Clear the board console's output."""
        return self.exec_command('clear\n')

    def map_init(self):
        """map initialization."""
        self.exec_command('bt auth none')

        # Register MSE MAS server instances
        lines = self.exec_command('test_map mse mas register')
        assert self.check_shell_response(lines, 'MSE MAS server.s. registered')

        # Register MSE MAS L2CAP server
        lines = self.exec_command('test_map mse mas l2cap_register')
        pattern = "Register L2CAP server done"
        found = self.check_shell_response(lines, pattern)
        if not found:
            found, new_lines = self.wait_for_shell_response(pattern)
            lines += new_lines
            assert found is True

        pattern = (
            r"L2CAP server \(psm (?P<psm>\w+), mas_instance_id (?P<instance_id>\w+), "
            r"map_supported_features (?P<supported_features>\w+)\) is registered"
        )
        found = self.check_shell_response(lines, pattern)
        assert found is True

        for line in lines:
            match = re.search(pattern, line)
            if match:
                server_info = {
                    'psm': int(match.group('psm'), 16),
                    'instance_id': int(match.group('instance_id'), 10),
                    'supported_features': int(match.group('supported_features'), 16),
                }
                self.mas_server.append(server_info)

        # Register MSE MAS RFCOMM server
        lines = self.exec_command('test_map mse mas rfcomm_register')
        pattern = "Register RFCOMM server done"
        found = self.check_shell_response(lines, pattern)
        if not found:
            found, new_lines = self.wait_for_shell_response(pattern)
            lines += new_lines
            assert found is True

        pattern = (
            r"RFCOMM server \(channel (?P<channel>\w+), mas_instance_id (?P<instance_id>\w+), "
            r"map_supported_features (?P<supported_features>\w+)\) is registered"
        )
        found = self.check_shell_response(lines, pattern)
        assert found is True

        for line in lines:
            match = re.search(pattern, line)
            if match:
                channel = int(match.group('channel'), 16)
                instance_id = int(match.group('instance_id'), 10)
                supported_features = int(match.group('supported_features'), 16)
                for server_info in self.mas_server:
                    if server_info['instance_id'] == instance_id:
                        server_info['channel'] = channel
                        assert server_info['supported_features'] == supported_features
                        break

        # Register MCE MNS server instances
        lines = self.exec_command('test_map mce mns register')
        assert self.check_shell_response(lines, 'MCE MNS server.s. registered')

        # Register MCE MNS L2CAP server
        lines = self.exec_command('test_map mce mns l2cap_register')
        pattern = (
            r"L2CAP server \(psm (?P<psm>\w+), "
            r"map_supported_features (?P<supported_features>\w+)\) is registered"
        )
        found = self.check_shell_response(lines, pattern)
        if not found:
            found, new_lines = self.wait_for_shell_response(pattern)
            lines += new_lines
            assert found is True

        for line in lines:
            match = re.search(pattern, line)
            if match:
                self.mns_server['psm'] = int(match.group('psm'), 16)
                self.mns_server['supported_features'] = int(match.group('supported_features'), 16)
                break

        # Register MCE MNS RFCOMM server
        lines = self.exec_command('test_map mce mns rfcomm_register')
        pattern = (
            r"RFCOMM server \(channel (?P<channel>\w+), "
            r"map_supported_features (?P<supported_features>\w+)\) is registered"
        )
        found = self.check_shell_response(lines, pattern)
        if not found:
            found, new_lines = self.wait_for_shell_response(pattern)
            lines += new_lines
            assert found is True

        for line in lines:
            match = re.search(pattern, line)
            if match:
                self.mns_server['channel'] = int(match.group('channel'), 16)
                supported_features = int(match.group('supported_features'), 16)
                assert self.mns_server['supported_features'] == supported_features
                break


def _setup_board(shell: Shell, dut: DeviceAdapter) -> BluetoothBoard:
    """Bring up Bluetooth Classic and the MAP servers on one board."""
    board = BluetoothBoard(shell, dut)
    board.exec_command('bt init')
    board.readlines_until(regex='Bluetooth initialized', timeout=10)
    board.get_address()
    board.bt_connectable()
    board.bt_discoverable()
    board.map_init()
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
