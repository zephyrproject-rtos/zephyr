# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=duplicate-code

"""Fixtures for the Bluetooth Classic HID Host board-to-board test suite.

The suite runs on two boards reserved by Twister through ``required_devices``.
Both boards run the same image, which enables the HID Host and the HID Device
shell modules (``CONFIG_BT_HID_HOST`` and ``CONFIG_BT_HID_DEVICE``), so the role
each board plays is decided at run time by the commands the test drives on it.
The device under test (``shells[0]``) plays the HID Host role and the second
reserved board (``shells[1]``) plays the HID Device role.

The expected strings carried by the board classes are the ones printed by the
in-tree shell modules ``subsys/bluetooth/host/classic/shell/hid_host.c`` and
``hid_device.c``.
"""

import logging
import re

import pytest
from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)


class BluetoothBoard:
    """Drive one board of the HID board-to-board pair over its shell."""

    def __init__(self, shell: Shell, name: str):
        self.shell = shell
        # The asynchronous HID callbacks are printed outside of a command
        # response, so the raw device output has to be read directly.
        self.dut: DeviceAdapter = shell._device
        self.name = name
        self.addr: str | None = None
        self.addr_type: str | None = None

    def exec_command(self, command: str, timeout: float = 10.0) -> list[str]:
        """Execute a shell command and return its output lines."""
        logger.info('%s: %s', self.name, command)
        return self.shell.exec_command(command, timeout=timeout)

    def wait_for(self, text: str, timeout: float = 10.0) -> list[str]:
        """Read device output until a line contains ``text``."""
        logger.info('%s: wait for %r', self.name, text)
        return self.dut.readlines_until(regex=f'.*{re.escape(text)}.*', timeout=timeout)

    def expect(self, command: str, text: str, timeout: float = 10.0) -> list[str]:
        """Execute a command and require ``text`` in its output or afterwards."""
        lines = self.exec_command(command, timeout=timeout)
        if any(text in line for line in lines):
            return lines
        return lines + self.wait_for(text, timeout=timeout)

    def expect_error(self, command: str, text: str, timeout: float = 10.0) -> list[str]:
        """Execute a command that must fail with ``text`` in its own output."""
        lines = self.exec_command(command, timeout=timeout)
        assert any(text in line for line in lines), (
            f'{self.name}: expected {text!r} in the output of {command!r}, got: {lines}'
        )
        return lines

    def match_lines(self, lines: list[str], text: str) -> list[str]:
        """Return the output lines that contain ``text``, at least one required."""
        matched = [line for line in lines if text in line]
        assert matched, f'{self.name}: no line containing {text!r} in: {lines}'
        return matched

    def bt_init(self, timeout: float = 20.0) -> list[str]:
        """Initialize the Bluetooth stack."""
        return self.expect('bt init', 'Bluetooth initialized', timeout=timeout)

    def get_address(self):
        """Get the Bluetooth address of the board.
        Handles new ``bt id-show`` output with a leading type letter.
        Stores address in legacy (self.addr, self.addr_type) and explicit
        attributes (self.pub_addr / self.rnd_addr)."""
        lines = self.exec_command('bt id-show')

        if not lines:
            return
        for line in lines:
            # New format: "P:AA:BB:CC:DD:EE:FF (public)" or "R:11:22:33:44:55:66 (random)"
            pattern = r".*:\s*([PR]):((?:[0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2})"
            match = re.search(pattern, line)
            if match:
                type_letter = match.group(1)
                mac_address = match.group(2)
                addr_type = "public" if type_letter == "P" else "random"
                self.addr, self.addr_type = mac_address, addr_type
                if addr_type == "public":
                    self.pub_addr = mac_address
                else:
                    self.rnd_addr = mac_address
                logger.info("BT address detected (%s)", addr_type)
                return (mac_address, addr_type)
        raise AssertionError(f'{self.name}: failed to read the identity address: {lines}')

    def br_discoverable(self, enable: bool = True) -> list[str]:
        """Enable or disable inquiry scan."""
        return self.exec_command(f'br iscan {"on" if enable else "off"}')

    def br_connectable(self, enable: bool = True) -> list[str]:
        """Enable or disable page scan."""
        return self.exec_command(f'br pscan {"on" if enable else "off"}')

    def br_connect(self, addr: str, timeout: float = 20.0) -> list[str]:
        """Create a BR/EDR ACL connection to ``addr``."""
        return self.expect(f'br connect {addr}', 'Connected:', timeout=timeout)

    def br_disconnect(self, timeout: float = 10.0) -> list[str]:
        """Tear down the ACL connection."""
        return self.expect('bt disconnect', 'Disconnected:', timeout=timeout)


class HidHostBoard(BluetoothBoard):
    """Board driving the HID Host shell, with its expected output."""

    REGISTERED = 'registered'
    UNREGISTERED = 'unregistered'
    CONNECTED = 'HID Host: connected'
    DISCONNECTED = 'HID Host: disconnected'
    DESCRIPTOR = 'HID descriptor'
    INPUT_REPORT = 'HID Host: input_report'
    GET_REPORT = 'HID Host: get_report type'
    GET_REPORT_FAIL_2 = 'HID Host: get_report failed (0x02)'
    GET_REPORT_FAIL_4 = 'HID Host: get_report failed (0x04)'
    SET_REPORT_DONE = 'HID Host: set_report done'
    GET_PROTOCOL = 'HID Host: get_protocol'
    SET_PROTOCOL_DONE = 'HID Host: set_protocol done'


class HidDeviceBoard(BluetoothBoard):
    """Board driving the HID Device shell, with its expected output."""

    REGISTERED = 'HID: registered'
    UNREGISTERED = 'HID: unregistered'
    CONNECTED = 'HID: connected'
    DISCONNECTED = 'HID: disconnected'
    SET_REPORT = 'HID: set report'
    SET_PROTOCOL = 'HID: set protocol'
    OUTPUT_REPORT = 'HID: output report'
    VC_UNPLUG = 'HID: virtual cable unplug'
    SUSPENDED = 'HID: suspended'
    EXIT_SUSPEND = 'HID: exit suspend'


class HidLink:
    """Set up and tear down the HID session between the two boards."""

    def __init__(self, host: HidHostBoard, device: HidDeviceBoard):
        self.host = host
        self.device = device

    def connect(self) -> None:
        """Bring up an ACL link and the HID control and interrupt channels."""
        assert self.device.addr is not None, 'HID Device address not resolved'
        self.host.br_connect(self.device.addr)
        self.host.expect('hid_host sdp_discover', self.host.DESCRIPTOR, timeout=20.0)
        self.host.expect('hid_host connect', self.host.CONNECTED, timeout=20.0)
        self.device.wait_for(self.device.CONNECTED, timeout=20.0)

    def disconnect(self) -> None:
        """Close the HID session and the underlying ACL link."""
        self.host.expect('hid_host disconnect', self.host.DISCONNECTED)
        self.device.wait_for(self.device.DISCONNECTED)
        self.host.br_disconnect()


@pytest.fixture(name='hid_link', scope='session')
def fixture_hid_link(shells: list[Shell]):
    """Initialize both boards and register the HID Host and HID Device roles."""
    assert len(shells) > 1, 'This test requires two DUTs to be configured.'

    host = HidHostBoard(shells[0], 'hid_host')
    device = HidDeviceBoard(shells[1], 'hid_device')

    for board in (host, device):
        board.bt_init()
        board.get_address()

    device.br_connectable(True)
    device.br_discoverable(True)
    device.expect('hid_device register', device.REGISTERED)

    host.expect('hid_host register', host.REGISTERED)

    yield HidLink(host, device)

    # Return both boards to the idle state, ignoring already idle errors.
    host.exec_command('hid_host disconnect')
    host.exec_command('bt disconnect')


@pytest.fixture(name='hid_host')
def fixture_hid_host(hid_link: HidLink) -> HidHostBoard:
    """Board playing the HID Host role."""
    return hid_link.host


@pytest.fixture(name='hid_device')
def fixture_hid_device(hid_link: HidLink) -> HidDeviceBoard:
    """Board playing the HID Device role."""
    return hid_link.device


@pytest.fixture(autouse=True)
def cleanup_after_test(hid_link: HidLink):
    yield
    hid_link.host.exec_command('bt disconnect')
