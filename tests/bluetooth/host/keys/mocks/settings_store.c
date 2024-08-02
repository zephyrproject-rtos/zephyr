/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "mocks/settings_store.h"

DEFINE_FAKE_VALUE_FUNC(int, bt_settings_store_keys, uint8_t, struct bt_addr_le_t *, const void *,
		       size_t);
DEFINE_FAKE_VALUE_FUNC(int, bt_settings_delete_keys, uint8_t, struct bt_addr_le_t *);
