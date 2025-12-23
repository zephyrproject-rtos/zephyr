# Copyright (c) 2024 Meta Platforms
# SPDX-License-Identifier: Apache-2.0

import logging

from twister_harness import Shell

logger = logging.getLogger(__name__)


def test_sensor_shell_info(shell: Shell):
    logger.info('send "sensor info" command')

    shell.wait_for_prompt()
    lines = shell.exec_command('sensor info')
    assert any(['device name: sensor@0' in line for line in lines]), 'expected response not found'
    assert any(['device name: sensor@1' in line for line in lines]), 'expected response not found'

    logger.info('response is valid')


def test_sensor_shell_get(shell: Shell):
    logger.info('get "sensor get" command count')

    lines = shell.exec_command('sensor get sensor@0')
    channel_count = int(lines[-2].split("=")[1].split("(")[0]) + 1
    logger.info(f'channel count: [{channel_count}]')

    logger.info('send "sensor get" command')
    for channel in range(channel_count):
        logger.info(f'channel {channel}')
        shell.wait_for_prompt()
        lines = shell.exec_command(f'sensor get sensor@0 {channel}')
        assert any([f'channel type={channel}' in line for line in lines]), 'expected response not found'

    logger.info('response is valid')


def test_sensor_shell_attr_get(shell: Shell):
    logger.info('send "sensor attr_get" command')

    shell.wait_for_prompt()
    lines = shell.exec_command('sensor attr_get sensor@0 co2 sampling_frequency')
    assert any(['sensor@0(channel=co2, attr=sampling_frequency)' in line for line in lines]), 'expected response not found'

    shell.wait_for_prompt()
    lines = shell.exec_command('sensor attr_get sensor@1 gauge_state_of_health 3')
    assert any(['sensor@1(channel=gauge_state_of_health, attr=slope_th)' in line for line in lines]), 'expected response not found'

    logger.info('response is valid')


def test_sensor_shell_attr_set(shell: Shell):
    logger.info('send "sensor attr_set" command')

    shell.wait_for_prompt()
    lines = shell.exec_command('sensor attr_set sensor@0 co2 sampling_frequency 1')
    expected_line = 'sensor@0 channel=co2, attr=sampling_frequency set to value=1'
    assert any([expected_line in line for line in lines]), 'expected response not found'

    shell.wait_for_prompt()
    lines = shell.exec_command('sensor attr_set sensor@1 gauge_state_of_health 3 1')
    expected_line = 'sensor@1 channel=gauge_state_of_health, attr=slope_th set to value=1'
    assert any([expected_line in line for line in lines]), 'expected response not found'

    logger.info('response is valid')


def test_sensor_shell_trig(shell: Shell):
    logger.info('send "sensor trig" command')

    shell.wait_for_prompt()
    lines = shell.exec_command('sensor trig sensor@0 on data_ready')
    expected_line = 'Enabled trigger idx=1 data_ready on device sensor@0'
    assert any([expected_line in line for line in lines]), 'expected response not found'

    shell.wait_for_prompt()
    lines = shell.exec_command('sensor trig sensor@0 off data_ready')
    expected_line = 'Disabled trigger idx=1 data_ready on device sensor@0'
    assert any([expected_line in line for line in lines]), 'expected response not found'

    logger.info('response is valid')
