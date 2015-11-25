/* keys.c - Bluetooth key handling */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include <nanokernel.h>
#include <atomic.h>
#include <misc/util.h>

#include <bluetooth/log.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#include "hci_core.h"
#include "smp.h"
#include "keys.h"

#if !defined(CONFIG_BLUETOOTH_DEBUG_KEYS)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

static struct bt_keys key_pool[CONFIG_BLUETOOTH_MAX_PAIRED];

struct bt_keys *bt_keys_get_addr(const bt_addr_le_t *addr)
{
	struct bt_keys *keys;
	int i;

	BT_DBG("%s", bt_addr_le_str(addr));

	for (i = 0; i < ARRAY_SIZE(key_pool); i++) {
		keys = &key_pool[i];

		if (!bt_addr_le_cmp(&keys->addr, addr)) {
			return keys;
		}

		if (!bt_addr_le_cmp(&keys->addr, BT_ADDR_LE_ANY)) {
			bt_addr_le_copy(&keys->addr, addr);
			BT_DBG("created %p for %s", keys,
			       bt_addr_le_str(addr));
			return keys;
		}
	}

	BT_DBG("unable to create keys for %s", bt_addr_le_str(addr));

	return NULL;
}

void bt_keys_clear(struct bt_keys *keys, int type)
{
	BT_DBG("keys for %s type %d", bt_addr_le_str(&keys->addr), type);

	keys->keys &= ~type;

	if (!keys->keys) {
		memset(keys, 0, sizeof(*keys));
	}
}

struct bt_keys *bt_keys_find(int type, const bt_addr_le_t *addr)
{
	int i;

	BT_DBG("type %d %s", type, bt_addr_le_str(addr));

	for (i = 0; i < ARRAY_SIZE(key_pool); i++) {
		if ((key_pool[i].keys & type) &&
		     !bt_addr_le_cmp(&key_pool[i].addr, addr)) {
			return &key_pool[i];
		}
	}

	return NULL;
}

void bt_keys_add_type(struct bt_keys *keys, int type)
{
	keys->keys |= type;
}

struct bt_keys *bt_keys_get_type(int type, const bt_addr_le_t *addr)
{
	struct bt_keys *keys;

	BT_DBG("type %d %s", type, bt_addr_le_str(addr));

	keys = bt_keys_find(type, addr);
	if (keys) {
		return keys;
	}

	keys = bt_keys_get_addr(addr);
	if (!keys) {
		return NULL;
	}

	bt_keys_add_type(keys, type);

	return keys;
}

struct bt_keys *bt_keys_find_irk(const bt_addr_le_t *addr)
{
	int i;

	BT_DBG("%s", bt_addr_le_str(addr));

	if (!bt_addr_le_is_rpa(addr)) {
		return NULL;
	}

	for (i = 0; i < ARRAY_SIZE(key_pool); i++) {
		if (!(key_pool[i].keys & BT_KEYS_IRK)) {
			continue;
		}

		if (!bt_addr_cmp((bt_addr_t *)addr->val,
				 &key_pool[i].irk.rpa)) {
			BT_DBG("cached RPA %s for %s",
			       bt_addr_str(&key_pool[i].irk.rpa),
			       bt_addr_le_str(&key_pool[i].addr));
			return &key_pool[i];
		}
	}

	for (i = 0; i < ARRAY_SIZE(key_pool); i++) {
		if (!(key_pool[i].keys & BT_KEYS_IRK)) {
			continue;
		}

		if (bt_smp_irk_matches(key_pool[i].irk.val,
				       (bt_addr_t *)addr->val)) {
			BT_DBG("RPA %s matches %s",
			       bt_addr_str(&key_pool[i].irk.rpa),
			       bt_addr_le_str(&key_pool[i].addr));

			bt_addr_copy(&key_pool[i].irk.rpa,
				     (bt_addr_t *)addr->val);

			return &key_pool[i];
		}
	}

	BT_DBG("No IRK for %s", bt_addr_le_str(addr));

	return NULL;
}

struct bt_keys *bt_keys_find_addr(const bt_addr_le_t *addr)
{
	int i;

	BT_DBG("%s", bt_addr_le_str(addr));

	for (i = 0; i < ARRAY_SIZE(key_pool); i++) {
		if (!bt_addr_le_cmp(&key_pool[i].addr, addr)) {
			return &key_pool[i];
		}
	}

	return NULL;
}
