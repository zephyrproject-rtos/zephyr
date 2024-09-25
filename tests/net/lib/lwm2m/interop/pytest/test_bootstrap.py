"""
LwM2M Bootstrap interface tests
###############################

Copyright (c) 2023 Nordic Semiconductor ASA

SPDX-License-Identifier: Apache-2.0

Test specification:
===================
https://www.openmobilealliance.org/release/LightweightM2M/ETS/OMA-ETS-LightweightM2M-V1_1-20190912-D.pdf


This module contains only testcases that verify the bootstrap.
"""

import logging
import pytest
from leshan import Leshan
from twister_harness import Shell
from twister_harness import DeviceAdapter
from conftest import Endpoint

logger = logging.getLogger(__name__)


#
# Test specification:
# https://www.openmobilealliance.org/release/LightweightM2M/ETS/OMA-ETS-LightweightM2M-V1_1-20190912-D.pdf
#
# Bootstrap Interface: [0-99]
#

def verify_LightweightM2M_1_1_int_0(dut: DeviceAdapter, endpoint_bootstrap: Endpoint):
    """LightweightM2M-1.1-int-0 - Client Initiated Bootstrap"""
    dut.readlines_until(regex='.*Bootstrap transfer complete', timeout=5.0)
    endpoint_bootstrap.bootstrap = True

def test_LightweightM2M_1_1_int_1(shell: Shell, dut: DeviceAdapter, leshan: Leshan, endpoint_bootstrap: Endpoint):
    """LightweightM2M-1.1-int-1 - Client Initiated Bootstrap Full (PSK)"""
    verify_LightweightM2M_1_1_int_0(dut, endpoint_bootstrap)
    verify_LightweightM2M_1_1_int_101(shell, dut, leshan, endpoint_bootstrap)
    verify_LightweightM2M_1_1_int_401(shell, leshan, endpoint_bootstrap)

def test_LightweightM2M_1_1_int_4(shell: Shell, dut: DeviceAdapter, leshan: Leshan, endpoint: Endpoint):
    """LightweightM2M-1.1-int-4 - Bootstrap Delete"""
    shell.exec_command('lwm2m create 1/2')
    shell.exec_command('lwm2m read 1/2/0')
    retval = int(shell.get_filtered_output(shell.exec_command('retval'))[0])
    assert retval == 0
    leshan.execute(endpoint, '1/0/9')
    dut.readlines_until(regex='.*Registration Done', timeout=5.0)
    shell.exec_command('lwm2m read 1/2/0')
    retval = int(shell.get_filtered_output(shell.exec_command('retval'))[0])
    assert retval < 0
    logger.info('retval: %s', retval)

def test_LightweightM2M_1_1_int_5(dut: DeviceAdapter, leshan: Leshan, endpoint: Endpoint):
    """LightweightM2M-1.1-int-5 - Server Initiated Bootstrap"""
    leshan.execute(endpoint, '1/0/9')
    dut.readlines_until(regex='.*Server Initiated Bootstrap', timeout=1)
    dut.readlines_until(regex='.*Bootstrap transfer complete', timeout=5.0)
    dut.readlines_until(regex='.*Registration Done', timeout=5.0)

def test_LightweightM2M_1_1_int_6(shell: Shell, dut: DeviceAdapter, endpoint: Endpoint):
    """LightweightM2M-1.1-int-6 - Bootstrap Sequence"""
    shell.exec_command('lwm2m stop')
    dut.readlines_until(regex=r'.*Deregistration success', timeout=5)
    shell.exec_command(f'lwm2m start {endpoint}')
    lines = dut.readlines_until(regex='.*Registration Done', timeout=5.0)
    assert not any("Bootstrap" in line for line in lines)
    shell.exec_command('lwm2m stop')
    dut.readlines_until(regex=r'.*Deregistration success', timeout=5)
    shell.exec_command("lwm2m delete 1/0")
    shell.exec_command("lwm2m delete 0/1")
    shell.exec_command(f'lwm2m start {endpoint}')
    lines = dut.readlines_until(regex='.*Registration Done', timeout=5.0)
    assert any("Bootstrap" in line for line in lines)

@pytest.mark.slow
def test_LightweightM2M_1_1_int_7(shell: Shell, dut: DeviceAdapter, endpoint: Endpoint):
    """LightweightM2M-1.1-int-7 - Fallback to bootstrap"""
    shell.exec_command('lwm2m stop')
    dut.readlines_until(regex=r'.*Deregistration success', timeout=5)
    shell.exec_command('lwm2m write 0/1/0 -s coaps://10.10.10.10:5684')
    shell.exec_command(f'lwm2m start {endpoint}')
    lines = dut.readlines_until(regex='.*Registration Done', timeout=600.0)
    assert any("Bootstrap" in line for line in lines)

def verify_LightweightM2M_1_1_int_101(shell: Shell, dut: DeviceAdapter, leshan: Leshan, endpoint: Endpoint):
    """LightweightM2M-1.1-int-101 - Initial Registration"""
    dut.readlines_until(regex='.*Registration Done', timeout=5.0)
    assert leshan.get(f'/clients/{endpoint}')
    endpoint.registered = True

def verify_LightweightM2M_1_1_int_401(shell: Shell, leshan: Leshan, endpoint: Endpoint):
    """LightweightM2M-1.1-int-401 - UDP Channel Security - Pre-shared Key Mode"""
    lines = shell.get_filtered_output(shell.exec_command('lwm2m read 0/0/0 -s'))
    host = lines[0]
    assert 'coaps://' in host
    lines = shell.get_filtered_output(shell.exec_command('lwm2m read 0/0/2 -u8'))
    mode = int(lines[0])
    assert mode == 0
    resp = leshan.get(f'/clients/{endpoint}')
    assert resp["secure"]
