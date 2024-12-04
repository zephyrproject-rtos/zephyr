# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import logging
import pytest
from twister_harness import DeviceAdapter
import re

logger = logging.getLogger(__name__)

TESTDATA = [
    ('zephyr', 'Echo: zephyr'),
    ('console', 'Echo: console'),
]
@pytest.mark.parametrize('command,expected', TESTDATA)
def test_echo_bot(dut: DeviceAdapter, command, expected):
    timeout = 10.0
    command_ext = f'{command}\n\n'
    regex_expected = f'.*{re.escape(expected)}'
    dut.clear_buffer()
    dut.write(command_ext.encode())
    lines: list[str] = []
    lines.extend(dut.readlines_until(regex=regex_expected, timeout=timeout, print_output=True))
    assert expected in lines, 'expected response not found'
