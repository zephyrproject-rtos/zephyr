# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
from __future__ import annotations

import logging
from pathlib import Path

from twister_harness import DeviceAdapter

logger = logging.getLogger(__name__)


def test_required_build_dirs_found(dut: DeviceAdapter, required_build_dirs):
    logger.info(f"Required build directories: {required_build_dirs}")
    assert len(required_build_dirs) == 2, "Expected two required build directories"
    assert Path(required_build_dirs[0]).is_dir()
    assert Path(Path(required_build_dirs[0]) / 'build.log').exists()
