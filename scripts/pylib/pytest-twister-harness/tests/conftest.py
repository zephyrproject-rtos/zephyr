# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import os
from pathlib import Path

import pytest


@pytest.fixture
def resources(request: pytest.FixtureRequest) -> Path:
    """Return path to `data` folder"""
    return Path(request.module.__file__).parent.joinpath('data')


@pytest.fixture
def zephyr_base() -> str:
    zephyr_base_path = os.getenv('ZEPHYR_BASE')
    if zephyr_base_path is None:
        pytest.fail('Environmental variable ZEPHYR_BASE has to be set.')
    else:
        return zephyr_base_path


@pytest.fixture
def twister_harness(zephyr_base) -> str:
    """Retrun path to pytest-twister-harness src directory"""
    pytest_twister_harness_path = str(Path(zephyr_base) / 'scripts' / 'pylib' / 'pytest-twister-harness' / 'src')
    return pytest_twister_harness_path
