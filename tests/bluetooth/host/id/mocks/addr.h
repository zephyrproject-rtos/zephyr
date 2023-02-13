/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/addr.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

/* List of fakes used by this unit tester */
#define ADDR_FFF_FAKES_LIST(FAKE) FAKE(bt_addr_le_create_static)

DECLARE_FAKE_VALUE_FUNC(int, bt_addr_le_create_static, bt_addr_le_t *);
