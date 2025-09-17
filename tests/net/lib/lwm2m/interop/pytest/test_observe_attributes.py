"""
Tests for Observe attributes in LwM2M
#####################################

Copyright (c) 2025 Nordic Semiconductor ASA

SPDX-License-Identifier: Apache-2.0
"""

import time

import pytest
from leshan import Leshan
from twister_harness import Shell

TEST_PMAX: float = 4
TEST_TIME_ACC: float = 0.5


def test_attribute_gt_lt_st_wrong_res_type(leshan: Leshan, endpoint: str):
    """
    Verify that gt/lt/st attributes only be assigned to numerical resources.
    """
    # Manufacturer - should fail as that's a string type
    assert leshan.write_attributes(endpoint, '3/0/0', {'gt': 4200})['status'] != 'CHANGED(204)'
    assert leshan.write_attributes(endpoint, '3/0/0', {'lt': 3000})['status'] != 'CHANGED(204)'
    assert leshan.write_attributes(endpoint, '3/0/0', {'st': 200})['status'] != 'CHANGED(204)'
    # Battery level - should be fine
    assert leshan.write_attributes(endpoint, '3/0/7', {'gt': 4200})['status'] == 'CHANGED(204)'
    assert leshan.write_attributes(endpoint, '3/0/7', {'lt': 3000})['status'] == 'CHANGED(204)'
    assert leshan.write_attributes(endpoint, '3/0/7', {'st': 200})['status'] == 'CHANGED(204)'
    leshan.remove_attributes(endpoint, '3/0/7', ['gt'])
    leshan.remove_attributes(endpoint, '3/0/7', ['lt'])
    leshan.remove_attributes(endpoint, '3/0/7', ['st'])


@pytest.mark.slow
def test_attribute_step(shell: Shell, leshan: Leshan, endpoint: str):
    """
    Verify that Step attribute is respected when sending notifications
    """
    assert leshan.write_attributes(endpoint, '3/0/7', {'st': 200})['status'] == 'CHANGED(204)'
    assert (
        leshan.write_attributes(endpoint, '3/0/7', {'pmax': TEST_PMAX})['status'] == 'CHANGED(204)'
    )
    leshan.observe(endpoint, '3/0/7')
    with leshan.get_event_stream(endpoint, timeout=30) as events:
        shell.exec_command('lwm2m write /3/0/7/0 -u32 3000')
        data = events.next_event('NOTIFICATION')
        assert data is not None
        assert data[3][0][7][0] == 3000
        # Ensure that we don't get new notification before pmax expires
        start = time.time()
        shell.exec_command('lwm2m write /3/0/7/0 -u32 3100')
        data = events.next_event('NOTIFICATION')
        assert data[3][0][7][0] == 3100
        assert (start + TEST_PMAX) < time.time() + TEST_TIME_ACC
        assert (start + TEST_PMAX) > time.time() - TEST_TIME_ACC
        # Step attribute condition should be satisfied
        start = time.time()
        shell.exec_command('lwm2m write /3/0/7/0 -u32 3500')
        data = events.next_event('NOTIFICATION')
        assert data[3][0][7][0] == 3500
        assert start <= time.time() + TEST_TIME_ACC
    leshan.cancel_observe(endpoint, '3/0/7')
    leshan.remove_attributes(endpoint, '3/0/7', ['st'])
    leshan.remove_attributes(endpoint, '3/0/7', ['pmax'])


@pytest.mark.slow
def test_attribute_greater_than(shell: Shell, leshan: Leshan, endpoint: str):
    """
    Verify that Greater Than attribute is respected when sending notifications
    """
    assert leshan.write_attributes(endpoint, '3/0/7', {'gt': 4200})['status'] == 'CHANGED(204)'
    assert (
        leshan.write_attributes(endpoint, '3/0/7', {'pmax': TEST_PMAX})['status'] == 'CHANGED(204)'
    )
    leshan.observe(endpoint, '3/0/7')
    with leshan.get_event_stream(endpoint, timeout=30) as events:
        shell.exec_command('lwm2m write /3/0/7/0 -u32 3000')
        data = events.next_event('NOTIFICATION')
        assert data is not None
        assert data[3][0][7][0] == 3000
        # Ensure that we don't get new notification before pmax expires
        start = time.time()
        shell.exec_command('lwm2m write /3/0/7/0 -u32 4100')
        data = events.next_event('NOTIFICATION')
        assert data[3][0][7][0] == 4100
        assert (start + TEST_PMAX) < time.time() + TEST_TIME_ACC
        assert (start + TEST_PMAX) > time.time() - TEST_TIME_ACC
        # Greater than attribute condition should be satisfied
        start = time.time()
        shell.exec_command('lwm2m write /3/0/7/0 -u32 4400')
        data = events.next_event('NOTIFICATION')
        assert data[3][0][7][0] == 4400
        assert start <= time.time() + TEST_TIME_ACC
        # Still above threshold - no new notification expected before pmax
        start = time.time()
        shell.exec_command('lwm2m write /3/0/7/0 -u32 4300')
        data = events.next_event('NOTIFICATION')
        assert data[3][0][7][0] == 4300
        assert (start + TEST_PMAX) < time.time() + TEST_TIME_ACC
        assert (start + TEST_PMAX) > time.time() - TEST_TIME_ACC
        # Greater than attribute condition should be satisfied again
        start = time.time()
        shell.exec_command('lwm2m write /3/0/7/0 -u32 4100')
        data = events.next_event('NOTIFICATION')
        assert data[3][0][7][0] == 4100
    leshan.cancel_observe(endpoint, '3/0/7')
    leshan.remove_attributes(endpoint, '3/0/7', ['gt'])
    leshan.remove_attributes(endpoint, '3/0/7', ['pmax'])


@pytest.mark.slow
def test_attribute_less_than(shell: Shell, leshan: Leshan, endpoint: str):
    """
    Verify that Less Than attribute is respected when sending notifications
    """
    assert leshan.write_attributes(endpoint, '3/0/7', {'gt': 3000})['status'] == 'CHANGED(204)'
    assert (
        leshan.write_attributes(endpoint, '3/0/7', {'pmax': TEST_PMAX})['status'] == 'CHANGED(204)'
    )
    leshan.observe(endpoint, '3/0/7')
    with leshan.get_event_stream(endpoint, timeout=30) as events:
        shell.exec_command('lwm2m write /3/0/7/0 -u32 3200')
        data = events.next_event('NOTIFICATION')
        assert data is not None
        assert data[3][0][7][0] == 3200
        # Ensure that we don't get new notification before pmax expires
        start = time.time()
        shell.exec_command('lwm2m write /3/0/7/0 -u32 3100')
        data = events.next_event('NOTIFICATION')
        assert data[3][0][7][0] == 3100
        assert (start + TEST_PMAX) < time.time() + TEST_TIME_ACC
        assert (start + TEST_PMAX) > time.time() - TEST_TIME_ACC
        # Less than attribute condition should be satisfied
        start = time.time()
        shell.exec_command('lwm2m write /3/0/7/0 -u32 2800')
        data = events.next_event('NOTIFICATION')
        assert data[3][0][7][0] == 2800
        assert start <= time.time() + TEST_TIME_ACC
        # Still below threshold - no new notification expected before pmax
        start = time.time()
        shell.exec_command('lwm2m write /3/0/7/0 -u32 2900')
        data = events.next_event('NOTIFICATION')
        assert data[3][0][7][0] == 2900
        assert (start + TEST_PMAX) < time.time() + TEST_TIME_ACC
        assert (start + TEST_PMAX) > time.time() - TEST_TIME_ACC
        # Less than attribute condition should be satisfied again
        start = time.time()
        shell.exec_command('lwm2m write /3/0/7/0 -u32 3100')
        data = events.next_event('NOTIFICATION')
        assert data[3][0][7][0] == 3100
    leshan.cancel_observe(endpoint, '3/0/7')
    leshan.remove_attributes(endpoint, '3/0/7', ['gt'])
    leshan.remove_attributes(endpoint, '3/0/7', ['pmax'])
