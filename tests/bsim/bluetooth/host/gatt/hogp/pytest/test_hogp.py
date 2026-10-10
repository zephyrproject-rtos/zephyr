# Copyright 2026 Xiaomi Corporation
# SPDX-License-Identifier: Apache-2.0

"""
Dual-dongle HOGP integration test using Bumble as peer.

Setup:
  - DUT dongle 0: Runs Zephyr peripheral_hogp (HID Device)
  - DUT dongle 1: Runs Zephyr central_hogp (HID Host)
  - Or: DUT dongle + Bumble as HOGP peer

Requirements:
  - Two CSR USB dongles (hci0, hci1)
  - bumble package: pip install bumble
  - Zephyr shell or sample flashed to DUT

Usage:
  pytest test_hogp.py --dut-serial=/dev/ttyACM0 --peer=bumble
  pytest test_hogp.py --dut0-serial=/dev/ttyACM0 --dut1-serial=/dev/ttyACM1
"""

import logging
import time

import pytest

logger = logging.getLogger(__name__)

# HID Report Map: Simple 3-button mouse
MOUSE_REPORT_MAP = bytes(
    [
        0x05,
        0x01,
        0x09,
        0x02,
        0xA1,
        0x01,
        0x85,
        0x01,
        0x09,
        0x01,
        0xA1,
        0x00,
        0x05,
        0x09,
        0x19,
        0x01,
        0x29,
        0x03,
        0x15,
        0x00,
        0x25,
        0x01,
        0x95,
        0x03,
        0x75,
        0x01,
        0x81,
        0x02,
        0x95,
        0x01,
        0x75,
        0x05,
        0x81,
        0x03,
        0x05,
        0x01,
        0x09,
        0x30,
        0x09,
        0x31,
        0x15,
        0x81,
        0x25,
        0x7F,
        0x75,
        0x08,
        0x95,
        0x02,
        0x81,
        0x06,
        0xC0,
        0xC0,
    ]
)

HIDS_UUID = "00001812-0000-1000-8000-00805f9b34fb"
BAS_UUID = "0000180f-0000-1000-8000-00805f9b34fb"
DIS_UUID = "0000180a-0000-1000-8000-00805f9b34fb"


class TestHOGPDevice:
    """Test Zephyr as HOGP Device (peripheral_hogp sample)."""

    def test_device_advertises_hids(self, dut_device):
        """Verify DUT advertises with HID Service UUID."""
        # Scan and verify HIDS UUID in advertisement
        assert dut_device.is_advertising(HIDS_UUID), "Device should advertise HID Service UUID"

    def test_device_connection_and_pairing(self, dut_device, peer_host):
        """Verify connection and pairing completes."""
        peer_host.connect(dut_device.address)
        peer_host.pair()
        assert peer_host.is_connected()
        assert peer_host.is_paired()

    def test_device_hids_discovery(self, dut_device, peer_host):
        """Verify HIDS can be discovered with correct characteristics."""
        peer_host.connect(dut_device.address)
        peer_host.pair()

        services = peer_host.discover_services()
        assert HIDS_UUID in services, "HID Service not found"
        assert BAS_UUID in services, "Battery Service not found"
        assert DIS_UUID in services, "DIS not found"

    def test_device_report_map(self, dut_device, peer_host):
        """Verify Report Map matches expected descriptor."""
        peer_host.connect(dut_device.address)
        peer_host.pair()
        peer_host.discover_services()

        report_map = peer_host.read_report_map()
        assert report_map == MOUSE_REPORT_MAP, f"Report Map mismatch: got {len(report_map)} bytes"

    def test_device_input_report_notification(self, dut_device, peer_host):
        """Verify Input Report notifications are received."""
        peer_host.connect(dut_device.address)
        peer_host.pair()
        peer_host.discover_services()
        peer_host.subscribe_input_reports()

        # Wait for device to send a report
        report = peer_host.wait_for_notification(timeout=5.0)
        assert report is not None, "No Input Report received"
        assert len(report.data) == 3, f"Expected 3-byte mouse report, got {len(report.data)}"

    def test_device_set_report(self, dut_device, peer_host):
        """Verify Output Report write is accepted."""
        peer_host.connect(dut_device.address)
        peer_host.pair()
        peer_host.discover_services()

        err = peer_host.write_output_report(report_id=1, data=b'\x00\x00\x00')
        assert err == 0, f"Write Output Report failed: {err}"

    def test_device_protocol_mode(self, dut_device, peer_host):
        """Verify Protocol Mode read/write."""
        peer_host.connect(dut_device.address)
        peer_host.pair()
        peer_host.discover_services()

        mode = peer_host.read_protocol_mode()
        assert mode == 1, "Default should be Report Protocol Mode (1)"

    def test_device_suspend(self, dut_device, peer_host):
        """Verify HID Control Point Suspend/Exit Suspend."""
        peer_host.connect(dut_device.address)
        peer_host.pair()
        peer_host.discover_services()

        err = peer_host.write_ctrl_point(0x00)  # Suspend
        assert err == 0
        err = peer_host.write_ctrl_point(0x01)  # Exit Suspend
        assert err == 0


class TestHOGPHost:
    """Test Zephyr as HOGP Host (central_hogp sample)."""

    def test_host_scans_for_hid(self, dut_host):
        """Verify DUT scans for HID peripherals."""
        assert dut_host.is_scanning(), "Host should be scanning"

    def test_host_connects_to_hid_device(self, dut_host, peer_device):
        """Verify Host connects to HID Device."""
        peer_device.start_advertising(HIDS_UUID)
        # Wait for DUT to connect
        assert peer_device.wait_for_connection(timeout=10.0), "Host did not connect to HID device"

    def test_host_discovers_services(self, dut_host, peer_device):
        """Verify Host completes HOGP discovery."""
        peer_device.start_advertising(HIDS_UUID)
        peer_device.wait_for_connection(timeout=10.0)

        # Check DUT log for "HOGP connected"
        assert dut_host.wait_for_log("HOGP connected", timeout=10.0), (
            "HOGP discovery did not complete"
        )

    def test_host_receives_input_report(self, dut_host, peer_device):
        """Verify Host receives Input Report from Device."""
        peer_device.start_advertising(HIDS_UUID)
        peer_device.wait_for_connection(timeout=10.0)
        time.sleep(2)  # Wait for discovery

        # Peer sends Input Report notification
        peer_device.notify_input_report(report_id=1, data=b'\x01\x05\x0a')

        assert dut_host.wait_for_log("Input Report", timeout=5.0), (
            "Host did not receive Input Report"
        )


# --- Fixtures (to be implemented per test environment) ---


@pytest.fixture
def dut_device():
    """Fixture for DUT running peripheral_hogp."""
    raise pytest.skip("Requires hardware DUT with peripheral_hogp flashed")


@pytest.fixture
def dut_host():
    """Fixture for DUT running central_hogp."""
    raise pytest.skip("Requires hardware DUT with central_hogp flashed")


@pytest.fixture
def peer_host():
    """Fixture for Bumble-based HOGP Host peer."""
    raise pytest.skip("Requires Bumble peer setup")


@pytest.fixture
def peer_device():
    """Fixture for Bumble-based HOGP Device peer."""
    raise pytest.skip("Requires Bumble peer setup")
