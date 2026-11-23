# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

"""Bluetooth Classic HID Host board-to-board tests.

The device under test plays the HID Host role, the second reserved board plays
the HID Device role. The cases mirror the hid_device suite but are written from
the HID Host point of view, so the device under test drives every HID Host
transaction. The board helpers and the expected shell output are provided by
``conftest.py``.
"""

import logging
import time

logger = logging.getLogger(__name__)


class TestHidHostConnection:
    """Connection establishment and service discovery."""

    def test_connect_disconnect(self, hid_link):
        """The HID Host connects to the HID Device and both sides report it."""
        hid_link.connect()
        hid_link.disconnect()

    def test_sdp_report_descriptor(self, hid_link, hid_host, hid_device):
        """The HID Host reads a non-empty report descriptor over SDP."""
        hid_host.br_connect(hid_device.addr)
        lines = hid_host.expect('hid_host sdp_discover', hid_host.DESCRIPTOR, timeout=20.0)
        descriptor = hid_host.match_lines(lines, hid_host.DESCRIPTOR)[0]
        assert ' 0 bytes' not in descriptor, f'empty report descriptor: {descriptor}'
        hid_host.br_disconnect()


class TestHidHostInputReport:
    def test_receive_input_report(self, hid_link, hid_host, hid_device):
        """An input report sent by the HID Device reaches the HID Host."""
        hid_link.connect()
        hid_device.exec_command('hid_device send 1 10 5')
        hid_host.wait_for(hid_host.INPUT_REPORT)
        hid_link.disconnect()


class TestHidHostReports:
    """Report exchange over the control and the interrupt channel."""

    def test_get_report(self, hid_link, hid_host):
        """A GET_REPORT from the HID Host is answered by the HID Device."""
        hid_link.connect()
        hid_host.expect('hid_host get_report 1 2 3', hid_host.GET_REPORT)
        hid_link.disconnect()

    def test_set_report(self, hid_link, hid_host, hid_device):
        """A SET_REPORT from the HID Host is accepted by the HID Device."""
        hid_link.connect()
        hid_host.expect('hid_host set_report 2 02 FF 00', hid_host.SET_REPORT_DONE)
        hid_device.wait_for(hid_device.SET_REPORT)
        hid_link.disconnect()


class TestHidHostProtocol:
    def test_get_protocol_default(self, hid_link, hid_host, hid_device):
        """Default protocol = report (1)."""
        hid_link.connect()
        hid_host.expect("hid_host get_protocol", "report")
        hid_link.disconnect()

    def test_set_protocol_boot(self, hid_link, hid_host, hid_device):
        """Switch to boot mode."""
        hid_link.connect()
        hid_host.expect("hid_host set_protocol boot", hid_host.SET_PROTOCOL_DONE)
        hid_device.wait_for(hid_device.SET_PROTOCOL)
        hid_host.expect("hid_host get_protocol", "boot")
        hid_link.disconnect()

    def test_set_protocol_report(self, hid_link, hid_host, hid_device):
        """Switch back to report mode."""
        hid_link.connect()
        hid_host.expect("hid_host set_protocol report", hid_host.SET_PROTOCOL_DONE)
        hid_host.expect("hid_host get_protocol", "report")
        hid_link.disconnect()


class TestHidHostOutputReport:
    def test_send_output_report(self, hid_link, hid_host, hid_device):
        """Host sends output report; device output_report callback fires."""
        hid_link.connect()
        hid_host.exec_command("hid_host send AA BB")
        hid_device.wait_for(hid_device.OUTPUT_REPORT)
        hid_link.disconnect()


class TestHidHostProtocolMode:
    """Boot and report protocol mode handling."""

    def test_default_protocol_is_report(self, hid_link, hid_host):
        """The protocol mode after connecting is the report mode."""
        hid_link.connect()
        lines = hid_host.expect('hid_host get_protocol', hid_host.GET_PROTOCOL)
        mode = hid_host.match_lines(lines, hid_host.GET_PROTOCOL)[0]
        assert 'report' in mode, f'expected the report mode, got: {mode}'
        hid_link.disconnect()

    def test_switch_protocol_mode(self, hid_link, hid_host, hid_device):
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


class TestHidHostSuspend:
    """SUSPEND and EXIT_SUSPEND transactions."""

    def test_suspend(self, hid_link, hid_host, hid_device):
        """Host sends SUSPEND; device reports suspended."""
        hid_link.connect()

        hid_host.expect('hid_host suspend', 'suspended')
        hid_device.wait_for(hid_device.SUSPENDED)

        hid_link.disconnect()

    def test_suspend_exit_suspend(self, hid_link, hid_host, hid_device):
        """The HID Host suspends and resumes the HID Device."""
        hid_link.connect()

        hid_host.expect('hid_host suspend', 'suspended')
        hid_device.wait_for(hid_device.SUSPENDED)

        hid_host.expect('hid_host exit_suspend', 'exit_suspend')
        hid_device.wait_for(hid_device.EXIT_SUSPEND)

        hid_link.disconnect()


class TestHidHostVirtualCableUnplug:
    """VIRTUAL_CABLE_UNPLUG handling."""

    def test_virtual_cable_unplug(self, hid_link, hid_host, hid_device):
        """A VCU sent by the HID Host tears the HID session down."""
        hid_link.connect()
        hid_host.exec_command('hid_host vcu')
        hid_device.wait_for(hid_device.VC_UNPLUG)


class TestHidHostReconnect:
    """Reconnection after a clean disconnect."""

    def test_reconnect(self, hid_link, hid_host, hid_device):
        """A second HID session can be established and still exchanges reports."""
        time.sleep(3)
        hid_link.connect()
        hid_link.disconnect()
        time.sleep(3)
        hid_link.connect()
        hid_device.exec_command('hid_device send 0 0 0')
        hid_host.wait_for(hid_host.INPUT_REPORT)
        hid_link.disconnect()


class TestHidHostRegistration:
    """Registration life cycle of the HID Host."""

    def test_register_twice_is_rejected(self, hid_host):
        """Registering an already registered HID Host fails."""
        hid_host.expect_error('hid_host register', 'already registered')

    def test_register_unregister_cycle(self, hid_link, hid_host):
        """The HID Host survives repeated register and unregister cycles."""
        for _ in range(3):
            hid_host.expect('hid_host unregister', hid_host.UNREGISTERED)
            hid_host.expect('hid_host register', hid_host.REGISTERED)

        hid_link.connect()
        hid_host.expect('hid_host get_protocol', hid_host.GET_PROTOCOL)
        hid_link.disconnect()


class TestHidHostErrorHandling:
    """Error paths that must be rejected without breaking the stack."""

    def test_report_operations_without_session(self, hid_host):
        """Report operations without a HID session report 'not connected'."""
        hid_host.expect_error('hid_host get_report 1 2 3', 'not connected')
        hid_host.expect_error('hid_host set_report 2 02 FF', 'not connected')
        hid_host.expect_error('hid_host send AA', 'not connected')

    def test_protocol_operations_without_session(self, hid_host):
        """Protocol operations without a HID session report 'not connected'."""
        hid_host.expect_error('hid_host get_protocol', 'not connected')
        hid_host.expect_error('hid_host set_protocol boot', 'not connected')

    def test_get_report_invalid_type(self, hid_link, hid_host):
        """An out of range GET_REPORT type is rejected and the session survives."""
        hid_link.connect()
        hid_host.expect_error('hid_host get_report 0 2 3', 'parameter out of range')
        hid_host.expect('hid_host get_report 1 2 3', hid_host.GET_REPORT)
        hid_link.disconnect()

    def test_set_report_invalid_type(self, hid_link, hid_host):
        """An invalid SET_REPORT type is rejected and the session survives."""
        hid_link.connect()
        hid_host.expect_error('hid_host set_report 0 02 FF', 'invalid report type')
        hid_host.expect('hid_host set_report 2 02 FF 00', hid_host.SET_REPORT_DONE)
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
