#!/usr/bin/env python3
#
# Copyright (c) 2023 KNS Group LLC (YADRO)
#
# SPDX-License-Identifier: Apache-2.0

import logging
import re

from twister_harness import Shell
from twister_harness import DeviceAdapter

logger = logging.getLogger(__name__)


def test_shell_perf(dut: DeviceAdapter, shell: Shell):

    shell.base_timeout=10

    logger.info('send "perf record 200 99" command')
    lines = shell.exec_command('perf record 200 99')
    assert 'Enabled perf' in lines, 'expected response not found'
    lines = dut.readlines_until(regex='.*Perf done!', print_output=True)
    logger.info('response is valid')

    logger.info('send "perf printbuf" command')
    lines = shell.exec_command('perf printbuf')
    lines = lines[1:-1]
    match = re.match(r"Perf buf length (\d+)", lines[0])
    assert match is not None, 'expected response not found'
    length = int(match.group(1))
    lines = lines[1:]
    assert length != 0, '0 length'
    assert length == len(lines), 'length dose not match with count of lines'

    i = 0
    while i < length:
        i += int(lines[i], 16) + 1
        assert i <= length, 'one of the samples is not true to size'
