# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path

import pytest

# pytest_plugins = ['pytester']


@pytest.fixture
def resources(request: pytest.FixtureRequest) -> Path:
    """Return path to `data` folder"""
    return Path(request.module.__file__).parent.joinpath('data')


@pytest.fixture(scope='function')
def copy_example(pytester) -> Path:
    """Copy example tests to temporary directory and return path the temp directory."""
    resources_dir = Path(__file__).parent / 'data'
    pytester.copy_example(str(resources_dir))
    return pytester.path
