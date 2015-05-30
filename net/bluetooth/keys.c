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

static struct bt_keys key_list[CONFIG_BLUETOOTH_MAX_PAIRED];

struct bt_keys *bt_keys_create(const bt_addr_le_t *addr)
{
	struct bt_keys *keys;
	int i;

	BT_DBG("%s\n", bt_addr_le_str(addr));

	keys = bt_keys_find(addr);
	if (keys) {
		return keys;
	}

	for (i = 0; i < ARRAY_SIZE(key_list); i++) {
		keys = &key_list[i];

		if (!bt_addr_le_cmp(&keys->addr, BT_ADDR_LE_ANY)) {
			bt_addr_le_copy(&keys->addr, addr);
			BT_DBG("created keys %p\n", keys);
			return keys;
		}
	}

	BT_DBG("no match\n");

	return NULL;
}

struct bt_keys *bt_keys_find(const bt_addr_le_t *addr)
{
	int i;

	BT_DBG("%s\n", bt_addr_le_str(addr));

	for (i = 0; i < ARRAY_SIZE(key_list); i++) {
		struct bt_keys *keys = &key_list[i];

		if (!bt_addr_le_cmp(&keys->addr, addr)) {
			BT_DBG("found keys %p\n", keys);
			return keys;
		}
	}

	BT_DBG("no match\n");

	return NULL;
}

void bt_keys_clear(struct bt_keys *keys)
{
	BT_DBG("keys %p\n", keys);
	memset(keys, 0, sizeof(*keys));
}
