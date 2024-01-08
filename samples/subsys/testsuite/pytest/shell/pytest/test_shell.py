# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import logging

import pytest
from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)


@pytest.fixture(scope='function')
def shell(dut: DeviceAdapter) -> Shell:
    """Return ready to use shell interface"""
    shell = Shell(dut, timeout=20.0)
    logger.info('wait for prompt')
    assert shell.wait_for_prompt()
    return shell


def test_shell_print_help(shell: Shell):
    logger.info('send "help" command')
    lines = shell.exec_command('help')
    assert 'Available commands:' in lines, 'expected response not found'
    logger.info('response is valid')


def test_shell_print_version(shell: Shell):
    logger.info('send "kernel version" command')
    lines = shell.exec_command('kernel version')
    assert any(['Zephyr version' in line for line in lines]), 'expected response not found'
    logger.info('response is valid')
