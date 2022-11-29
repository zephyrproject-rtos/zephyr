/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/keys.h>

typedef void (*bt_keys_foreach_type_cb)(struct bt_keys *keys, void *data);

/* List of fakes used by this unit tester */
#define KEYS_FFF_FAKES_LIST(FAKE)                                                                  \
	FAKE(bt_keys_find_irk)                                                                     \
	FAKE(bt_keys_foreach_type)

DECLARE_FAKE_VALUE_FUNC(struct bt_keys *, bt_keys_find_irk, uint8_t, const bt_addr_le_t *);
DECLARE_FAKE_VOID_FUNC(bt_keys_foreach_type, enum bt_keys_type, bt_keys_foreach_type_cb, void *);
