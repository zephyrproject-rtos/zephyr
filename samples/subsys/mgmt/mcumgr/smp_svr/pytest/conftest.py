# Copyright (c) 2024 Marcin Niestroj
#
# SPDX-License-Identifier: Apache-2.0

import pytest


@pytest.fixture(scope='session')
def anyio_backend():
    return 'asyncio'
