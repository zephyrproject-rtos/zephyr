# Copyright (c) 2024 Meta Platforms
# SPDX-License-Identifier: Apache-2.0

import logging

from twister_harness import Shell

logger = logging.getLogger(__name__)


def assert_lines_has(lines, expected):
    assert any([expected in line for line in lines]), 'expected response not found'


def test_sensor_shell_info(shell: Shell):
    logger.info('send "sensor info" command')

    lines = shell.exec_command('sensor info')
    assert_lines_has(lines, 'device name: sensor@0')
    assert_lines_has(lines, 'device name: sensor@1')

    logger.info('response is valid')


def test_sensor_shell_get(shell: Shell):
    logger.info('send "sensor get" command')

    lines = shell.exec_command('sensor get sensor@0 voltage')
    assert_lines_has(lines, 'channel type=31(voltage)')

    # Currently this is expected to fail, as some of the channels do not
    # produce an output
    for channel in range(59):
        logger.info(f'channel {channel}')
        lines = shell.exec_command(f'sensor get sensor@0 {channel}')
        assert_lines_has(lines, f'channel type={channel}')

    logger.info('response is valid')


def test_sensor_shell_attr_get(shell: Shell):
    logger.info('send "sensor attr_get" command')

    lines = shell.exec_command('sensor attr_get sensor@0 co2 sampling_frequency')
    assert_lines_has(lines, 'sensor@0(channel=co2, attr=sampling_frequency)')

    lines = shell.exec_command('sensor attr_get sensor@1 53 3')
    assert_lines_has(lines, 'sensor@1(channel=gauge_time_to_empty, attr=slope_th)')

    logger.info('response is valid')


def test_sensor_shell_attr_set(shell: Shell):
    logger.info('send "sensor attr_set" command')

    lines = shell.exec_command('sensor attr_set sensor@0 co2 sampling_frequency 1')
    expected_line = 'sensor@0 channel=co2, attr=sampling_frequency set to value=1'
    assert_lines_has(lines, expected_line)

    lines = shell.exec_command('sensor attr_set sensor@1 53 3 1')
    expected_line = 'sensor@1 channel=gauge_time_to_empty, attr=slope_th set to value=1'
    assert_lines_has(lines, expected_line)

    logger.info('response is valid')


def test_sensor_shell_trig(shell: Shell):
    logger.info('send "sensor trig" command')

    lines = shell.exec_command('sensor trig sensor@0 on data_ready')
    expected_line = 'Enabled trigger idx=1 data_ready on device sensor@0'
    assert_lines_has(lines, expected_line)

    lines = shell.exec_command('sensor trig sensor@0 off data_ready')
    expected_line = 'Disabled trigger idx=1 data_ready on device sensor@0'
    assert_lines_has(lines, expected_line)

    logger.info('response is valid')
