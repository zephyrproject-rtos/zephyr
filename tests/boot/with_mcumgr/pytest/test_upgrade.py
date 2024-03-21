# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
from __future__ import annotations

import pytest
import logging

from pathlib import Path
from twister_harness import DeviceAdapter, Shell, MCUmgr
from west_sign_wrapper import west_sign_with_imgtool
from utils import (
    find_in_config,
    match_lines,
    match_no_lines,
    check_with_shell_command,
    check_with_mcumgr_command,
)

logger = logging.getLogger(__name__)
PROJECT_NAME = 'with_mcumgr'


def create_signed_image(build_dir: Path, version: str) -> Path:
    image_to_test = Path(build_dir) / 'test_{}.signed.bin'.format(
        version.replace('.', '_').replace('+', '_'))
    origin_key_file = find_in_config(
        Path(build_dir) / 'mcuboot' / 'zephyr' / '.config',
        'CONFIG_BOOT_SIGNATURE_KEY_FILE'
    )
    west_sign_with_imgtool(
        build_dir=Path(build_dir) / PROJECT_NAME,
        output_bin=image_to_test,
        key_file=Path(origin_key_file),
        version=version
    )
    assert image_to_test.is_file()
    return image_to_test


def clear_buffer(dut: DeviceAdapter) -> None:
    disconnect = False
    if not dut.is_device_connected():
        dut.connect()
        disconnect = True
    dut.clear_buffer()
    if disconnect:
        dut.disconnect()


def test_upgrade_with_confirm(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
    """
    Verify that the application can be updated
    1) Device flashed with MCUboot and an application that contains SMP server
    2) Prepare an update of an application containing the SMP server
    3) Upload the application update to slot 1 using mcumgr
    4) Flag the application update in slot 1 as 'pending' by using mcumgr 'test'
    5) Restart the device, verify that swapping process is initiated
    6) Verify that the updated application is booted
    7) Confirm the image using mcumgr
    8) Restart the device, and verify that the new application is still booted
    """
    logger.info('Prepare upgrade image')
    new_version = '0.0.2+0'
    image_to_test = create_signed_image(dut.device_config.build_dir, new_version)

    logger.info('Upload image with mcumgr')
    dut.disconnect()
    mcumgr.image_upload(image_to_test)

    logger.info('Test uploaded APP image')
    second_hash = mcumgr.get_hash_to_test()
    mcumgr.image_test(second_hash)
    clear_buffer(dut)
    mcumgr.reset_device()

    dut.connect()
    output = dut.readlines_until('Launching primary slot application')
    match_lines(output, [
        'Swap type: test',
        'Starting swap using move algorithm'
    ])
    logger.info('Verify new APP is booted')
    check_with_shell_command(shell, new_version, swap_type='test')
    dut.disconnect()
    check_with_mcumgr_command(mcumgr, new_version)

    logger.info('Confirm the image')
    mcumgr.image_confirm(second_hash)
    mcumgr.reset_device()

    dut.connect()
    output = dut.readlines_until('Launching primary slot application')
    match_no_lines(output, [
        'Starting swap using move algorithm'
    ])
    logger.info('Verify new APP is still booted')
    check_with_shell_command(shell, new_version)


def test_upgrade_with_revert(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
    """
    Verify that MCUboot will roll back an image that is not confirmed
    1) Device flashed with MCUboot and an application that contains SMP server
    2) Prepare an update of an application containing the SMP server
    3) Upload the application update to slot 1 using mcumgr
    4) Flag the application update in slot 1 as 'pending' by using mcumgr 'test'
    5) Restart the device, verify that swapping process is initiated
    6) Verify that the updated application is booted
    7) Reset the device without confirming the image
    8) Verify that MCUboot reverts update
    """
    origin_version = find_in_config(
        Path(dut.device_config.build_dir) / PROJECT_NAME / 'zephyr' / '.config',
        'CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION'
    )
    logger.info('Prepare upgrade image')
    new_version = '0.0.3+0'
    image_to_test = create_signed_image(dut.device_config.build_dir, new_version)

    logger.info('Upload image with mcumgr')
    dut.disconnect()
    mcumgr.image_upload(image_to_test)

    logger.info('Test uploaded APP image')
    second_hash = mcumgr.get_hash_to_test()
    mcumgr.image_test(second_hash)
    clear_buffer(dut)
    mcumgr.reset_device()

    dut.connect()
    output = dut.readlines_until('Launching primary slot application')
    match_lines(output, [
        'Swap type: test',
        'Starting swap using move algorithm'
    ])
    logger.info('Verify new APP is booted')
    check_with_shell_command(shell, new_version, swap_type='test')
    dut.disconnect()
    check_with_mcumgr_command(mcumgr, new_version)

    logger.info('Revert images')
    mcumgr.reset_device()

    dut.connect()
    output = dut.readlines_until('Launching primary slot application')
    match_lines(output, [
        'Swap type: revert',
        'Starting swap using move algorithm'
    ])
    logger.info('Verify that MCUboot reverts update')
    check_with_shell_command(shell, origin_version)


@pytest.mark.parametrize(
    'key_file', [None, 'root-ec-p256.pem'],
    ids=[
        'no_key',
        'invalid_key'
    ])
def test_upgrade_signature(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr, key_file):
    """
    Verify that the application is not updated when app is not signed or signed with invalid key
    1) Device flashed with MCUboot and an application that contains SMP server
    2) Prepare an update of an application containing the SMP server that has
       been signed:
       a) without any key
       b) with a different key than MCUboot was compiled with
    3) Upload the application update to slot 1 using mcumgr
    4) Flag the application update in slot 1 as 'pending' by using mcumgr 'test'
    5) Restart the device, verify that swap is not started
    """
    if key_file:
        origin_key_file = find_in_config(
            Path(dut.device_config.build_dir) / 'mcuboot' / 'zephyr' / '.config',
            'CONFIG_BOOT_SIGNATURE_KEY_FILE'
        ).strip('"\'')
        key_file = Path(origin_key_file).parent / key_file
        assert key_file.is_file()
        assert not key_file.samefile(origin_key_file)
        image_to_test = image_to_test = Path(dut.device_config.build_dir) / 'test_invalid_key.bin'
        logger.info('Sign second image with an invalid key')
    else:
        image_to_test = image_to_test = Path(dut.device_config.build_dir) / 'test_no_key.bin'
        logger.info('Sign second imagewith no key')

    west_sign_with_imgtool(
        build_dir=Path(dut.device_config.build_dir) / PROJECT_NAME,
        output_bin=image_to_test,
        key_file=key_file,
        version='0.0.3+4'  # must differ from the origin version, if not then hash is not updated
    )
    assert image_to_test.is_file()

    logger.info('Upload image with mcumgr')
    dut.disconnect()
    mcumgr.image_upload(image_to_test)

    logger.info('Test uploaded APP image')
    second_hash = mcumgr.get_hash_to_test()
    mcumgr.image_test(second_hash)

    logger.info('Verify that swap is not started')
    clear_buffer(dut)
    mcumgr.reset_device()

    dut.connect()
    output = dut.readlines_until('Launching primary slot application')
    match_no_lines(output, ['Starting swap using move algorithm'])
    match_lines(output, ['Image in the secondary slot is not valid'])
