# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
from __future__ import annotations

import logging
from pathlib import Path

from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)


def test_shell_is_ready(shell: Shell):
    # wait_for_prompt is called from shell fixture, so if passed, shell is ready to use
    assert True


def test_second_app_is_found(dut: DeviceAdapter, shell: Shell, required_images):
    logger.info(f"Required images: {required_images}")
    assert required_images
    assert Path(required_images[0]).is_dir()
    assert Path(Path(required_images[0]) / 'build.log').exists()
