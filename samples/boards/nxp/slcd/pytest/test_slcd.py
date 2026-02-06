# Copyright (c) 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0

import logging

from twister_harness import DeviceAdapter

logger = logging.getLogger(__name__)


def test_slcd(dut: DeviceAdapter):
    '''
    tag memory twice and check result
    '''
    dut.readlines_until(regex='NXP SLCD demo starting', print_output=True)
    lines = dut.readlines_until(regex='Counter:', print_output=True)
    logger.info(lines)
    ret = any('NXP SLCD demo running' in line for line in lines)
    assert ret
