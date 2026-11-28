#!/usr/bin/env python3
#
# Copyright (c) 2023 KNS Group LLC (YADRO)
#
# SPDX-FileCopyrightText: Copyright 2026 Arm Limited and/or its
# SPDX-FileCopyrightText: affiliates <open-source-office@arm.com>
#
# SPDX-License-Identifier: Apache-2.0

import logging
import re

from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)


def test_shell_perf(dut: DeviceAdapter, shell: Shell):

    shell.base_timeout=20

    logger.info('send "perf record 200 99" command')
    lines = shell.exec_command('perf record 200 99')
    assert 'Enabled perf' in lines, 'expected response not found'

    logger.info('verify stat cannot overlap an active record session')
    lines = shell.exec_command('perf stat start -e cpu0.cycles')
    assert any('Perf is running' in line for line in lines)

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

    logger.info('list perf counter providers and events')
    lines = shell.exec_command('perf list')
    assert any('cpu0 : CPU performance counter provider' in line for line in lines)
    lines = shell.exec_command('perf list cpu0')
    assert any('cpu0.cycles : CPU cycle counter' in line for line in lines)

    logger.info('run a perf stat session')
    lines = shell.exec_command('perf stat start -e cpu0.cycles')
    assert 'Perf stat started' in lines
    lines = shell.exec_command('perf stat stop')
    assert any('Perf buffer type: stat, version: 1' in line for line in lines)
    assert any(re.search(r'\b\d+\s+cpu0\.cycles$', line) for line in lines)

    logger.info('print and clear the retained stat result')
    lines = shell.exec_command('perf printbuf')
    assert any('Perf buffer type: stat, version: 1' in line for line in lines)
    assert any(re.search(r'\b\d+\s+cpu0\.cycles$', line) for line in lines)

    logger.info('verify record cannot overlap an active stat session')
    lines = shell.exec_command('perf stat start -e cpu0.cycles')
    assert 'Perf stat started' in lines
    lines = shell.exec_command('perf record 200 99')
    assert any('Perf is running' in line for line in lines)
    lines = shell.exec_command('perf stat stop')
    assert any('Perf buffer type: stat, version: 1' in line for line in lines)

    logger.info('verify a new recording replaces the retained stat result')
    lines = shell.exec_command('perf record 200 99')
    assert 'Enabled perf' in lines
    dut.readlines_until(regex='.*Perf done!', print_output=True)
    lines = shell.exec_command('perf printbuf')
    assert not any('Perf buffer type: stat' in line for line in lines)
    assert any(re.match(r'Perf buf length [1-9]\d*$', line) for line in lines)
