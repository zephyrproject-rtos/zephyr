/* keys.c - Bluetooth key handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_KEYS)
#define LOG_MODULE_NAME bt_keys
#include "common/log.h"

#include "common/rpa.h"
#include "conn_internal.h"
#include "gatt_internal.h"
#include "hci_core.h"
#include "smp.h"
#include "settings.h"
#include "keys.h"

static struct bt_keys key_pool[CONFIG_BT_MAX_PAIRED];

#define BT_KEYS_STORAGE_LEN_COMPAT (BT_KEYS_STORAGE_LEN - sizeof(uint32_t))

#if IS_ENABLED(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
static uint32_t aging_counter_val;
static struct bt_keys *last_keys_updated;

struct key_data {
	bool in_use;
	uint8_t id;
};

static void find_key_in_use(struct bt_conn *conn, void *data)
{
	struct key_data *kdata = data;
	struct bt_keys *key;

	__ASSERT_NO_MSG(conn != NULL);
	__ASSERT_NO_MSG(data != NULL);

	if (conn->state == BT_CONN_CONNECTED) {
		key = bt_keys_find_addr(conn->id, bt_conn_get_dst(conn));
		if (key == NULL) {
			return;
		}

		/* Ensure that the reference returned matches the current pool item */
		if (key == &key_pool[kdata->id]) {
			kdata->in_use = true;
			BT_DBG("Connected device %s is using key_pool[%d]",
			       bt_addr_le_str(bt_conn_get_dst(conn)), kdata->id);
		}
	}
}

static bool key_is_in_use(uint8_t id)
{
	struct key_data kdata = { false, id };

	bt_conn_foreach(BT_CONN_TYPE_ALL, find_key_in_use, &kdata);

	return kdata.in_use;
}
#endif /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */

struct bt_keys *bt_keys_get_addr(uint8_t id, const bt_addr_le_t *addr)
{
	struct bt_keys *keys;
	int i;
	size_t first_free_slot = ARRAY_SIZE(key_pool);

	__ASSERT_NO_MSG(addr != NULL);

	BT_DBG("%s", bt_addr_le_str(addr));

	for (i = 0; i < ARRAY_SIZE(key_pool); i++) {
		keys = &key_pool[i];

		if (keys->id == id && !bt_addr_le_cmp(&keys->addr, addr)) {
			return keys;
		}
		if (first_free_slot == ARRAY_SIZE(key_pool) &&
		    !bt_addr_le_cmp(&keys->addr, BT_ADDR_LE_ANY)) {
			first_free_slot = i;
		}
	}

#if IS_ENABLED(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
	if (first_free_slot == ARRAY_SIZE(key_pool)) {
		struct bt_keys *oldest = NULL;
		bt_addr_le_t oldest_addr;

		for (i = 0; i < ARRAY_SIZE(key_pool); i++) {
			struct bt_keys *current = &key_pool[i];
			bool key_in_use = key_is_in_use(i);

			if (key_in_use) {
				continue;
			}

			if ((oldest == NULL) || (current->aging_counter < oldest->aging_counter)) {
				oldest = current;
			}
		}

		if (oldest == NULL) {
			BT_DBG("unable to create keys for %s", bt_addr_le_str(addr));
			return NULL;
		}

		/* Use a copy as bt_unpair will clear the oldest key. */
		bt_addr_le_copy(&oldest_addr, &oldest->addr);
		bt_unpair(oldest->id, &oldest_addr);
		if (!bt_addr_le_cmp(&oldest->addr, BT_ADDR_LE_ANY)) {
			first_free_slot = oldest - &key_pool[0];
		}
	}

#endif  /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */
	if (first_free_slot < ARRAY_SIZE(key_pool)) {
		keys = &key_pool[first_free_slot];
		keys->id = id;
		bt_addr_le_copy(&keys->addr, addr);
#if IS_ENABLED(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
		keys->aging_counter = ++aging_counter_val;
		last_keys_updated = keys;
#endif  /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */
		BT_DBG("created %p for %s", keys, bt_addr_le_str(addr));
		return keys;
	}

	BT_DBG("unable to create keys for %s", bt_addr_le_str(addr));

	return NULL;
}

void bt_foreach_bond(uint8_t id, void (*func)(const struct bt_bond_info *info,
					   void *user_data),
		     void *user_data)
{
	int i;

	__ASSERT_NO_MSG(func != NULL);

	for (i = 0; i < ARRAY_SIZE(key_pool); i++) {
		struct bt_keys *keys = &key_pool[i];

		if (keys->keys && keys->id == id) {
			struct bt_bond_info info;

			bt_addr_le_copy(&info.addr, &keys->addr);
			func(&info, user_data);
		}
	}
}

void bt_keys_foreach_type(enum bt_keys_type type, void (*func)(struct bt_keys *keys, void *data),
			  void *data)
{
	int i;

	__ASSERT_NO_MSG(func != NULL);

	for (i = 0; i < ARRAY_SIZE(key_pool); i++) {
		if ((key_pool[i].keys & type)) {
			func(&key_pool[i], data);
		}
	}
}

struct bt_keys *bt_keys_find(enum bt_keys_type type, uint8_t id, const bt_addr_le_t *addr)
{
	int i;

	__ASSERT_NO_MSG(addr != NULL);

	BT_DBG("type %d %s", type, bt_addr_le_str(addr));

	for (i = 0; i < ARRAY_SIZE(key_pool); i++) {
		if ((key_pool[i].keys & type) && key_pool[i].id == id &&
		    !bt_addr_le_cmp(&key_pool[i].addr, addr)) {
			return &key_pool[i];
		}
	}

	return NULL;
}

struct bt_keys *bt_keys_get_type(enum bt_keys_type type, uint8_t id, const bt_addr_le_t *addr)
{
	struct bt_keys *keys;

	__ASSERT_NO_MSG(addr != NULL);

	BT_DBG("type %d %s", type, bt_addr_le_str(addr));

	keys = bt_keys_find(type, id, addr);
	if (keys) {
		return keys;
	}

	keys = bt_keys_get_addr(id, addr);
	if (!keys) {
		return NULL;
	}

	bt_keys_add_type(keys, type);

	return keys;
}

struct bt_keys *bt_keys_find_irk(uint8_t id, const bt_addr_le_t *addr)
{
	int i;

	__ASSERT_NO_MSG(addr != NULL);

	BT_DBG("%s", bt_addr_le_str(addr));

	if (!bt_addr_le_is_rpa(addr)) {
		return NULL;
	}

	for (i = 0; i < ARRAY_SIZE(key_pool); i++) {
		if (!(key_pool[i].keys & BT_KEYS_IRK)) {
			continue;
		}

		if (key_pool[i].id == id &&
		    !bt_addr_cmp(&addr->a, &key_pool[i].irk.rpa)) {
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

		if (key_pool[i].id != id) {
			continue;
		}

		if (bt_rpa_irk_matches(key_pool[i].irk.val, &addr->a)) {
			BT_DBG("RPA %s matches %s",
			       bt_addr_str(&key_pool[i].irk.rpa),
			       bt_addr_le_str(&key_pool[i].addr));

			bt_addr_copy(&key_pool[i].irk.rpa, &addr->a);

			return &key_pool[i];
		}
	}

	BT_DBG("No IRK for %s", bt_addr_le_str(addr));

	return NULL;
}

struct bt_keys *bt_keys_find_addr(uint8_t id, const bt_addr_le_t *addr)
{
	int i;

	__ASSERT_NO_MSG(addr != NULL);

	BT_DBG("%s", bt_addr_le_str(addr));

	for (i = 0; i < ARRAY_SIZE(key_pool); i++) {
		if (key_pool[i].id == id &&
		    !bt_addr_le_cmp(&key_pool[i].addr, addr)) {
			return &key_pool[i];
		}
	}

	return NULL;
}

void bt_keys_add_type(struct bt_keys *keys, enum bt_keys_type type)
{
	__ASSERT_NO_MSG(keys != NULL);

	keys->keys |= type;
}

void bt_keys_clear(struct bt_keys *keys)
{
	__ASSERT_NO_MSG(keys != NULL);

	BT_DBG("%s (keys 0x%04x)", bt_addr_le_str(&keys->addr), keys->keys);

	if (keys->state & BT_KEYS_ID_ADDED) {
		bt_id_del(keys);
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		char key[BT_SETTINGS_KEY_MAX];

		/* Delete stored keys from flash */
		if (keys->id) {
			char id[4];

			u8_to_dec(id, sizeof(id), keys->id);
			bt_settings_encode_key(key, sizeof(key), "keys",
					       &keys->addr, id);
		} else {
			bt_settings_encode_key(key, sizeof(key), "keys",
					       &keys->addr, NULL);
		}

		BT_DBG("Deleting key %s", key);
		settings_delete(key);
	}

	(void)memset(keys, 0, sizeof(*keys));
}

#if defined(CONFIG_BT_SETTINGS)
int bt_keys_store(struct bt_keys *keys)
{
	char key[BT_SETTINGS_KEY_MAX];
	int err;

	__ASSERT_NO_MSG(keys != NULL);

	if (keys->id) {
		char id[4];

		u8_to_dec(id, sizeof(id), keys->id);
		bt_settings_encode_key(key, sizeof(key), "keys", &keys->addr,
				       id);
	} else {
		bt_settings_encode_key(key, sizeof(key), "keys", &keys->addr,
				       NULL);
	}

	err = settings_save_one(key, keys->storage_start, BT_KEYS_STORAGE_LEN);
	if (err) {
		BT_ERR("Failed to save keys (err %d)", err);
		return err;
	}

	BT_DBG("Stored keys for %s (%s)", bt_addr_le_str(&keys->addr),
	       key);

	return 0;
}

static int keys_set(const char *name, size_t len_rd, settings_read_cb read_cb,
		    void *cb_arg)
{
	struct bt_keys *keys;
	bt_addr_le_t addr;
	uint8_t id;
	ssize_t len;
	int err;
	char val[BT_KEYS_STORAGE_LEN];
	const char *next;

	if (!name) {
		BT_ERR("Insufficient number of arguments");
		return -EINVAL;
	}

	len = read_cb(cb_arg, val, sizeof(val));
	if (len < 0) {
		BT_ERR("Failed to read value (err %zd)", len);
		return -EINVAL;
	}

	BT_DBG("name %s val %s", name,
	       (len) ? bt_hex(val, sizeof(val)) : "(null)");

	err = bt_settings_decode_key(name, &addr);
	if (err) {
		BT_ERR("Unable to decode address %s", name);
		return -EINVAL;
	}

	settings_name_next(name, &next);

	if (!next) {
		id = BT_ID_DEFAULT;
	} else {
		unsigned long next_id = strtoul(next, NULL, 10);

		if (next_id >= CONFIG_BT_ID_MAX) {
			BT_ERR("Invalid local identity %lu", next_id);
			return -EINVAL;
		}

		id = (uint8_t)next_id;
	}

	if (!len) {
		keys = bt_keys_find(BT_KEYS_ALL, id, &addr);
		if (keys) {
			(void)memset(keys, 0, sizeof(*keys));
			BT_DBG("Cleared keys for %s", bt_addr_le_str(&addr));
		} else {
			BT_WARN("Unable to find deleted keys for %s",
				bt_addr_le_str(&addr));
		}

		return 0;
	}

	keys = bt_keys_get_addr(id, &addr);
	if (!keys) {
		BT_ERR("Failed to allocate keys for %s", bt_addr_le_str(&addr));
		return -ENOMEM;
	}
	if (len != BT_KEYS_STORAGE_LEN) {
		if (IS_ENABLED(CONFIG_BT_KEYS_OVERWRITE_OLDEST) &&
		    len == BT_KEYS_STORAGE_LEN_COMPAT) {
			/* Load shorter structure for compatibility with old
			 * records format with no counter.
			 */
			BT_WARN("Keys for %s have no aging counter",
				bt_addr_le_str(&addr));
			memcpy(keys->storage_start, val, len);
		} else {
			BT_ERR("Invalid key length %zd != %zu", len,
			       BT_KEYS_STORAGE_LEN);
			bt_keys_clear(keys);

			return -EINVAL;
		}
	} else {
		memcpy(keys->storage_start, val, len);
	}

	BT_DBG("Successfully restored keys for %s", bt_addr_le_str(&addr));
#if IS_ENABLED(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
	if (aging_counter_val < keys->aging_counter) {
		aging_counter_val = keys->aging_counter;
	}
#endif  /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */
	return 0;
}

static void id_add(struct bt_keys *keys, void *user_data)
{
	__ASSERT_NO_MSG(keys != NULL);

	bt_id_add(keys);
}

static int keys_commit(void)
{
	/* We do this in commit() rather than add() since add() may get
	 * called multiple times for the same address, especially if
	 * the keys were already removed.
	 */
	if (IS_ENABLED(CONFIG_BT_CENTRAL) && IS_ENABLED(CONFIG_BT_PRIVACY)) {
		bt_keys_foreach_type(BT_KEYS_ALL, id_add, NULL);
	} else {
		bt_keys_foreach_type(BT_KEYS_IRK, id_add, NULL);
	}

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(bt_keys, "bt/keys", NULL, keys_set, keys_commit,
			       NULL);

#endif /* CONFIG_BT_SETTINGS */

#if IS_ENABLED(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
void bt_keys_update_usage(uint8_t id, const bt_addr_le_t *addr)
{
	__ASSERT_NO_MSG(addr != NULL);

	struct bt_keys *keys = bt_keys_find_addr(id, addr);

	if (!keys) {
		return;
	}

	if (last_keys_updated == keys) {
		return;
	}

	keys->aging_counter = ++aging_counter_val;
	last_keys_updated = keys;

	BT_DBG("Aging counter for %s is set to %u", bt_addr_le_str(addr),
	       keys->aging_counter);

	if (IS_ENABLED(CONFIG_BT_KEYS_SAVE_AGING_COUNTER_ON_PAIRING)) {
		bt_keys_store(keys);
	}
}

#endif  /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */

#if defined(CONFIG_BT_LOG_SNIFFER_INFO)
void bt_keys_show_sniffer_info(struct bt_keys *keys, void *data)
{
	uint8_t ltk[16];

	__ASSERT_NO_MSG(keys != NULL);

	if (keys->keys & BT_KEYS_LTK_P256) {
		sys_memcpy_swap(ltk, keys->ltk.val, keys->enc_size);
		BT_INFO("SC LTK: 0x%s", bt_hex(ltk, keys->enc_size));
	}

#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
	if (keys->keys & BT_KEYS_PERIPH_LTK) {
		sys_memcpy_swap(ltk, keys->periph_ltk.val, keys->enc_size);
		BT_INFO("Legacy LTK: 0x%s (peripheral)",
			bt_hex(ltk, keys->enc_size));
	}
#endif /* !CONFIG_BT_SMP_SC_PAIR_ONLY */

	if (keys->keys & BT_KEYS_LTK) {
		sys_memcpy_swap(ltk, keys->ltk.val, keys->enc_size);
		BT_INFO("Legacy LTK: 0x%s (central)",
			bt_hex(ltk, keys->enc_size));
	}
}
#endif /* defined(CONFIG_BT_LOG_SNIFFER_INFO) */

#ifdef ZTEST_UNITTEST
struct bt_keys *bt_keys_get_key_pool(void)
{
	return key_pool;
}

#if IS_ENABLED(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
uint32_t bt_keys_get_aging_counter_val(void)
{
	return aging_counter_val;
}
#endif /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */
#endif /* ZTEST_UNITTEST */
