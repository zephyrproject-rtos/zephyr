# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

"""Bluetooth Classic HID Device board-to-board tests.

The device under test plays the HID Device role, the second reserved board
plays the HID Host role. Every test case drives both shells and checks the
callbacks logged on both sides of the link. The board helpers and the expected
shell output are provided by ``conftest.py``.
"""

import logging

logger = logging.getLogger(__name__)


class TestHidDeviceConnection:
    """Connection establishment and service discovery."""

    def test_connect_disconnect(self, hid_link):
        """The HID Host connects to the HID Device and both sides report it."""
        hid_link.connect()
        hid_link.disconnect()

    def test_sdp_report_descriptor(self, hid_link, hid_device, hid_host):
        """The HID Host reads a non-empty report descriptor over SDP."""
        hid_host.br_connect(hid_device.addr)
        lines = hid_host.expect('hid_host sdp_discover', hid_host.DESCRIPTOR, timeout=20.0)
        descriptor = hid_host.match_lines(lines, hid_host.DESCRIPTOR)[0]
        assert ' 0 bytes' not in descriptor, f'empty report descriptor: {descriptor}'
        hid_host.br_disconnect()


class TestHidDeviceReports:
    """Report exchange over the control and the interrupt channel."""

    def test_send_input_report(self, hid_link, hid_device, hid_host):
        """An input report sent by the HID Device reaches the HID Host."""
        hid_link.connect()
        hid_device.exec_command('hid_device send 1 10 5')
        hid_host.wait_for(hid_host.INPUT_REPORT)
        hid_link.disconnect()

    def test_get_report(self, hid_link, hid_host):
        """A GET_REPORT from the HID Host is answered by the HID Device."""
        hid_link.connect()
        hid_host.expect('hid_host get_report 1 2 3', hid_host.GET_REPORT)
        hid_link.disconnect()

    def test_set_report(self, hid_link, hid_device, hid_host):
        """A SET_REPORT from the HID Host is accepted by the HID Device."""
        hid_link.connect()
        hid_host.expect('hid_host set_report 2 02 FF 00', hid_host.SET_REPORT_DONE)
        hid_device.wait_for(hid_device.SET_REPORT)
        hid_link.disconnect()

    def test_output_report(self, hid_link, hid_device, hid_host):
        """An output report from the HID Host reaches the HID Device."""
        hid_link.connect()
        hid_host.exec_command('hid_host send AA BB')
        hid_device.wait_for(hid_device.OUTPUT_REPORT)
        hid_link.disconnect()


class TestHidDeviceProtocolMode:
    """Boot and report protocol mode handling."""

    def test_default_protocol_is_report(self, hid_link, hid_host):
        """The protocol mode after connecting is the report mode."""
        hid_link.connect()
        lines = hid_host.expect('hid_host get_protocol', hid_host.GET_PROTOCOL)
        mode = hid_host.match_lines(lines, hid_host.GET_PROTOCOL)[0]
        assert 'report' in mode, f'expected the report mode, got: {mode}'
        hid_link.disconnect()

    def test_switch_protocol_mode(self, hid_link, hid_device, hid_host):
        """The HID Host switches the HID Device to boot mode and back."""
        hid_link.connect()

        hid_host.expect('hid_host set_protocol boot', hid_host.SET_PROTOCOL_DONE)
        hid_device.wait_for(hid_device.SET_PROTOCOL)
        lines = hid_host.expect('hid_host get_protocol', hid_host.GET_PROTOCOL)
        mode = hid_host.match_lines(lines, hid_host.GET_PROTOCOL)[0]
        assert 'boot' in mode, f'expected the boot mode, got: {mode}'

        hid_host.expect('hid_host set_protocol report', hid_host.SET_PROTOCOL_DONE)
        hid_device.wait_for(hid_device.SET_PROTOCOL)
        lines = hid_host.expect('hid_host get_protocol', hid_host.GET_PROTOCOL)
        mode = hid_host.match_lines(lines, hid_host.GET_PROTOCOL)[0]
        assert 'report' in mode, f'expected the report mode, got: {mode}'

        hid_link.disconnect()


class TestHidDevicePowerControl:
    """SUSPEND and EXIT_SUSPEND transactions."""

    def test_suspend(self, hid_link, hid_device, hid_host):
        """The HID Device reports the suspend state requested by the HID Host."""
        hid_link.connect()

        hid_host.expect('hid_host suspend', 'suspended')
        hid_device.wait_for(hid_device.SUSPENDED)

        hid_link.disconnect()

    def test_suspend_exit_suspend(self, hid_link, hid_device, hid_host):
        """The HID Device reports the suspend state requested by the HID Host."""
        hid_link.connect()

        hid_host.expect('hid_host suspend', 'suspended')
        hid_device.wait_for(hid_device.SUSPENDED)

        hid_host.expect('hid_host exit_suspend', 'exit_suspend')
        hid_device.wait_for(hid_device.EXIT_SUSPEND)

        hid_link.disconnect()


class TestHidDeviceVirtualCableUnplug:
    """VIRTUAL_CABLE_UNPLUG handling."""

    def test_virtual_cable_unplug(self, hid_link, hid_device, hid_host):
        """A VCU from the HID Host tears the HID session down."""
        hid_link.connect()
        hid_host.exec_command('hid_host vcu')
        hid_device.wait_for(hid_device.VC_UNPLUG)


class TestHidDeviceReconnect:
    """Reconnection after a clean disconnect."""

    def test_reconnect(self, hid_link, hid_device, hid_host):
        """A second HID session can be established and still exchanges reports."""
        hid_link.connect()
        hid_link.disconnect()

        hid_link.connect()
        hid_device.exec_command('hid_device send 0 0 0')
        hid_host.wait_for(hid_host.INPUT_REPORT)
        hid_link.disconnect()


class TestHidDeviceRegistration:
    """Registration life cycle of the HID Device."""

    def test_register_twice_is_rejected(self, hid_device):
        """Registering an already registered HID Device fails."""
        hid_device.expect_error('hid_device register', 'already registered')

    def test_register_unregister_cycle(self, hid_link, hid_device, hid_host):
        """The HID Device survives repeated register and unregister cycles."""
        for _ in range(3):
            hid_device.expect('hid_device unregister', hid_device.UNREGISTERED)
            hid_device.expect('hid_device register', hid_device.REGISTERED)

        hid_link.connect()
        hid_device.exec_command('hid_device send 0 1 1')
        hid_host.wait_for(hid_host.INPUT_REPORT)
        hid_link.disconnect()


class TestHidDeviceErrorHandling:
    """Error paths that must be rejected without breaking the stack."""

    def test_operations_without_connection(self, hid_device):
        """HID Device operations without an ACL link report 'not connected'."""
        hid_device.expect_error('hid_device connect', 'not connected')
        hid_device.expect_error('hid_device disconnect', 'not connected')
        hid_device.expect_error('hid_device send 0 0 0', 'not connected')

    def test_report_value_out_of_bounds(self, hid_link, hid_device, hid_host):
        """An out of range report value is rejected and the session survives."""
        hid_link.connect()
        hid_device.expect_error('hid_device send 0 200 0', 'out of bounds')
        hid_device.exec_command('hid_device send 0 0 0')
        hid_host.wait_for(hid_host.INPUT_REPORT)
        hid_link.disconnect()

    def test_unregister_while_connected(self, hid_link, hid_device, hid_host):
        """Unregistering during an active HID session keeps the session alive."""
        hid_link.connect()
        lines = hid_device.exec_command('hid_device unregister')
        assert not any(hid_device.UNREGISTERED in line for line in lines), (
            f'unregister should have been rejected while connected, got: {lines}'
        )
        hid_device.exec_command('hid_device send 0 0 0')
        hid_host.wait_for(hid_host.INPUT_REPORT)
        hid_link.disconnect()

    def test_get_report_error_2(self, hid_link, hid_host):
        """A GET_REPORT from the HID Host is answered by the HID Device."""
        hid_link.connect()
        hid_host.expect('hid_host get_report 1 1 3', hid_host.GET_REPORT_FAIL_2)
        hid_link.disconnect()

    def test_get_report_error_4(self, hid_link, hid_host):
        """A GET_REPORT from the HID Host is answered by the HID Device."""
        hid_link.connect()
        hid_host.expect('hid_host get_report 1 0 3', hid_host.GET_REPORT_FAIL_4)
        hid_link.disconnect()
