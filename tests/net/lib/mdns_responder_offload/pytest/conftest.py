# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

import pytest


def pytest_addoption(parser):
    # When set, a host that lacks a routable address of the given family makes
    # the test fail instead of skip, letting tests.yaml enforce that the CI
    # host provides that family.
    parser.addoption("--require-ipv4", action="store_true", default=False)
    parser.addoption("--require-ipv6", action="store_true", default=False)


@pytest.fixture()
def require_ipv4(request) -> bool:
    return request.config.getoption("--require-ipv4")


@pytest.fixture()
def require_ipv6(request) -> bool:
    return request.config.getoption("--require-ipv6")
