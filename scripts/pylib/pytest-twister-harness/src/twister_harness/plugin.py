# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import logging
import os

import pytest

from twister_harness.argument_handler import ArgumentHandler
from twister_harness.twister_harness_config import TwisterHarnessConfig

logger = logging.getLogger(__name__)

pytest_plugins = (
    'twister_harness.fixtures'
)


def pytest_addoption(parser: pytest.Parser):
    twister_harness_group = parser.getgroup('Twister harness')
    handler = ArgumentHandler(twister_harness_group.addoption)
    handler.add_common_arguments()

    parser.addini(
        'twister_harness',
        'Activate Twister harness plugin',
        type='bool'
    )


def pytest_configure(config: pytest.Config):
    if config.getoption('help'):
        return

    if not (config.getoption('twister_harness') or config.getini('twister_harness')):
        return

    options = config.option

    ArgumentHandler.sanitize_options(options)

    config.twister_harness_config = TwisterHarnessConfig.create(options)  # type: ignore
