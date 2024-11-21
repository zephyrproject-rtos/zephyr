# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path

import pytest


def pytest_addoption(parser):
    parser.addoption('--tool')
    parser.addoption('--tool-options')
    parser.addoption('--sys-clock-hw-cycles-per-sec', default=None)


@pytest.fixture()
def tool(request):
    return request.config.getoption('--tool')


@pytest.fixture()
def tool_options(request):
    return request.config.getoption('--tool-options')


@pytest.fixture()
def config(request):
    build_dir = Path(request.config.getoption('--build-dir'))
    file_name = build_dir / 'zephyr' / '.config'

    cfgs = {}
    with open(file_name) as fp:
        for line in fp:
            if line.startswith('CONFIG_'):
                k, v = line.split('=', maxsplit=1)
                cfgs[k[7:]] = v

    return cfgs


@pytest.fixture()
def sys_clock_hw_cycles_per_sec(request, config):
    if request.config.getoption('--sys-clock-hw-cycles-per-sec'):
        return int(request.config.getoption('--sys-clock-hw-cycles-per-sec'))

    return int(config['SYS_CLOCK_HW_CYCLES_PER_SEC'])
