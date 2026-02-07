"""
Tests for Async responses from LwM2M engine
###########################################

Copyright (c) 2025 Nordic Semiconductor ASA

SPDX-License-Identifier: Apache-2.0
"""

import time

import pytest
from leshan import Leshan
from twister_harness import Shell

TEST_ASYNC_READ_DELAY: float = 0.5
TEST_ASYNC_VALID_DELAY: float = 0.2
TEST_ASYNC_WRITE_DELAY: float = 0.5
TEST_ASYNC_EXEC_DELAY: float = 0.5


@pytest.mark.slow
def test_async_read_resource(shell: Shell, leshan: Leshan, endpoint: str):
    """
    Verify Read Operation on a single resource that requires asynchronous
    handling.
    """
    fmt = leshan.format
    to = leshan.timeout
    shell.exec_command('lwm2m create /3300/0')

    leshan.timeout = 600
    expected_value = 0.0
    for fmt in ['TLV', 'CBOR', 'TEXT', 'JSON', 'SENML_CBOR', 'SENML_JSON']:
        leshan.format = fmt
        # Test object increments value on each read
        expected_value += 1.0
        start = time.time()
        resp = leshan.read(endpoint, '3300/0/5700')
        # Verify read response was delayed
        assert (start + TEST_ASYNC_READ_DELAY) <= time.time()
        assert resp == expected_value
        time.sleep(TEST_ASYNC_VALID_DELAY)

    shell.exec_command('lwm2m delete /3300/0')
    leshan.format = fmt
    leshan.timeout = to


@pytest.mark.slow
def test_async_read_object_inst(shell: Shell, leshan: Leshan, endpoint: str):
    """
    Verify Read Operation on an object instance containing a resource that
    requires asynchronous handling.
    """
    fmt = leshan.format
    to = leshan.timeout
    shell.exec_command('lwm2m create /3300/0')

    leshan.timeout = 600
    expected_value = 0.0
    for fmt in ['TLV', 'JSON', 'SENML_CBOR', 'SENML_JSON']:
        leshan.format = fmt
        # Test object increments value on each read
        expected_value += 1.0
        start = time.time()
        resp = leshan.read(endpoint, '3300/0')
        # Verify read response was delayed
        assert (start + TEST_ASYNC_READ_DELAY) <= time.time()
        assert resp[0][5700] == expected_value
        # Sanity check on min/max value
        assert resp[0][5601] == 1.0
        assert resp[0][5602] == expected_value
        time.sleep(TEST_ASYNC_VALID_DELAY)

    shell.exec_command('lwm2m delete /3300/0')
    leshan.format = fmt
    leshan.timeout = to


@pytest.mark.slow
def test_async_read_composite(shell: Shell, leshan: Leshan, endpoint: str):
    """
    Verify Read-Composite Operation on a path list containing a resource that
    requires asynchronous handling.
    """
    fmt = leshan.format
    to = leshan.timeout
    shell.exec_command('lwm2m create /3300/0')

    leshan.timeout = 600
    expected_value = 0.0
    for fmt in ['SENML_CBOR', 'SENML_JSON']:
        leshan.format = fmt
        # Test object increments value on each read
        expected_value += 1.0
        start = time.time()
        resp = leshan.composite_read(endpoint, ['/3300/0/5700', '/3'])
        # Verify read response was delayed
        assert (start + TEST_ASYNC_READ_DELAY) <= time.time()
        assert len(resp.keys()) == 2
        assert resp[3] is not None
        assert resp[3300][0][5700] == expected_value
        # Sanity check on min/max value
        time.sleep(TEST_ASYNC_VALID_DELAY)

    shell.exec_command('lwm2m delete /3300/0')
    leshan.format = fmt
    leshan.timeout = to


@pytest.mark.slow
def test_async_write_resource(shell: Shell, leshan: Leshan, endpoint: str):
    """
    Verify Write Operation on a single resource that requires asynchronous
    handling.
    """
    fmt = leshan.format
    to = leshan.timeout
    shell.exec_command('lwm2m create /3300/0')

    cnt = 0
    leshan.timeout = 600
    for fmt in ['TLV', 'CBOR', 'TEXT', 'JSON', 'SENML_CBOR', 'SENML_JSON']:
        leshan.format = fmt
        cnt = cnt + 1
        expected_string = 'test_value' + str(cnt)
        start = time.time()
        assert leshan.write(endpoint, '3300/0/5750', expected_string)['status'] == 'CHANGED(204)'
        # Verify write response was delayed and the resource was actually written to
        assert (start + TEST_ASYNC_WRITE_DELAY) <= time.time()
        assert leshan.read(endpoint, '3300/0/5750') == expected_string

    shell.exec_command('lwm2m delete /3300/0')
    leshan.format = fmt
    leshan.timeout = to


@pytest.mark.slow
def test_async_write_object_inst(shell: Shell, leshan: Leshan, endpoint: str):
    """
    Verify Write Operation on an object instance containing a resource that
    requires asynchronous handling.
    """
    fmt = leshan.format
    to = leshan.timeout
    shell.exec_command('lwm2m create /3300/0')

    cnt = 0
    leshan.timeout = 600
    for fmt in ['TLV', 'JSON', 'SENML_CBOR', 'SENML_JSON']:
        leshan.format = fmt
        cnt = cnt + 1
        expected_string = 'test_value' + str(cnt)
        resources = {
            5750: expected_string,
        }
        start = time.time()
        assert leshan.update_obj_instance(endpoint, '3300/0', resources)['status'] == 'CHANGED(204)'
        # Verify write response was delayed and the resource was actually written to
        assert (start + TEST_ASYNC_WRITE_DELAY) <= time.time()
        assert leshan.read(endpoint, '3300/0/5750') == expected_string

    shell.exec_command('lwm2m delete /3300/0')
    leshan.format = fmt
    leshan.timeout = to


@pytest.mark.slow
def test_async_write_composite(shell: Shell, leshan: Leshan, endpoint: str):
    """
    Verify Write-Composite Operation on a path list containing a resource that
    requires asynchronous handling.
    """
    fmt = leshan.format
    to = leshan.timeout
    shell.exec_command('lwm2m create /3300/0')

    cnt = 0
    leshan.timeout = 600
    for fmt in ['SENML_CBOR', 'SENML_JSON']:
        leshan.format = fmt
        cnt = cnt + 1
        expected_string = 'test_value' + str(cnt)
        resources = {
            "/1/0/2": 102,
            "3300/0/5750": expected_string,
        }
        start = time.time()
        assert leshan.composite_write(endpoint, resources)['status'] == 'CHANGED(204)'
        # Verify write response was delayed and the resource was actually written to
        assert (start + TEST_ASYNC_WRITE_DELAY) <= time.time()
        assert leshan.read(endpoint, '1/0/2') == 102
        assert leshan.read(endpoint, '3300/0/5750') == expected_string

        # Return to default
        shell.exec_command('lwm2m write /1/0/2 -u32 1')

    shell.exec_command('lwm2m delete /3300/0')
    leshan.format = fmt
    leshan.timeout = to


@pytest.mark.slow
def test_async_exec(shell: Shell, leshan: Leshan, endpoint: str):
    """
    Verify Execute Operation that requires asynchronous handling.
    """
    to = leshan.timeout
    shell.exec_command('lwm2m create /3300/0')

    # Read sensor value twice so that min/max values differ
    assert leshan.read(endpoint, '3300/0/5700') == 1.0
    time.sleep(TEST_ASYNC_VALID_DELAY)
    assert leshan.read(endpoint, '3300/0/5700') == 2.0
    assert leshan.read(endpoint, '3300/0/5601') == 1.0
    assert leshan.read(endpoint, '3300/0/5602') == 2.0

    start = time.time()
    assert leshan.execute(endpoint, '3300/0/5605')['status'] == 'CHANGED(204)'
    # Verify exec response was delayed and min/max value were actually reset
    assert (start + TEST_ASYNC_EXEC_DELAY) <= time.time()
    assert leshan.read(endpoint, '3300/0/5601') == 2.0
    assert leshan.read(endpoint, '3300/0/5602') == 2.0

    shell.exec_command('lwm2m delete /3300/0')
    leshan.timeout = to
