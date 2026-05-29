/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fff.h>
#include <zephyr/bluetooth/addr.h>

/* List of fakes used by this unit tester */
#define SETTINGS_STORE_FFF_FAKES_LIST(FAKE)                                                        \
	FAKE(bt_settings_store_keys)                                                               \
	FAKE(bt_settings_delete_keys)

DECLARE_FAKE_VALUE_FUNC(int, bt_settings_store_keys, uint8_t, struct bt_addr_le_t *, const void *,
			size_t);
DECLARE_FAKE_VALUE_FUNC(int, bt_settings_delete_keys, uint8_t, struct bt_addr_le_t *);
