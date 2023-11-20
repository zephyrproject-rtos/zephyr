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
from leshan import Leshan
from twister_harness import Shell
from twister_harness import DeviceAdapter

logger = logging.getLogger(__name__)


#
# Test specification:
# https://www.openmobilealliance.org/release/LightweightM2M/ETS/OMA-ETS-LightweightM2M-V1_1-20190912-D.pdf
#
# Bootstrap Interface: [0-99]
#

def verify_LightweightM2M_1_1_int_0(shell: Shell, dut: DeviceAdapter):
    """LightweightM2M-1.1-int-0 - Client Initiated Bootstrap"""
    dut.readlines_until(regex='.*Bootstrap started with endpoint', timeout=5.0)
    dut.readlines_until(regex='.*Bootstrap registration done', timeout=5.0)
    dut.readlines_until(regex='.*Bootstrap data transfer done', timeout=5.0)

def test_LightweightM2M_1_1_int_1(shell: Shell, dut: DeviceAdapter, leshan: Leshan, endpoint_bootstrap: str):
    """LightweightM2M-1.1-int-1 - Client Initiated Bootstrap Full (PSK)"""
    verify_LightweightM2M_1_1_int_0(shell, dut)
    verify_LightweightM2M_1_1_int_101(shell, dut, leshan, endpoint_bootstrap)
    verify_LightweightM2M_1_1_int_401(shell, leshan, endpoint_bootstrap)

def verify_LightweightM2M_1_1_int_101(shell: Shell, dut: DeviceAdapter, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-101 - Initial Registration"""
    dut.readlines_until(regex='.*Registration Done', timeout=5.0)
    assert leshan.get(f'/clients/{endpoint}')

def verify_LightweightM2M_1_1_int_401(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-401 - UDP Channel Security - Pre-shared Key Mode"""
    lines = shell.get_filtered_output(shell.exec_command('lwm2m read 0/0/0 -s'))
    host = lines[0]
    assert 'coaps://' in host
    lines = shell.get_filtered_output(shell.exec_command('lwm2m read 0/0/2 -u8'))
    mode = int(lines[0])
    assert mode == 0
    resp = leshan.get(f'/clients/{endpoint}')
    assert resp["secure"]
