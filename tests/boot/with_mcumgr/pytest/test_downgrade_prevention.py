# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
from __future__ import annotations

import logging

from pathlib import Path
from twister_harness import DeviceAdapter, Shell, MCUmgr
from utils import (
    find_in_config,
    match_lines,
    match_no_lines,
    check_with_shell_command,
    check_with_mcumgr_command,
)
from test_upgrade import create_signed_image


logger = logging.getLogger(__name__)


def test_downgrade_prevention(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
    """
    Verify that the application is not downgraded
    1) Device flashed with MCUboot and an application that contains SMP server.
       Image version is 1.1.1+1
    2) Prepare an update of an application containing the SMP server, where
       image version is 0.0.0 (lower than version of the original app)
    3) Upload the application update to slot 1 using mcumgr
    4) Flag the application update in slot 1 as 'pending' by using mcumgr 'test'
    5) Restart the device, verify that downgrade prevention mechanism
       blocked the image swap
    6) Verify that the original application is booted (version 1.1.1)
    """
    origin_version = find_in_config(
        Path(dut.device_config.app_build_dir) / 'zephyr' / '.config',
        'CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION'
    )
    check_with_shell_command(shell, origin_version)
    assert origin_version != '0.0.0+0'

    logger.info('Prepare upgrade image with lower version')
    image_to_test = create_signed_image(dut.device_config.build_dir,
                                        dut.device_config.app_build_dir, '0.0.0+0')

    logger.info('Upload image with mcumgr')
    dut.disconnect()
    mcumgr.image_upload(image_to_test)

    logger.info('Test uploaded APP image')
    second_hash = mcumgr.get_hash_to_test()
    mcumgr.image_test(second_hash)
    mcumgr.reset_device()

    dut.connect()
    output = dut.readlines_until('Launching primary slot application')
    match_no_lines(output, ['Starting swap using move algorithm'])
    match_lines(output, ['erased due to downgrade prevention'])
    logger.info('Verify that the original APP is booted')
    check_with_shell_command(shell, origin_version)
    dut.disconnect()
    check_with_mcumgr_command(mcumgr, origin_version)
