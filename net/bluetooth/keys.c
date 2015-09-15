/* keys.c - Bluetooth key handling */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

	BT_DBG("%s\n", bt_addr_le_str(addr));

	for (i = 0; i < ARRAY_SIZE(key_pool); i++) {
		keys = &key_pool[i];

		if (!bt_addr_le_cmp(&keys->addr, addr)) {
			return keys;
		}

		if (!bt_addr_le_cmp(&keys->addr, BT_ADDR_LE_ANY)) {
			bt_addr_le_copy(&keys->addr, addr);
			BT_DBG("created %p for %s\n", keys,
			       bt_addr_le_str(addr));
			return keys;
		}
	}

	BT_DBG("unable to create keys for %s\n", bt_addr_le_str(addr));

	return NULL;
}

void bt_keys_clear(struct bt_keys *keys, int type)
{
	BT_DBG("keys for %s type %d\n", bt_addr_le_str(&keys->addr), type);

	keys->keys &= ~type;

	if (!keys->keys) {
		memset(keys, 0, sizeof(*keys));
	}
}

struct bt_keys *bt_keys_find(int type, const bt_addr_le_t *addr)
{
	int i;

	BT_DBG("type %d %s\n", type, bt_addr_le_str(addr));

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

	BT_DBG("type %d %s\n", type, bt_addr_le_str(addr));

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

	BT_DBG("%s\n", bt_addr_le_str(addr));

	if (!bt_addr_le_is_rpa(addr)) {
		return NULL;
	}

	for (i = 0; i < ARRAY_SIZE(key_pool); i++) {
		if (!(key_pool[i].keys & BT_KEYS_IRK)) {
			continue;
		}

		if (!bt_addr_cmp((bt_addr_t *)addr->val,
				 &key_pool[i].irk.rpa)) {
			BT_DBG("cached RPA %s for %s\n",
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
			BT_DBG("RPA %s matches %s\n",
			       bt_addr_str(&key_pool[i].irk.rpa),
			       bt_addr_le_str(&key_pool[i].addr));

			bt_addr_copy(&key_pool[i].irk.rpa,
				     (bt_addr_t *)addr->val);

			return &key_pool[i];
		}
	}

	BT_DBG("No IRK for %s\n", bt_addr_le_str(addr));

	return NULL;
}

struct bt_keys *bt_keys_find_addr(const bt_addr_le_t *addr)
{
	int i;

	BT_DBG("%s\n", bt_addr_le_str(addr));

	for (i = 0; i < ARRAY_SIZE(key_pool); i++) {
		if (!bt_addr_le_cmp(&key_pool[i].addr, addr)) {
			return &key_pool[i];
		}
	}

	return NULL;
}
