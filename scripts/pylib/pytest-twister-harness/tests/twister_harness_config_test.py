# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import textwrap
from pathlib import Path
from unittest.mock import Mock

import pytest
from twister_harness.plugin import pytest_addoption
from twister_harness.twister_harness_config import TwisterHarnessConfig


def create_mock_config_with_defaults() -> pytest.Config:
    """Create a mock config with defaults from pytest_addoption"""
    parser = pytest.Parser()
    pytest_addoption(parser)
    config = Mock(spec=pytest.Config)
    config.option = Mock()
    # Extract defaults from all parser options (including grouped ones)
    for group in parser._groups:
        for action in group.options:
            if hasattr(action, 'dest'):
                attr_name = action.dest.replace('-', '_')
                default_value = action.default if hasattr(action, 'default') else None
                setattr(config.option, attr_name, default_value)
    return config


def test_if_test_config_file_is_used(tmp_path: Path):
    content = textwrap.dedent("""
        base_timeout: 60
        device_type: hardware
        platform: sample/board/name
        required_builds: []
        west_flash_extra_args: --erase
        west_runner: ''
        duts:
        - connected: true
          flash_timeout: 60
          id: 0123456789
          platform: sample/board/name
          runner: jlink
          serial: /dev/ttyACM1
          serial_baud: 115200
          fixtures: ['ble_hci_adapter', 'usb']
          cores:
          - connected: true
            platform: unknown
            serial: /dev/ttyACM0
    """)
    twister_config = tmp_path / 'twister_config.yaml'
    twister_config.write_text(content)

    config = create_mock_config_with_defaults()
    config.option.twister_config = str(twister_config)
    twister_harness_config = TwisterHarnessConfig.create(config)

    assert len(twister_harness_config.devices) == 1
    device = twister_harness_config.devices[0]
    assert device.type == 'hardware'
    assert device.platform == 'sample/board/name'
    assert device.id == '0123456789'
    assert device.west_flash_extra_args == ['--erase']
    assert device.fixtures == ['ble_hci_adapter', 'usb']
    assert device.flash_timeout == 60
    assert device.runner == 'jlink'
    assert len(device.serial_configs) == 2
    assert device.serial_configs[0].port == '/dev/ttyACM1'
    assert device.serial_configs[0].baud == 115200
    assert device.serial_configs[1].port == '/dev/ttyACM0'
