# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
from __future__ import annotations

import logging

from twister_harness import DeviceAdapter

logger = logging.getLogger(__name__)


def test_hello_world_with_platform(dut: DeviceAdapter):
    """Test that the application prints the expected message with platform information."""
    dut.readlines_until(regex=f"Hello World! {dut.device_config.platform}", timeout=5.0)
