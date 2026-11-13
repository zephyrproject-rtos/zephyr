# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

from types import SimpleNamespace
from unittest.mock import Mock, call

import pytest
from twister_harness import fixtures
from twister_harness.device.device_adapter import DeviceAdapter
from twister_harness.exceptions import TwisterHarnessTimeoutException


@pytest.mark.parametrize('fixture_name', ['device_objects', 'unlaunched_duts'])
def test_teardown_closes_remaining_devices_after_timeout(fixture_name, monkeypatch):
    devices = [Mock(spec=DeviceAdapter), Mock(spec=DeviceAdapter)]
    close_calls = Mock()
    close_calls.attach_mock(devices[0].close, 'first')
    close_calls.attach_mock(devices[1].close, 'second')
    timeout = TwisterHarnessTimeoutException('Timed out waiting for GCOV_COVERAGE_DUMP_END')
    devices[0].close.side_effect = timeout

    if fixture_name == 'device_objects':
        config = SimpleNamespace(devices=[SimpleNamespace(type='qemu')] * len(devices))
        factory = Mock(side_effect=devices)
        monkeypatch.setattr(fixtures.DeviceFactory, 'get_device', Mock(return_value=factory))
        fixture = fixtures.device_objects.__wrapped__(config)
    else:
        request = SimpleNamespace(node=SimpleNamespace(name='test'))
        fixture = fixtures.unlaunched_duts.__wrapped__(request, devices)

    assert next(fixture) == devices
    with pytest.raises(TwisterHarnessTimeoutException) as exc_info:
        next(fixture)

    assert exc_info.value is timeout
    assert close_calls.mock_calls == [call.first(), call.second()]
