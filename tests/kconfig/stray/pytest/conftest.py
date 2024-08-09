# Copyright (c) 2024 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0

import pytest

# add option "--cmdopt" to pytest, or it will report "unknown option"
# this option is passed from twister.
def pytest_addoption(parser):
    parser.addoption(
        '--test_kconfig'
    )

@pytest.fixture()
def test_kconfig(request):
    return request.config.getoption('--test_kconfig')
