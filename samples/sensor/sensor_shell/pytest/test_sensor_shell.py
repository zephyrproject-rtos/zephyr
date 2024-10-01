# Copyright (c) 2024 Meta Platforms
# SPDX-License-Identifier: Apache-2.0

import logging

from twister_harness import Shell

logger = logging.getLogger(__name__)


def test_sensor_shell_info(shell: Shell):
    logger.info('send "sensor info" command')

    lines = shell.exec_command('sensor info')
    assert any(['device name: sensor@0' in line for line in lines]), 'expected response not found'
    assert any(['device name: sensor@1' in line for line in lines]), 'expected response not found'

    logger.info('response is valid')


def test_sensor_shell_get(shell: Shell):
    logger.info('send "sensor get" command')

    lines = shell.exec_command('sensor get sensor@0 voltage')
    assert any(['channel type=31(voltage)' in line for line in lines]), 'expected response not found'

    lines = shell.exec_command('sensor get sensor@1 53')
    assert any(['channel type=53(gauge_state_of_health)' in line for line in lines]), 'expected response not found'

    # Channel should be the last one before 'all' (because 'all' doesn't print anything) so that the
    # for-loop in `parse_named_int()` will go through everything
    lines = shell.exec_command('sensor get sensor@0 gauge_desired_charging_current')
    assert any(['channel type=59(gauge_desired_charging_current)' in line for line in lines]), 'expected response not found'

    logger.info('response is valid')


def test_sensor_shell_attr_get(shell: Shell):
    logger.info('send "sensor attr_get" command')

    lines = shell.exec_command('sensor attr_get sensor@0 co2 sampling_frequency')
    assert any(['sensor@0(channel=co2, attr=sampling_frequency)' in line for line in lines]), 'expected response not found'

    lines = shell.exec_command('sensor attr_get sensor@1 53 3')
    assert any(['sensor@1(channel=gauge_state_of_health, attr=slope_th)' in line for line in lines]), 'expected response not found'

    logger.info('response is valid')


def test_sensor_shell_attr_set(shell: Shell):
    logger.info('send "sensor attr_set" command')

    lines = shell.exec_command('sensor attr_set sensor@0 co2 sampling_frequency 1')
    expected_line = 'sensor@0 channel=co2, attr=sampling_frequency set to value=1'
    assert any([expected_line in line for line in lines]), 'expected response not found'

    lines = shell.exec_command('sensor attr_set sensor@1 53 3 1')
    expected_line = 'sensor@1 channel=gauge_state_of_health, attr=slope_th set to value=1'
    assert any([expected_line in line for line in lines]), 'expected response not found'

    logger.info('response is valid')


def test_sensor_shell_trig(shell: Shell):
    logger.info('send "sensor trig" command')

    lines = shell.exec_command('sensor trig sensor@0 on data_ready')
    expected_line = 'Enabled trigger idx=1 data_ready on device sensor@0'
    assert any([expected_line in line for line in lines]), 'expected response not found'

    lines = shell.exec_command('sensor trig sensor@0 off data_ready')
    expected_line = 'Disabled trigger idx=1 data_ready on device sensor@0'
    assert any([expected_line in line for line in lines]), 'expected response not found'

    logger.info('response is valid')
