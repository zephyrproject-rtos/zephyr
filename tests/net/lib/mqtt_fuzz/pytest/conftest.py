# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: Apache-2.0

import pytest


def pytest_addoption(parser):
    parser.addoption("--fuzz-max-total-time", type=int, default=3300)
    parser.addoption("--fuzz-max-len", type=int, default=4096)


@pytest.fixture()
def fuzz_max_total_time(request):
    return request.config.getoption("--fuzz-max-total-time")


@pytest.fixture()
def fuzz_max_len(request):
    return request.config.getoption("--fuzz-max-len")
