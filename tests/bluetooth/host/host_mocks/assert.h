/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>
#include <zephyr/zephyr.h>

DECLARE_FAKE_VALUE_FUNC(bool, mock_check_if_assert_expected);

#define expect_assert()     (mock_check_if_assert_expected_fake.return_val = 1)
