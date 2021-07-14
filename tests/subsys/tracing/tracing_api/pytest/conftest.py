#!/usr/bin/env python3
#
# Copyright (c) 2021 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0

import pytest

def pytest_addoption(parser):
    parser.addoption(
        '--cmdopt'
    )

@pytest.fixture()
def cmdopt(request):
    return request.config.getoption('--cmdopt')
