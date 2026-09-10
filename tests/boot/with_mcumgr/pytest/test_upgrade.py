# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
from __future__ import annotations

import logging
from pathlib import Path

import pytest
from twister_harness import DeviceAdapter, MCUmgr, MCUmgrBle, Shell
from twister_harness.helpers.domains_helper import get_default_domain_name
from twister_harness.helpers.shell import ShellMCUbootCommandParsed
from twister_harness.helpers.utils import find_in_config

logger = logging.getLogger(__name__)

# This string is used to verify that the device is running the application
WELCOME_STRING = "smp_sample: build time:"


def get_sign_version(app_build_dir: Path) -> str:
    version = find_in_config(
        Path(app_build_dir) / 'zephyr' / '.config',
        'CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION'
    )
    assert version is not None
    return version


def get_new_app_build_dir(
        dut: DeviceAdapter, required_build_dirs: list[str], index: int = 0
) -> Path:
    assert len(required_build_dirs), "No required application provided"
    # If `build: false` was set then first required application is used as main application
    if str(dut.device_config.current_build_dir) == required_build_dirs[0]:
        index += 1
    assert len(required_build_dirs) > index, "Not enough required applications provided"
    build_dir = Path(required_build_dirs[index])
    app_build_dir = build_dir / get_default_domain_name(build_dir / 'domains.yaml')
    assert app_build_dir.is_dir(), f"Application build dir {app_build_dir} does not exist"
    return app_build_dir


def reset_device(dut: DeviceAdapter) -> None:
    """Reset DUT using shell command."""
    dut.connect()
    dut.clear_buffer()
    command = "kernel reboot cold"
    logger.debug(f"Reset device from shell: {command}")
    dut.write(f"{command}\n".encode())


def run_upgrade(dut: DeviceAdapter, mcumgr: MCUmgr, app_build_dir: Path) -> str:
    """Run upgrade procedure: upload image, test it and reset device to trigger upgrade.

    Returns hash of the image used to test.
    """
    image_to_test = Path(app_build_dir) / 'zephyr' / 'zephyr.signed.bin'
    assert image_to_test.is_file()
    logger.info('Upload image with mcumgr')
    dut.disconnect()
    mcumgr.image_upload(image_to_test)
    logger.info('Test uploaded APP image')
    second_hash = mcumgr.get_hash_to_test()
    mcumgr.image_test(second_hash)
    reset_device(dut)
    return second_hash


def verify_upgrade_after_reset(
        dut: DeviceAdapter, shell: Shell, version: str, swap_type: str = 'test'
) -> None:
    dut.connect()
    lines = dut.readlines_until(regex=WELCOME_STRING, timeout=20)
    pytest.LineMatcher(lines).fnmatch_lines([f"*Swap type: {swap_type}*", "*Starting swap*"])
    logger.info('Verify new APP is booted')
    check_with_shell_command(shell, version)


def check_with_shell_command(shell: Shell, version: str) -> None:
    mcuboot_areas = ShellMCUbootCommandParsed.create_from_cmd_output(shell.exec_command('mcuboot'))
    assert mcuboot_areas.areas[0].version == version


def run_upgrade_with_revert(
        dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr, required_build_dirs: list[str]
) -> None:
    origin_version = get_sign_version(dut.device_config.app_build_dir)
    new_app_dir = get_new_app_build_dir(dut, required_build_dirs)
    new_version = get_sign_version(new_app_dir)

    run_upgrade(dut, mcumgr, new_app_dir)
    verify_upgrade_after_reset(dut, shell, new_version)

    logger.info('Revert images')
    reset_device(dut)
    verify_upgrade_after_reset(dut, shell, origin_version, swap_type='revert')


def test_upgrade_with_revert(
        dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr, required_build_dirs: list[str]
):
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
    run_upgrade_with_revert(dut, shell, mcumgr, required_build_dirs)


def test_upgrade_ble_with_revert(
        mcumgr_ble: MCUmgrBle, dut: DeviceAdapter, shell: Shell, required_build_dirs: list[str]
):
    """Verify that the application can be updated over BLE"""
    run_upgrade_with_revert(dut, shell, mcumgr_ble, required_build_dirs)


def test_upgrade_with_confirm(
        dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr, required_build_dirs: list[str]
):
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
    new_app_dir = get_new_app_build_dir(dut, required_build_dirs)
    new_version = get_sign_version(new_app_dir)

    second_hash = run_upgrade(dut, mcumgr, new_app_dir)
    verify_upgrade_after_reset(dut, shell, new_version)

    dut.disconnect()
    logger.info('Confirm the image')
    mcumgr.image_confirm(second_hash)
    dut.connect()
    reset_device(dut)
    lines = dut.readlines_until(regex=WELCOME_STRING, timeout=20)
    pytest.LineMatcher(lines).no_fnmatch_line("*Starting swap*")
    logger.info('Verify new APP is still booted')
    check_with_shell_command(shell, new_version)


def test_downgrade_prevention(
        dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr, required_build_dirs: list[str]
):
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
    origin_version = get_sign_version(dut.device_config.app_build_dir)
    new_app_dir = get_new_app_build_dir(dut, required_build_dirs)

    run_upgrade(dut, mcumgr, new_app_dir)

    dut.connect()
    lines = dut.readlines_until(regex=WELCOME_STRING, timeout=20)
    matcher = pytest.LineMatcher(lines)
    matcher.no_fnmatch_line("*Starting swap*")
    matcher.fnmatch_lines(["*erased due to downgrade prevention*"])

    logger.info('Verify that the original APP is booted')
    check_with_shell_command(shell, origin_version)


def test_upgrade_wrong_signature(
        dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr, required_build_dirs: list[str]
):
    """
    Verify that the application is not updated when app is not signed or signed with invalid key
    1) Device flashed with MCUboot and an application that contains SMP server
    2) Prepare an update of an application containing the SMP server that has
       been signed with a different key than MCUboot was compiled with
    3) Upload the application update to slot 1 using mcumgr
    4) Flag the application update in slot 1 as 'pending' by using mcumgr 'test'
    5) Restart the device, verify that swap is not started
    """
    origin_version = get_sign_version(dut.device_config.app_build_dir)
    new_app_dir = get_new_app_build_dir(dut, required_build_dirs)

    run_upgrade(dut, mcumgr, new_app_dir)

    dut.connect()
    lines = dut.readlines_until(regex=WELCOME_STRING, timeout=20)
    matcher = pytest.LineMatcher(lines)
    matcher.no_fnmatch_line("*Starting swap*")
    matcher.fnmatch_lines(["*Image in the secondary slot is not valid*"])

    logger.info('Verify that the original APP is booted')
    check_with_shell_command(shell, origin_version)
