/* keys_br.c - Bluetooth BR/EDR key handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/settings/settings.h>

#include <common/bt_str.h>

#include <host/conn_internal.h>
#include <host/hci_core.h>
#include <host/settings.h>
#include <host/keys.h>

#define LOG_LEVEL CONFIG_BT_KEYS_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_keys_br);

static struct bt_keys_link_key key_pool[CONFIG_BT_MAX_PAIRED];

#if defined(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
static uint32_t aging_counter_val;
static struct bt_keys_link_key *last_keys_updated;

struct key_data {
	const struct bt_keys_link_key *key;
	bool in_use;
};

static void find_key_in_use(struct bt_conn *conn, void *data)
{
	struct key_data *kdata = data;
	bt_conn_state_t state = conn->state;

	/* Disconnect cleanup still accesses the key in both pending-disconnect states. */
	if ((state == BT_CONN_CONNECTED || state == BT_CONN_DISCONNECTING ||
	     state == BT_CONN_DISCONNECT_COMPLETE) &&
	    bt_addr_eq(&conn->br.dst, &kdata->key->addr)) {
		kdata->in_use = true;
	}
}

static bool key_is_in_use(const struct bt_keys_link_key *key)
{
	struct key_data kdata = {
		.key = key,
	};

	bt_conn_foreach(BT_CONN_TYPE_BR, find_key_in_use, &kdata);

	return kdata.in_use;
}
#endif /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */

struct bt_keys_link_key *bt_keys_find_link_key(const bt_addr_t *addr)
{
	struct bt_keys_link_key *key;
	int i;

	LOG_DBG("%s", bt_addr_str(addr));

	for (i = 0; i < ARRAY_SIZE(key_pool); i++) {
		key = &key_pool[i];

		if (bt_addr_eq(&key->addr, addr)) {
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
#if defined(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
	if (!key) {
		int i;

		for (i = 0; i < ARRAY_SIZE(key_pool); i++) {
			struct bt_keys_link_key *current = &key_pool[i];

			if (key_is_in_use(current)) {
				continue;
			}

			if (key == NULL || current->aging_counter < key->aging_counter) {
				key = current;
			}
		}

		if (key) {
			bt_keys_link_key_clear(key);
		}
	}
#endif

	if (key) {
		bt_addr_copy(&key->addr, addr);
#if defined(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
		key->aging_counter = ++aging_counter_val;
		last_keys_updated = key;
#endif
		LOG_DBG("created %p for %s", key, bt_addr_str(addr));
		return key;
	}

	LOG_DBG("unable to create keys for %s", bt_addr_str(addr));

	return NULL;
}

void bt_keys_link_key_clear(struct bt_keys_link_key *link_key)
{
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_addr_le_t le_addr;

		le_addr.type = BT_ADDR_LE_PUBLIC;
		bt_addr_copy(&le_addr.a, &link_key->addr);

		bt_settings_delete_link_key(&le_addr);
	}

	LOG_DBG("%s", bt_addr_str(&link_key->addr));
	(void)memset(link_key, 0, sizeof(*link_key));
}

void bt_keys_link_key_clear_addr(const bt_addr_t *addr)
{
	int i;
	struct bt_keys_link_key *key;

	if (!addr) {
		for (i = 0; i < ARRAY_SIZE(key_pool); i++) {
			key = &key_pool[i];
			bt_keys_link_key_clear(key);
		}
		return;
	}

	key = bt_keys_find_link_key(addr);
	if (key) {
		bt_keys_link_key_clear(key);
	}
}

void bt_keys_link_key_store(struct bt_keys_link_key *link_key)
{
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		int err;
		bt_addr_le_t le_addr;

		le_addr.type = BT_ADDR_LE_PUBLIC;
		bt_addr_copy(&le_addr.a, &link_key->addr);

		err = bt_settings_store_link_key(&le_addr, link_key->storage_start,
						 BT_KEYS_LINK_KEY_STORAGE_LEN);
		if (err) {
			LOG_ERR("Failed to save link key (err %d)", err);
		}
	}
}

void bt_br_foreach_bond(void (*func)(const struct bt_br_bond_info *info, void *user_data),
			void *user_data)
{
	__ASSERT_NO_MSG(func != NULL);

	for (size_t i = 0; i < ARRAY_SIZE(key_pool); i++) {
		const struct bt_keys_link_key *key = &key_pool[i];

		if (!bt_addr_eq(&key->addr, BT_ADDR_ANY)) {
			struct bt_br_bond_info info;

			bt_addr_copy(&info.addr, &key->addr);
			func(&info, user_data);
		}
	}
}

#if defined(CONFIG_BT_SETTINGS)

static int link_key_set(const char *name, size_t len_rd,
			settings_read_cb read_cb, void *cb_arg)
{
	int err;
	ssize_t len;
	bt_addr_le_t le_addr;
	struct bt_keys_link_key *link_key;
	char val[BT_KEYS_LINK_KEY_STORAGE_LEN];

	if (!name) {
		LOG_ERR("Insufficient number of arguments");
		return -EINVAL;
	}

	len = read_cb(cb_arg, val, sizeof(val));
	if (len < 0) {
		LOG_ERR("Failed to read value (err %zu)", len);
		return -EINVAL;
	}

	LOG_DBG("name %s val %s", name, len ? bt_hex(val, sizeof(val)) : "(null)");

	err = bt_settings_decode_key(name, &le_addr);
	if (err) {
		LOG_ERR("Unable to decode address %s", name);
		return -EINVAL;
	}

	link_key = bt_keys_get_link_key(&le_addr.a);
	if (len != BT_KEYS_LINK_KEY_STORAGE_LEN) {
		if (link_key) {
			bt_keys_link_key_clear(link_key);
			LOG_DBG("Clear keys for %s", bt_addr_le_str(&le_addr));
		} else {
			LOG_WRN("Unable to find deleted keys for %s", bt_addr_le_str(&le_addr));
		}

		return 0;
	}

	if (link_key == NULL) {
		LOG_ERR("Unable to restore link key for %s", bt_addr_le_str(&le_addr));
		return -ENOMEM;
	}

	memcpy(link_key->storage_start, val, len);
	LOG_DBG("Successfully restored link key for %s", bt_addr_le_str(&le_addr));
#if defined(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
	if (aging_counter_val < link_key->aging_counter) {
		aging_counter_val = link_key->aging_counter;
	}
#endif  /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(bt_link_key, "bt/link_key", NULL, link_key_set,
			       NULL, NULL);

#endif /* defined(CONFIG_BT_SETTINGS) */

#if defined(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
void bt_keys_link_key_update_usage(const bt_addr_t *addr)
{
	struct bt_keys_link_key *link_key = bt_keys_find_link_key(addr);

	if (!link_key) {
		return;
	}

	if (last_keys_updated == link_key) {
		return;
	}

	link_key->aging_counter = ++aging_counter_val;
	last_keys_updated = link_key;

	LOG_DBG("Aging counter for %s is set to %u", bt_addr_str(addr), link_key->aging_counter);

	if (IS_ENABLED(CONFIG_BT_KEYS_SAVE_AGING_COUNTER_ON_PAIRING)) {
		bt_keys_link_key_store(link_key);
	}
}
#endif  /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */
