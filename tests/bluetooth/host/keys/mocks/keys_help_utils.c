/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/addr.h>
#include <host/keys.h>
#include "keys_help_utils.h"

void clear_key_pool(void)
{
	struct bt_keys *key_pool;

	key_pool = bt_keys_get_key_pool();
	memset(key_pool, 0x00, sizeof(struct bt_keys) * CONFIG_BT_MAX_PAIRED);
}

int fill_key_pool_by_id_addr(const struct id_addr_pair src[], int size, struct bt_keys *refs[])
{
	uint8_t id;
	bt_addr_le_t *addr;
	struct id_addr_pair const *params_vector;

	if (!check_key_pool_is_empty()) {
		printk("'%s' Error ! Keys pool isn't empty\n", __func__);
		return -ENOSR;
	}

	for (size_t it = 0; it < size; it++) {
		params_vector = &src[it];
		id = params_vector->id;
		addr = params_vector->addr;
		refs[it] = bt_keys_get_addr(id, addr);
		if (refs[it] == NULL) {
			printk("'%s' Failed to add key %d to the keys pool\n", __func__, it);
			return -ENOBUFS;
		}
	}

	return 0;
}

int fill_key_pool_by_id_addr_type(const struct id_addr_type src[], int size, struct bt_keys *refs[])
{
	int type;
	uint8_t id;
	bt_addr_le_t *addr;
	struct id_addr_type const *params_vector;

	if (!check_key_pool_is_empty()) {
		printk("'%s' Error ! Keys pool isn't empty\n", __func__);
		return -ENOSR;
	}

	for (size_t it = 0; it < size; it++) {
		params_vector = &src[it];
		type = params_vector->type;
		id = params_vector->id;
		addr = params_vector->addr;
		refs[it] = bt_keys_get_type(type, id, addr);
		if (refs[it] == NULL) {
			printk("'%s' Failed to add key %d to the keys pool\n", __func__, it);
			return -ENOBUFS;
		}
	}

	return 0;
}

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
