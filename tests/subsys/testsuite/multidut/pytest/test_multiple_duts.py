# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
import logging

import pytest
from twister_harness import DeviceAdapter, Shell
from twister_harness.helpers.utils import find_in_config

logger = logging.getLogger(__name__)


def test_multidut_check_history(shells: list[Shell]):
    """
    Test scenario using two DUTs flashed with shell-enabled image.

    Steps:
    1. Send a command ('date get') on the first DUT
    2. Verify the command appears in the history on the first DUT
    3. Check the history on the second DUT
    4. Validate that the command does NOT appear in the second DUT's history
    """
    assert len(shells) > 1, "This test requires more than one DUT to be configured."

    shell = shells[0]
    command = "date get"
    shell.exec_command(command)
    lines = shell.exec_command("history")
    pytest.LineMatcher(lines).fnmatch_lines(["*history", f"*{command}"])

    logger.info("Validate that the command does NOT appear in the second DUT's history")
    shell1 = shells[1]
    lines1 = shell1.exec_command("history")

    pytest.LineMatcher(lines1).fnmatch_lines(["*history"])
    pytest.LineMatcher(lines1).no_fnmatch_line(f"*{command}")


def test_multidut_different_apps(duts: list[DeviceAdapter]):
    """Test scenario using two DUTs flashed with different images.

    Steps:
    1. Observe the boot banner on both DUTs
    2. Validate that the first DUT shows the changed banner
    3. Validate that the second DUT shows the original banner
    """

    assert len(duts) > 1, "This test requires more than one DUT to be configured."
    dut = duts[0]
    dut1 = duts[1]

    banner = find_in_config(
        dut.device_config.app_build_dir / 'zephyr' / '.config', "CONFIG_BOOT_BANNER_STRING"
    )
    banner1 = find_in_config(
        dut1.device_config.app_build_dir / 'zephyr' / '.config', "CONFIG_BOOT_BANNER_STRING"
    )
    if banner == banner1:
        pytest.skip("Both DUTs have the same banner, cannot validate different applications")

    logger.info("Validate that the first DUT shows the changed banner")
    dut.readlines_until(regex=f".*{banner}")

    logger.info("Validate that the second DUT shows the original banner")
    lines1 = dut1.readlines_until(regex=f".*{banner1}")
    pytest.LineMatcher(lines1).no_fnmatch_line(f"*{banner}*")
