# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import logging
import os
from pathlib import Path

import pytest

from twister_harness.log import configure_logging
from twister_harness.twister_harness_config import TwisterHarnessConfig

logger = logging.getLogger(__name__)

pytest_plugins = (
    'twister_harness.fixtures.dut'
)


def pytest_addoption(parser: pytest.Parser):
    twister_harness_group = parser.getgroup('Twister harness')
    twister_harness_group.addoption(
        '--twister-harness',
        action='store_true',
        default=False,
        help='Activate Twister harness plugin'
    )
    parser.addini(
        'twister_harness',
        'Activate Twister harness plugin',
        type='bool'
    )
    twister_harness_group.addoption(
        '-O',
        '--outdir',
        metavar='PATH',
        dest='output_dir',
        help='Output directory for logs. If not provided then use '
             '--build-dir path as default.'
    )
    twister_harness_group.addoption(
        '--platform',
        help='Choose specific platform'
    )
    twister_harness_group.addoption(
        '--device-type',
        choices=('native', 'qemu', 'hardware', 'unit', 'custom'),
        help='Choose type of device (hardware, qemu, etc.)'
    )
    twister_harness_group.addoption(
        '--device-serial',
        help='Serial device for accessing the board '
             '(e.g., /dev/ttyACM0)'
    )
    twister_harness_group.addoption(
        '--device-serial-baud',
        type=int,
        default=115200,
        help='Serial device baud rate (default 115200)'
    )
    twister_harness_group.addoption(
        '--runner',
        help='use the specified west runner (pyocd, nrfjprog, etc)'
    )
    twister_harness_group.addoption(
        '--device-id',
        help='ID of connected hardware device (for example 000682459367)'
    )
    twister_harness_group.addoption(
        '--device-product',
        help='Product name of connected hardware device (for example "STM32 STLink")'
    )
    twister_harness_group.addoption(
        '--device-serial-pty',
        metavar='PATH',
        help='Script for controlling pseudoterminal. '
             'E.g --device-testing --device-serial-pty=<script>'
    )
    twister_harness_group.addoption(
        '--west-flash-extra-args',
        help='Extend parameters for west flash. '
             'E.g. --west-flash-extra-args="--board-id=foobar,--erase" '
             'will translate to "west flash -- --board-id=foobar --erase"'
    )
    twister_harness_group.addoption(
        '--flashing-timeout',
        type=int,
        default=60,
        help='Set timeout for the device flash operation in seconds.'
    )
    twister_harness_group.addoption(
        '--build-dir',
        dest='build_dir',
        metavar='PATH',
        help='Directory with built application.'
    )
    twister_harness_group.addoption(
        '--binary-file',
        metavar='PATH',
        help='Path to file which should be flashed.'
    )
    twister_harness_group.addoption(
        '--pre-script',
        metavar='PATH'
    )
    twister_harness_group.addoption(
        '--post-script',
        metavar='PATH'
    )
    twister_harness_group.addoption(
        '--post-flash-script',
        metavar='PATH'
    )


def pytest_configure(config: pytest.Config):
    if config.getoption('help'):
        return

    if not (config.getoption('twister_harness') or config.getini('twister_harness')):
        return

    validate_options(config)

    if config.option.output_dir is None:
        config.option.output_dir = config.option.build_dir
    config.option.output_dir = _normalize_path(config.option.output_dir)

    # create output directory if not exists
    os.makedirs(config.option.output_dir, exist_ok=True)

    configure_logging(config)

    config.twister_harness_config = TwisterHarnessConfig.create(config)  # type: ignore


def validate_options(config: pytest.Config) -> None:
    """Verify if user provided proper options"""
    # TBD


def _normalize_path(path: str | Path) -> str:
    path = os.path.expanduser(os.path.expandvars(path))
    path = os.path.normpath(os.path.abspath(path))
    return path
