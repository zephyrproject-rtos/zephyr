# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import logging
import os

import pytest

from twister_harness.twister_harness_config import TwisterHarnessConfig

logger = logging.getLogger(__name__)

pytest_plugins = (
    'twister_harness.fixtures'
)


def pytest_addoption(parser: pytest.Parser):
    twister_harness_group = parser.getgroup('Twister harness')
    twister_harness_group.addoption(
        '--twister-harness',
        action='store_true',
        default=False,
        help='Activate Twister harness plugin.'
    )
    parser.addini(
        'twister_harness',
        'Activate Twister harness plugin',
        type='bool'
    )
    twister_harness_group.addoption(
        '--base-timeout',
        type=float,
        default=60.0,
        help='Set base timeout (in seconds) used during monitoring if some '
             'operations are finished in a finite amount of time (e.g. waiting '
             'for flashing).'
    )
    twister_harness_group.addoption(
        '--build-dir',
        metavar='PATH',
        help='Directory with built application.'
    )
    twister_harness_group.addoption(
        '--device-type',
        choices=('native', 'qemu', 'hardware', 'unit', 'custom'),
        help='Choose type of device (hardware, qemu, etc.).'
    )
    twister_harness_group.addoption(
        '--platform',
        help='Name of used platform (qemu_x86, nrf52840dk/nrf52840, etc.).'
    )
    twister_harness_group.addoption(
        '--device-serial',
        help='Serial device for accessing the board (e.g., /dev/ttyACM0).'
    )
    twister_harness_group.addoption(
        '--device-serial-baud',
        type=int,
        default=115200,
        help='Serial device baud rate (default 115200).'
    )
    twister_harness_group.addoption(
        '--runner',
        help='Use the specified west runner (pyocd, nrfjprog, etc.).'
    )
    twister_harness_group.addoption(
        '--runner-params',
        action='append',
        help='Use the specified west runner params.'
    )
    twister_harness_group.addoption(
        '--device-id',
        help='ID of connected hardware device (for example 000682459367).'
    )
    twister_harness_group.addoption(
        '--device-product',
        help='Product name of connected hardware device (e.g. "STM32 STLink").'
    )
    twister_harness_group.addoption(
        '--device-serial-pty',
        help='Script for controlling pseudoterminal.'
    )
    twister_harness_group.addoption(
        '--west-flash-extra-args',
        help='Extend parameters for west flash. '
             'E.g. --west-flash-extra-args="--board-id=foobar,--erase" '
             'will translate to "west flash -- --board-id=foobar --erase".'
    )
    twister_harness_group.addoption(
        '--pre-script',
        metavar='PATH',
        help='Script executed before flashing and connecting to serial.'
    )
    twister_harness_group.addoption(
        '--post-flash-script',
        metavar='PATH',
        help='Script executed after flashing.'
    )
    twister_harness_group.addoption(
        '--post-script',
        metavar='PATH',
        help='Script executed after closing serial connection.'
    )
    twister_harness_group.addoption(
        '--dut-scope',
        choices=('function', 'class', 'module', 'package', 'session'),
        help='The scope for which `dut` and `shell` fixtures are shared.'
    )


def pytest_configure(config: pytest.Config):
    if config.getoption('help'):
        return

    if not (config.getoption('twister_harness') or config.getini('twister_harness')):
        return

    _normalize_paths(config)
    _validate_options(config)

    config.twister_harness_config = TwisterHarnessConfig.create(config)  # type: ignore


def _validate_options(config: pytest.Config) -> None:
    if not config.option.build_dir:
        raise Exception('--build-dir has to be provided')
    if not os.path.isdir(config.option.build_dir):
        raise Exception(f'Provided --build-dir does not exist: {config.option.build_dir}')
    if not config.option.device_type:
        raise Exception('--device-type has to be provided')


def _normalize_paths(config: pytest.Config) -> None:
    """Normalize paths provided by user via CLI"""
    config.option.build_dir = _normalize_path(config.option.build_dir)
    config.option.pre_script = _normalize_path(config.option.pre_script)
    config.option.post_script = _normalize_path(config.option.post_script)
    config.option.post_flash_script = _normalize_path(config.option.post_flash_script)


def _normalize_path(path: str | None) -> str:
    if path is not None:
        path = os.path.expanduser(os.path.expandvars(path))
        path = os.path.normpath(os.path.abspath(path))
    return path
