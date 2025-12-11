/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <mocks/keys.h>

DEFINE_FAKE_VALUE_FUNC(struct bt_keys *, bt_keys_find_irk, uint8_t, const bt_addr_le_t *);
DEFINE_FAKE_VOID_FUNC(bt_keys_foreach_type, enum bt_keys_type, bt_keys_foreach_type_cb, void *);
