#
# Copyright (c) 2023 intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
#

import pytest

def pytest_addoption(parser):
    parser.addoption('--gdb_timeout')
    parser.addoption('--gdb_script')

@pytest.fixture()
def gdb_script(request):
    return request.config.getoption('--gdb_script')

@pytest.fixture()
def gdb_timeout(request):
    return int(request.config.getoption('--gdb_timeout', default=60))
#
