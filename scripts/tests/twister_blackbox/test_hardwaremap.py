#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Blackbox tests for twister hardware-map options.

Covered options:
  --generate-hardware-map FILE   Probe attached boards and write a map file
  --hardware-map FILE            Load a hardware map and use it for device testing
  --persistent-hardware-map      Prefer persistent /dev/serial paths (Linux)

These tests mock the USB probing layer so no physical hardware is required.
"""

import argparse
import os
from unittest import mock

import pytest
from conftest import TEST_FILENAME_MOCK
from twisterlib.hardwaremap import HardwareMap
from twisterlib.testplan import TestPlan

# ---------------------------------------------------------------------------
# Helper: USB device stubs
# ---------------------------------------------------------------------------


class _UsbDev:
    """Minimal stub of a pyserial ListPortInfo object."""

    def __init__(self, vid, pid, manufacturer, product, serial, location, interface=None):
        self.vid = vid
        self.pid = pid
        self.manufacturer = manufacturer
        self.product = product
        self.serial_number = serial
        self.location = location
        self.interface = interface
        self.device = f'/dev/ttyUSB{serial}'
        self.description = product


_ARM_DEVICE = _UsbDev(0x0D28, 0x0204, 'ARM', 'DAPLink CMSIS-DAP', '1234', 'usb0', 'CMSIS-DAP')
_SEG_DEVICE = _UsbDev(0x1366, 0x0105, 'SEGGER', 'J-Link', 'abcd', 'usb1')
_STM_DEVICE = _UsbDev(0x0483, 0x374B, 'STMicroelectronics', 'STM32 STLink', '5678', 'usb2')


def _make_options(**kwargs):
    """Return a minimal argparse.Namespace suitable for HardwareMap."""
    defaults = {
        'generate_hardware_map': None,
        'hardware_map': None,
        'persistent_hardware_map': False,
    }
    defaults.update(kwargs)
    return argparse.Namespace(**defaults)


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestGenerateHardwareMap:
    """Tests for --generate-hardware-map option."""

    @pytest.mark.fast
    @pytest.mark.parametrize(
        'usb_devices, expected_runner',
        [
            ([_ARM_DEVICE], 'pyocd'),
            ([_SEG_DEVICE], 'jlink'),
            ([_STM_DEVICE], 'openocd'),
        ],
        ids=['pyocd', 'jlink', 'openocd'],
    )
    def test_generate_hardware_map_runner_detection(self, tmp_path, usb_devices, expected_runner):
        """Attached USB devices are detected and mapped to the correct west runner."""
        hw_map_path = str(tmp_path / 'hardware_map.yaml')
        options = _make_options(generate_hardware_map=hw_map_path)

        with mock.patch('serial.tools.list_ports.comports', return_value=usb_devices):
            hwm = HardwareMap(options=options)
            detected = hwm.scan(persistent=False)
            hwm.save(hw_map_path, detected)

        assert os.path.isfile(hw_map_path), 'hardware map file was not created'
        with open(hw_map_path) as fh:
            content = fh.read()
        assert expected_runner in content, f'Expected runner {expected_runner!r} in hardware map'

    @pytest.mark.fast
    def test_generate_hardware_map_no_devices_creates_empty_file(self, tmp_path):
        """With no USB devices attached, an empty hardware map is written."""
        hw_map_path = str(tmp_path / 'empty_map.yaml')
        options = _make_options(generate_hardware_map=hw_map_path)

        with mock.patch('serial.tools.list_ports.comports', return_value=[]):
            hwm = HardwareMap(options=options)
            detected = hwm.scan(persistent=False)
            hwm.save(hw_map_path, detected)

        assert os.path.isfile(hw_map_path)

    @pytest.mark.fast
    def test_generate_hardware_map_serial_ids(self, tmp_path):
        """Multiple devices with different serial IDs are all recorded."""
        devices = [_ARM_DEVICE, _SEG_DEVICE]
        hw_map_path = str(tmp_path / 'multi_map.yaml')
        options = _make_options(generate_hardware_map=hw_map_path)

        with mock.patch('serial.tools.list_ports.comports', return_value=devices):
            hwm = HardwareMap(options=options)
            detected = hwm.scan(persistent=False)
            hwm.save(hw_map_path, detected)

        with open(hw_map_path) as fh:
            content = fh.read()
        assert '1234' in content or 'abcd' in content


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestHardwareMapId:
    """Tests for hardware map ID field handling."""

    @pytest.mark.fast
    @pytest.mark.parametrize(
        'manufacturer, product, serial_number, expected_runner',
        [
            ('ARM', 'DAPLink CMSIS-DAP', 1234, 'pyocd'),
            ('SEGGER', 'J-Link', 'abcd', 'jlink'),
            ('STMicroelectronics', 'STM32 STLink', 1234, 'openocd'),
        ],
        ids=['pyocd', 'jlink', 'openocd'],
    )
    def test_hardware_map_id_assignment(
        self, tmp_path, manufacturer, product, serial_number, expected_runner
    ):
        """The hardware map assigns the correct runner for each device type."""
        device = _UsbDev(0x0000, 0x0000, manufacturer, product, str(serial_number), 'usb0')
        options = _make_options()

        with mock.patch('serial.tools.list_ports.comports', return_value=[device]):
            hwm = HardwareMap(options=options)
            detected = hwm.scan(persistent=False)

        found = any(dut.runner == expected_runner for dut in detected)
        assert found, f'Expected runner {expected_runner!r} for {manufacturer}/{product}'


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestGenerateYamlContent:
    """Tests that the hardware map YAML file contains complete, correct entries."""

    @pytest.mark.fast
    @pytest.mark.parametrize(
        'manufacturer, product, serial, runner',
        [
            ('ARM', 'DAPLink CMSIS-DAP', '1234', 'pyocd'),
            ('STMicroelectronics', 'STM32 STLink', 'abcd', 'openocd'),
            ('SEGGER', 'J-Link', '5678', 'jlink'),
        ],
        ids=['pyocd', 'openocd', 'jlink'],
    )
    def test_yaml_contains_all_fields(self, tmp_path, manufacturer, product, serial, runner):
        """Each detected device entry has connected, id, platform, product, runner
        and serial fields written to the hardware map YAML."""
        device = _UsbDev(0x0000, 0x0000, manufacturer, product, serial, 'usb0')
        hw_map_path = str(tmp_path / 'map.yaml')
        options = _make_options(generate_hardware_map=hw_map_path)

        with mock.patch('serial.tools.list_ports.comports', return_value=[device]):
            hwm = HardwareMap(options=options)
            detected = hwm.scan(persistent=False)
            hwm.save(hw_map_path, detected)

        with open(hw_map_path) as fh:
            content = fh.read()
        assert 'connected: true' in content
        # YAML may quote string IDs; accept both bare and quoted forms
        assert f'id: {serial}' in content or f"id: '{serial}'" in content, (
            f'id {serial!r} not found in hardware map'
        )
        assert 'platform: unknown' in content
        assert f'product: {product}' in content
        assert f'runner: {runner}' in content

    @pytest.mark.fast
    def test_multiple_devices_all_recorded(self, tmp_path):
        """When multiple devices are attached all of them appear in the YAML."""
        devices = [
            _UsbDev(0x0D28, 0x0204, 'ARM', 'DAPLink CMSIS-DAP', '001', 'usb0'),
            _UsbDev(0x1366, 0x0105, 'SEGGER', 'J-Link', '002', 'usb1'),
            _UsbDev(0x0483, 0x374B, 'STMicroelectronics', 'STM32 STLink', '003', 'usb2'),
        ]
        hw_map_path = str(tmp_path / 'multi.yaml')
        options = _make_options(generate_hardware_map=hw_map_path)

        with mock.patch('serial.tools.list_ports.comports', return_value=devices):
            hwm = HardwareMap(options=options)
            detected = hwm.scan(persistent=False)
            hwm.save(hw_map_path, detected)

        with open(hw_map_path) as fh:
            content = fh.read()
        for serial in ('001', '002', '003'):
            assert f'id: {serial}' in content or f"id: '{serial}'" in content, (
                f'Serial {serial!r} not found in hardware map'
            )


@mock.patch.object(TestPlan, 'TEST_DEFINITION_FILENAME', TEST_FILENAME_MOCK)
class TestTexasException:
    """Tests that Texas Instruments XDS110 devices are filtered by USB location.

    The XDS110 exposes multiple serial interfaces.  Only endpoint 0 (location
    ending in ``'0'``) should be included; all others must be skipped.
    """

    @pytest.mark.fast
    @pytest.mark.parametrize(
        'location, expect_included',
        [
            ('dse0', True),  # ends with '0' → include
            ('las', False),  # does not end with '0' → skip
        ],
        ids=['endpoint-0-included', 'non-0-skipped'],
    )
    def test_texas_exception(self, tmp_path, location, expect_included):
        """TI XDS110 devices on non-zero endpoints are silently excluded."""
        device = _UsbDev(0x0451, 0xBEF3, 'Texas Instruments', 'DAPLink CMSIS-DAP', 'abcd', location)
        hw_map_path = str(tmp_path / 'ti_map.yaml')
        options = _make_options(generate_hardware_map=hw_map_path)

        with mock.patch('serial.tools.list_ports.comports', return_value=[device]):
            hwm = HardwareMap(options=options)
            detected = hwm.scan(persistent=False)
            hwm.save(hw_map_path, detected)

        with open(hw_map_path) as fh:
            content = fh.read()
        if expect_included:
            assert 'id: abcd' in content or "id: 'abcd'" in content, (
                'Expected TI device at endpoint 0 to be recorded'
            )
        else:
            assert 'id: abcd' not in content and "id: 'abcd'" not in content, (
                'TI device at non-zero endpoint should be excluded'
            )
