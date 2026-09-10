"""
Various LwM2M interoperability tests
####################################

Copyright (c) 2024 Nordic Semiconductor ASA

SPDX-License-Identifier: Apache-2.0

Test specification:
===================
https://www.openmobilealliance.org/release/LightweightM2M/ETS/OMA-ETS-LightweightM2M-V1_1-20190912-D.pdf


This module contains testcases for
 * Portfolio Object (ID 16) [1600-1699]

"""

import logging
import pytest
from leshan import Leshan

from twister_harness import Shell

logger = logging.getLogger(__name__)

def test_LightweightM2M_1_1_int_1630(shell: Shell, leshan: Leshan, endpoint: str, configuration_C13: str):
    """LightweightM2M-1.1-int-1630 - Create Portfolio Object Instance"""
    resp = leshan.discover(endpoint, '16/0')
    expected_keys = ['/16/0', '/16/0/0']
    missing_keys = [key for key in expected_keys if key not in resp.keys()]
    assert len(missing_keys) == 0
    assert int(resp['/16/0/0']['dim']) == 4
    resources = {
        0: {0: 'Host Device ID #2',
            1: 'Host Device Model #2'}
    }
    assert leshan.create_obj_instance(endpoint, '16/1', resources)['status'] == 'CREATED(201)'
    resp = leshan.discover(endpoint, '16/1')
    assert int(resp['/16/1/0']['dim']) == 2
    resp = leshan.read(endpoint, '16')
    expected = {
        16: {0: {0: {0: 'Host Device ID #1',
                     1: 'Host Device Manufacturer #1',
                     2: 'Host Device Model #1',
                     3: 'Host Device Software Version #1'}},
             1: {0: {0: 'Host Device ID #2',
                     1: 'Host Device Model #2'}}}
    }
    assert resp == expected
    shell.exec_command('lwm2m delete /16/1')

# Testcase invalidates the C13 configuration, so mark it as indirect
@pytest.mark.parametrize(("configuration_C13"),[None], scope='function', indirect=['configuration_C13'])
def test_LightweightM2M_1_1_int_1635(shell: Shell, leshan: Leshan, endpoint: str, configuration_C13: str):
    """LightweightM2M-1.1-int-1635 - Delete Portfolio Object Instance"""
    resp = leshan.discover(endpoint, '16')
    expected_keys = ['/16/0', '/16/0/0']
    missing_keys = [key for key in expected_keys if key not in resp.keys()]
    assert len(missing_keys) == 0
    assert leshan.delete(endpoint, '16/0')['status'] == 'DELETED(202)'
    resp = leshan.discover(endpoint, '16')
    assert resp == {'/16' : {}}
