/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <mocks/addr.h>

DEFINE_FAKE_VALUE_FUNC(int, bt_addr_le_create_static, bt_addr_le_t *);
