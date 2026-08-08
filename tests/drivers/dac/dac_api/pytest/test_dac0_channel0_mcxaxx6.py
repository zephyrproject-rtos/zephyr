# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

import pytest
from twister_harness import DeviceAdapter

SUCCESS_LINE = "*PROJECT EXECUTION SUCCESSFUL*"
ZTEST_LINE = "*START - test_task_write_value*"
END_PATTERN = r"PROJECT EXECUTION SUCCESSFUL|PROJECT EXECUTION FAILED"


def test_dac_aux_output(dut: DeviceAdapter):
    if len(dut.device_config.serial_configs) < 2:
        pytest.fail(
            "AUX UART mapping is missing for this test. "
            "Expected a second serial connection at connection_index=1."
        )

    aux_lines = dut.readlines_until(
        connection_index=1,
        regex=END_PATTERN,
        timeout=30.0,
    )

    pytest.LineMatcher(aux_lines).fnmatch_lines([ZTEST_LINE, SUCCESS_LINE])
