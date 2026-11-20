# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

import pytest


def pytest_addoption(parser):
    parser.addoption('--gdb_port', default=None)


@pytest.fixture()
def gdb_port(request):
    return request.config.getoption('--gdb_port')
