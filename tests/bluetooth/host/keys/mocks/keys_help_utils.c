/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <bluetooth/addr.h>
#include <host/keys.h>
#include "keys_help_utils.h"

bool check_key_pool_is_empty(void)
{
	int i;
	struct bt_keys *keys, *key_pool;

	key_pool = bt_keys_get_key_pool();
	for (i = 0; i < CONFIG_BT_MAX_PAIRED; i++) {
		keys = &key_pool[i];
		if (bt_addr_le_cmp(&keys->addr, BT_ADDR_LE_ANY)) {
			return false;
		}
	}

	return true;
}
