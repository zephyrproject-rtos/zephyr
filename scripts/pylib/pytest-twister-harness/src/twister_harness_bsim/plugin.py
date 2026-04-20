# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

"""Pytest plugin for twister harness bsim tests. Keeps fixtures and hooks related to bsim tests."""

import logging
import os
from collections.abc import Generator

import pytest
from twister_harness.device.device_adapter import DeviceAdapter
from twister_harness_bsim.bsim_helper import BsimHelper

bsim_out_path = os.getenv("BSIM_OUT_PATH")
verbosity_level = os.getenv("BSIM_VERBOSITY_LEVEL", "2")
logger = logging.getLogger(__name__)


@pytest.fixture(scope='function')
def bsim(
    request: pytest.FixtureRequest, unlaunched_duts: list[DeviceAdapter]
) -> Generator[BsimHelper, None, None]:
    """Fixture to provide access to the BSIM helper for starting and stopping the BSIM process."""
    if not bsim_out_path:
        pytest.skip("BSIM_OUT_PATH environment variable is not set.")
    normalized_platform = unlaunched_duts[0].device_config.platform.replace('/', '_')
    simulation_id = f"{request.node.name}_{normalized_platform}"
    bsim = BsimHelper(bsim_out_path, simulation_id, unlaunched_duts, verbosity_level)
    try:
        yield bsim
    finally:
        bsim.close()
