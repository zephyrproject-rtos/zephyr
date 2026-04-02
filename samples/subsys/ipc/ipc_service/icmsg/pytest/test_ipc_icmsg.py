# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import pytest
from twister_harness import DeviceAdapter


def test_ipc_icmsg(dut: DeviceAdapter):
    """Test IPC ICMSG sample application."""
    expected_lines = [
        "*IPC-service * demo started*",
        "*Ep bounded*",
        "*Perform sends for*",
        "*Sent*",
        "*Received*",
        "*IPC-service * demo ended*",
    ]

    # check output from the application core
    lines_from_app = dut.readlines_until(regex="demo ended")
    pytest.LineMatcher(lines_from_app).fnmatch_lines(expected_lines)

    # check output from the remote core (skip for non-hardware devices, e.g. bsim)
    if dut.device_config.type == "hardware":
        if len(dut.connections) < 2:
            pytest.skip("Only one UART connection configured for this device")
        lines_from_remote = dut.readlines_until(connection_index=1, regex="demo ended")
        pytest.LineMatcher(lines_from_remote).fnmatch_lines(expected_lines)
