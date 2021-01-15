# Copyright (c) 2020 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0

import pytest

# add option "--comdopt" to pytest, or it will report "unknown option"
# this option is passed from twister.
def pytest_addoption(parser):
    parser.addoption(
        '--cmdopt'
    )

# define fixture to return value of option "--cmdopt", this fixture
# will be requested by other fixture of tests.
@pytest.fixture()
def cmdopt(request):
    return request.config.getoption('--cmdopt')
