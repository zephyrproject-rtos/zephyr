# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import os

import pytest
from twister_harness import DeviceAdapter


def pytest_addoption(parser):
    parser.addoption('--server-type')


@pytest.fixture()
def server_type(request):
    return request.config.getoption('--server-type')


@pytest.fixture()
def app_config(device_object: DeviceAdapter, config_reader):
    return config_reader(
        os.path.join(device_object.device_config.app_build_dir, 'zephyr', '.config')
    )


@pytest.fixture()
def port(app_config):
    return app_config.read_int('CONFIG_SERVER_PORT')


@pytest.fixture()
def test_timeout(app_config):
    return app_config.read_int('CONFIG_TEST_TIMEOUT')
