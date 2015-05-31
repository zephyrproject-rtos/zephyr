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
#include <misc/util.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#include "hci_core.h"
#include "keys.h"

#define bt_keys_foreach(list, cur, member)   \
	for (cur = list; *cur; cur = &(*cur)->member)

static struct bt_keys key_pool[CONFIG_BLUETOOTH_MAX_PAIRED];

static struct bt_keys *slave_ltks;

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
	struct bt_keys **cur;

	BT_DBG("keys %p\n", keys);

	if (((type & keys->keys) & BT_KEYS_SLAVE_LTK)) {
		bt_keys_foreach(&slave_ltks, cur, slave_ltk.next) {
			if (*cur == keys) {
				*cur = (*cur)->slave_ltk.next;
				break;
			}
		}
		keys->keys &= ~BT_KEYS_SLAVE_LTK;
	}

	if (!keys->keys) {
		memset(keys, 0, sizeof(*keys));
	}
}

struct bt_keys *bt_keys_find(int type, const bt_addr_le_t *addr)
{
	struct bt_keys **cur;

	BT_DBG("type %d %s\n", type, bt_addr_le_str(addr));

	switch (type) {
	case BT_KEYS_SLAVE_LTK:
		bt_keys_foreach(&slave_ltks, cur, slave_ltk.next) {
			if (!bt_addr_le_cmp(&(*cur)->addr, addr)) {
				break;
			}
		}
		return *cur;
	default:
		return NULL;
	}
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

	switch (type) {
	case BT_KEYS_SLAVE_LTK:
		keys->slave_ltk.next = slave_ltks;
		slave_ltks = keys;
		break;
	default:
		return NULL;
	}

	keys->keys |= type;

	return keys;
}
