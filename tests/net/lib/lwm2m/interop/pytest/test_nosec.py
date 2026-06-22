"""
Tests for No-security mode
##########################

Copyright (c) 2023 Nordic Semiconductor ASA

SPDX-License-Identifier: Apache-2.0

Test specification:
===================
https://www.openmobilealliance.org/release/LightweightM2M/ETS/OMA-ETS-LightweightM2M-V1_1-20190912-D.pdf


This module contains only testcases that are able to run on non-secure mode.
"""

import time
import logging
from leshan import Leshan

from twister_harness import Shell
from twister_harness import DeviceAdapter

logger = logging.getLogger(__name__)

def test_LightweightM2M_1_1_int_101(shell: Shell, dut: DeviceAdapter, leshan: Leshan, endpoint_nosec: str):
    """
    Verify that the client is registered.
    """
    logger.info("LightweightM2M-1.1-int-101 - Initial Registration")
    assert leshan.get(f'/clients/{endpoint_nosec}')

def test_LightweightM2M_1_1_int_105(shell: Shell, dut: DeviceAdapter, leshan: Leshan, endpoint_nosec: str, helperclient: object):
    """
    Run testcase LightweightM2M-1.1-int-105 - Discarded Register Update
    """
    logger.info("LightweightM2M-1.1-int-105 - Discarded Register Update")
    status = leshan.get(f'/clients/{endpoint_nosec}')
    if status["secure"]:
        logger.debug("Skip, requires non-secure connection")
        return
    regid = status["registrationId"]
    assert regid
    # Fake unregister message
    helperclient.delete(f'rd/{regid}', timeout=0.1)
    helperclient.stop()
    time.sleep(1)
    shell.exec_command('lwm2m update')
    dut.readlines_until(regex=r'.*Failed with code 4\.4', timeout=5.0)
    dut.readlines_until(regex='.*Registration Done', timeout=10.0)
