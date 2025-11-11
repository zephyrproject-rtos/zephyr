#
# Copyright (c) 2024 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
#

import pytest


def pytest_addoption(parser):
    parser.addoption('--fpu', action="store_true")


@pytest.fixture()
def is_fpu_build(request):
    return request.config.getoption('--fpu')
