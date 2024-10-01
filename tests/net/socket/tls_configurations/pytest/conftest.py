# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import pytest

def pytest_addoption(parser):
    parser.addoption('--server-type')
    parser.addoption('--port')

@pytest.fixture()
def server_type(request):
    return request.config.getoption('--server-type')

@pytest.fixture()
def port(request):
    return request.config.getoption('--port')
