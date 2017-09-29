/* keys_br.c - Bluetooth BR/EDR key handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <atomic.h>
#include <misc/util.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/hci.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_KEYS)
#include "common/log.h"

#include "hci_core.h"
#include "keys.h"

static struct bt_keys_link_key key_pool[CONFIG_BT_MAX_PAIRED];

struct bt_keys_link_key *bt_keys_find_link_key(const bt_addr_t *addr)
{
	struct bt_keys_link_key *key;
	int i;

	BT_DBG("%s", bt_addr_str(addr));

	for (i = 0; i < ARRAY_SIZE(key_pool); i++) {
		key = &key_pool[i];

		if (!bt_addr_cmp(&key->addr, addr)) {
			return key;
		}
	}

	return NULL;
}

struct bt_keys_link_key *bt_keys_get_link_key(const bt_addr_t *addr)
{
	struct bt_keys_link_key *key;

	key = bt_keys_find_link_key(addr);
	if (key) {
		return key;
	}

	key = bt_keys_find_link_key(BT_ADDR_ANY);
	if (key) {
		bt_addr_copy(&key->addr, addr);
		BT_DBG("created %p for %s", key, bt_addr_str(addr));

		return key;
	}

	BT_DBG("unable to create link key for %s", bt_addr_str(addr));

	return NULL;
}

void bt_keys_link_key_clear(struct bt_keys_link_key *link_key)
{
	BT_DBG("%s", bt_addr_str(&link_key->addr));

	memset(link_key, 0, sizeof(*link_key));
}

void bt_keys_link_key_clear_addr(const bt_addr_t *addr)
{
	struct bt_keys_link_key *key;

	if (!addr) {
		memset(key_pool, 0, sizeof(key_pool));
		return;
	}

	key = bt_keys_find_link_key(addr);
	if (key) {
		bt_keys_link_key_clear(key);
	}
}
