# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import pytest

pytest.register_assert_rewrite('twister_harness.fixtures.dut')
pytest.register_assert_rewrite('twister_harness.fixtures.mcumgr')
