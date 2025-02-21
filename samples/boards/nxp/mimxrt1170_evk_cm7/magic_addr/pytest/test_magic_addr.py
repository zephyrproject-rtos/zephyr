# Copyright (c) 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0

import logging

from twister_harness import DeviceAdapter

logger = logging.getLogger(__name__)


def test_magic_addr(dut: DeviceAdapter):
    '''
    tag memory twice and check result
    '''
    dut.readlines_until(regex='Cast some characters:', print_output=True)
    dut.write(str.encode('A'))
    lines = dut.readlines_until(regex='Magic DTCM address accessed', print_output=True)
    logger.info(lines)
    dut.write(str.encode('B'))
    lines = dut.readlines_until(regex='Magic DTCM address accessed', print_output=True)
    logger.info(lines)
    ret = any('Magic DTCM address accessed' in line for line in lines)
    assert ret
