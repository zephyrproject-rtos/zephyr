/* keys.c - Bluetooth key handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util_macro.h>

#include "common/bt_str.h"
#include "conn_internal.h"
#include "gatt_internal.h"
#include "hci_core.h"
#include "id.h"
#include "keys.h"
#include "keys_psa.h"
#include "settings.h"
#include "smp.h"
#include "sys/types.h"

#define LOG_LEVEL CONFIG_BT_KEYS_LOG_LEVEL
LOG_MODULE_REGISTER(bt_keys);

static struct bt_keys key_pool[CONFIG_BT_MAX_PAIRED];

#define STORAGE_CFG_VERSION		17U
BUILD_ASSERT(STORAGE_CFG_VERSION <= UINT8_MAX, "STORAGE_CFG_VERSION is too large");
/* Configuration flags for storage. Based on the bt_keys_cfg_flags enum. */
#define STORAGE_CFG_FLAGS                                                                          \
	((IS_ENABLED(CONFIG_BT_SIGNING) ? BT_KEYS_CFG_SIGNING : 0) |                               \
	 (IS_ENABLED(CONFIG_BT_SMP_SC_PAIR_ONLY) ? BT_KEYS_CFG_SC_PAIR_ONLY : 0) |                 \
	 (IS_ENABLED(CONFIG_BT_KEYS_OVERWRITE_OLDEST) ? BT_KEYS_CFG_OVERWRITE_OLDEST : 0))
BUILD_ASSERT(STORAGE_CFG_FLAGS < BIT(24), "STORAGE_CFG_FLAGS is too large");

#define KEYS_SECRET_COUNT (IS_ENABLED(CONFIG_BT_SMP_SC_PAIR_ONLY) ? 2 : 3)

#if defined(CONFIG_BT_SETTINGS)
struct keys_secret {
	/* Whether the key type of this secret is set in `bt_keys.keys` */
	bool present;
	uint8_t *val;
	psa_key_id_t *psa_id;
};

static void keys_secrets_get(struct bt_keys *keys, struct keys_secret secrets[KEYS_SECRET_COUNT])
{
	secrets[0] = (struct keys_secret){
		.present = (keys->keys & (BT_KEYS_LTK | BT_KEYS_LTK_P256)) != 0U,
		.val = keys->ltk.val,
		.psa_id = &keys->ltk_psa_id,
	};
	secrets[1] = (struct keys_secret){
		.present = (keys->keys & BT_KEYS_IRK) != 0U,
		.val = keys->irk.val,
		.psa_id = &keys->irk_psa_id,
	};
#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
	secrets[2] = (struct keys_secret){
		.present = (keys->keys & BT_KEYS_PERIPH_LTK) != 0U,
		.val = keys->periph_ltk.val,
		.psa_id = &keys->periph_ltk_psa_id,
	};
#endif /* !CONFIG_BT_SMP_SC_PAIR_ONLY */
}

/* Stage the live secret `cur` in PSA storage and set its key ID in the secret `out` of the record
 * to be stored.
 *
 * A changed value is stored in a new PSA key. The value in `out` is cleared, except when an error
 * is returned.
 */
static int keys_secret_psa_stage(const struct keys_secret *cur, const struct keys_secret *out)
{
	int err;

	if (!cur->present) {
		*out->psa_id = PSA_KEY_ID_NULL;
	} else if (*cur->psa_id == PSA_KEY_ID_NULL ||
		   !bt_keys_psa_matches(*cur->psa_id, cur->val)) {
		err = bt_keys_psa_put(cur->val, out->psa_id);
		if (err != 0) {
			*out->psa_id = *cur->psa_id;
			return err;
		}
	}

	(void)memset(out->val, 0, BT_KEYS_PSA_KEY_SIZE);

	return 0;
}

/* Destroy the PSA key that is no longer needed and keep the other one in the live secret `cur`.
 *
 * If the record was `committed`, the replaced keys are obsolete, otherwise the new ones are.
 */
static void keys_secret_psa_finalize(const struct keys_secret *cur, const struct keys_secret *out,
				     bool committed)
{
	const psa_key_id_t obsolete = committed ? *cur->psa_id : *out->psa_id;
	const psa_key_id_t kept = committed ? *out->psa_id : *cur->psa_id;

	if (obsolete != kept) {
		bt_keys_psa_del(obsolete);
	}

	*cur->psa_id = kept;
}

/* Write the record of `keys` to settings with the values replaced by PSA key IDs.
 *
 * A changed value is stored in a new PSA key. The replaced PSA keys are only destroyed after the
 * record is written, so a failure at any point leaves the previous record usable.
 */
static int keys_psa_store(struct bt_keys *keys)
{
	struct bt_keys rec = *keys;
	struct keys_secret cur[KEYS_SECRET_COUNT];
	struct keys_secret out[KEYS_SECRET_COUNT];
	int err = 0;
	size_t i;

	keys_secrets_get(keys, cur);
	keys_secrets_get(&rec, out);

	for (i = 0; i < KEYS_SECRET_COUNT; i++) {
		err = keys_secret_psa_stage(&cur[i], &out[i]);
		if (err != 0) {
			break;
		}
	}

	if (err == 0) {
		err = bt_settings_store_keys(rec.id, &rec.addr, rec.storage_start,
					     BT_KEYS_STORAGE_LEN);
	}

	for (i = 0; i < KEYS_SECRET_COUNT; i++) {
		keys_secret_psa_finalize(&cur[i], &out[i], err == 0);
	}

	/* `rec` holds the values of the secrets that were not cleared if the loop was left early */
	(void)memset(&rec, 0, sizeof(rec));

	return err;
}

/* Read the values of the PSA keys referenced by the stored record of `keys` */
static int keys_psa_load(struct bt_keys *keys)
{
	struct keys_secret secrets[KEYS_SECRET_COUNT];
	int err;

	keys_secrets_get(keys, secrets);

	for (size_t i = 0; i < KEYS_SECRET_COUNT; i++) {
		if (!secrets[i].present) {
			bt_keys_psa_del(*secrets[i].psa_id);
			*secrets[i].psa_id = PSA_KEY_ID_NULL;
			continue;
		}

		if (*secrets[i].psa_id == PSA_KEY_ID_NULL) {
			return -ENOENT;
		}

		err = bt_keys_psa_get(*secrets[i].psa_id, secrets[i].val);
		if (err != 0) {
			return err;
		}
	}

	return 0;
}

static void keys_psa_clear(struct bt_keys *keys)
{
	struct keys_secret secrets[KEYS_SECRET_COUNT];

	keys_secrets_get(keys, secrets);

	for (size_t i = 0; i < KEYS_SECRET_COUNT; i++) {
		*secrets[i].psa_id = PSA_KEY_ID_NULL;
	}
}

static void keys_psa_del(struct bt_keys *keys)
{
	struct keys_secret secrets[KEYS_SECRET_COUNT];

	keys_secrets_get(keys, secrets);

	for (size_t i = 0; i < KEYS_SECRET_COUNT; i++) {
		bt_keys_psa_del(*secrets[i].psa_id);
		*secrets[i].psa_id = PSA_KEY_ID_NULL;
	}
}
#endif /* CONFIG_BT_SETTINGS */

#if defined(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
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
			LOG_DBG("Connected device %s is using key_pool[%d]",
				bt_conn_dst_str(conn), kdata->id);
		}
	}
}

static bool key_is_in_use(uint8_t id)
{
	struct key_data kdata = { false, id };

	bt_conn_foreach(BT_CONN_TYPE_LE, find_key_in_use, &kdata);

	return kdata.in_use;
}
#endif /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */

void bt_keys_reset(void)
{
	memset(key_pool, 0, sizeof(key_pool));

	/* The PSA keys stay in the secure storage, they are in use again once loaded. */
	bt_keys_psa_reset();
}

bool bt_keys_has_bond(uint8_t id)
{
	for (size_t i = 0U; i < ARRAY_SIZE(key_pool); i++) {
		if ((key_pool[i].keys != 0U) && (key_pool[i].id == id)) {
			return true;
		}
	}

	return false;
}

struct bt_keys *bt_keys_get_addr(uint8_t id, const bt_addr_le_t *addr)
{
	struct bt_keys *keys;
	int i;
	size_t first_free_slot = ARRAY_SIZE(key_pool);

	__ASSERT_NO_MSG(addr != NULL);

	LOG_DBG("%s", bt_addr_le_str(addr));

	for (i = 0; i < ARRAY_SIZE(key_pool); i++) {
		keys = &key_pool[i];

		if (keys->id == id && bt_addr_le_eq(&keys->addr, addr)) {
			return keys;
		}
		if (first_free_slot == ARRAY_SIZE(key_pool) &&
		    bt_addr_le_eq(&keys->addr, BT_ADDR_LE_ANY)) {
			first_free_slot = i;
		}
	}

#if defined(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
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
			LOG_DBG("unable to create keys for %s", bt_addr_le_str(addr));
			return NULL;
		}

		/* Use a copy as bt_unpair will clear the oldest key. */
		bt_addr_le_copy(&oldest_addr, &oldest->addr);
		bt_unpair(oldest->id, &oldest_addr);
		if (bt_addr_le_eq(&oldest->addr, BT_ADDR_LE_ANY)) {
			first_free_slot = oldest - &key_pool[0];
		}
	}

#endif  /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */
	if (first_free_slot < ARRAY_SIZE(key_pool)) {
		keys = &key_pool[first_free_slot];
		keys->id = id;
		bt_addr_le_copy(&keys->addr, addr);
		keys->cfg_version = STORAGE_CFG_VERSION;
		sys_put_le24(STORAGE_CFG_FLAGS, keys->cfg_flags);
#if defined(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
		keys->aging_counter = ++aging_counter_val;
		last_keys_updated = keys;
#endif  /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */
		LOG_DBG("created %p for %s", keys, bt_addr_le_str(addr));
		return keys;
	}

	LOG_DBG("unable to create keys for %s", bt_addr_le_str(addr));

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

enum bt_le_addr_res_support bt_le_bond_addr_res_support(uint8_t id, const bt_addr_le_t *peer)
{
	struct bt_keys *keys;

	__ASSERT_NO_MSG(peer != NULL);

	if (!IS_ENABLED(CONFIG_BT_GATT_AUTO_READ_CENTRAL_ADDR_RES)) {
		/* Also ignores an answer stored by a previous firmware that
		 * had the option enabled.
		 */
		return BT_LE_ADDR_RES_SUPPORT_UNKNOWN;
	}

	keys = bt_keys_find_addr(id, peer);
	if (keys == NULL || (keys->flags & BT_KEYS_CENTRAL_ADDR_RES_KNOWN) == 0) {
		return BT_LE_ADDR_RES_SUPPORT_UNKNOWN;
	}

	if ((keys->flags & BT_KEYS_CENTRAL_ADDR_RES_SUPPORT) != 0) {
		return BT_LE_ADDR_RES_SUPPORT_YES;
	}

	return BT_LE_ADDR_RES_SUPPORT_NO;
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

	LOG_DBG("type %d %s", type, bt_addr_le_str(addr));

	for (i = 0; i < ARRAY_SIZE(key_pool); i++) {
		if ((key_pool[i].keys & type) && key_pool[i].id == id &&
		    bt_addr_le_eq(&key_pool[i].addr, addr)) {
			return &key_pool[i];
		}
	}

	return NULL;
}

struct bt_keys *bt_keys_get_type(enum bt_keys_type type, uint8_t id, const bt_addr_le_t *addr)
{
	struct bt_keys *keys;

	__ASSERT_NO_MSG(addr != NULL);

	LOG_DBG("type %d %s", type, bt_addr_le_str(addr));

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

	LOG_DBG("%s", bt_addr_le_str(addr));

	if (!bt_addr_le_is_rpa(addr)) {
		return NULL;
	}

	for (i = 0; i < ARRAY_SIZE(key_pool); i++) {
		if (!(key_pool[i].keys & BT_KEYS_IRK)) {
			continue;
		}

		if (key_pool[i].id == id &&
		    bt_addr_eq(&addr->a, &key_pool[i].irk.rpa)) {
			LOG_DBG("cached RPA %s for %s", bt_addr_str(&key_pool[i].irk.rpa),
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

		if (bt_addr_matches_irk(&addr->a, key_pool[i].irk.val)) {
			LOG_DBG("RPA %s matches %s", bt_addr_str(&key_pool[i].irk.rpa),
				bt_addr_le_str(&key_pool[i].addr));

			bt_addr_copy(&key_pool[i].irk.rpa, &addr->a);

			return &key_pool[i];
		}
	}

	LOG_DBG("No IRK for %s", bt_addr_le_str(addr));

	return NULL;
}

struct bt_keys *bt_keys_find_addr(uint8_t id, const bt_addr_le_t *addr)
{
	int i;

	__ASSERT_NO_MSG(addr != NULL);

	LOG_DBG("%s", bt_addr_le_str(addr));

	for (i = 0; i < ARRAY_SIZE(key_pool); i++) {
		if (key_pool[i].id == id &&
		    bt_addr_le_eq(&key_pool[i].addr, addr)) {
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

	LOG_DBG("%s (keys 0x%04x)", bt_addr_le_str(&keys->addr), keys->keys);

	if (keys->state & BT_KEYS_ID_ADDED) {
		bt_id_del(keys);
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		/* Delete stored keys from flash */
		bt_settings_delete_keys(keys->id, &keys->addr);

		/* After the record is deleted, so a failure leaves an unused key, not a dangling ID. */
		keys_psa_del(keys);
	}


	(void)memset(keys, 0, sizeof(*keys));
}

#if defined(CONFIG_BT_SETTINGS)
int bt_keys_store(struct bt_keys *keys)
{
	int err;

	__ASSERT_NO_MSG(keys != NULL);

	err = keys_psa_store(keys);
	if (err) {
		LOG_ERR("Failed to save keys (err %d)", err);
		return err;
	}

	LOG_DBG("Stored keys for %s", bt_addr_le_str(&keys->addr));

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
		LOG_ERR("Insufficient number of arguments");
		return -EINVAL;
	}

	len = read_cb(cb_arg, val, sizeof(val));
	if (len < 0) {
		LOG_ERR("Failed to read value (err %zd)", len);
		return -EINVAL;
	}

	LOG_DBG("name %s val %s", name, (len) ? bt_hex(val, sizeof(val)) : "(null)");

	err = bt_settings_decode_key(name, &addr);
	if (err) {
		LOG_ERR("Unable to decode address %s", name);
		return -EINVAL;
	}

	settings_name_next(name, &next);

	if (!next) {
		id = BT_ID_DEFAULT;
	} else {
		unsigned long next_id = strtoul(next, NULL, 10);

		if (next_id >= CONFIG_BT_ID_MAX) {
			LOG_ERR("Invalid local identity %lu", next_id);
			return -EINVAL;
		}

		id = (uint8_t)next_id;
	}

	if (!len) {
		keys = bt_keys_find(BT_KEYS_ALL, id, &addr);
		if (keys) {
			keys_psa_del(keys);
			(void)memset(keys, 0, sizeof(*keys));
			LOG_DBG("Cleared keys for %s", bt_addr_le_str(&addr));
		} else {
			LOG_WRN("Unable to find deleted keys for %s", bt_addr_le_str(&addr));
		}

		return 0;
	}

	keys = bt_keys_get_addr(id, &addr);
	if (!keys) {
		LOG_ERR("Failed to allocate keys for %s", bt_addr_le_str(&addr));
		return -ENOMEM;
	}
	/* The older formats store the values of the keys, they cannot be loaded from PSA keys. */
	if (len != BT_KEYS_STORAGE_LEN) {
		LOG_ERR("Invalid key length %zd != %zu", len, BT_KEYS_STORAGE_LEN);
		bt_keys_clear(keys);

		return -EINVAL;
	}

	memcpy(keys->storage_start, val, len);

	/* Some Kconfig options can change the size of the keys structure. This check will clear
	 * the stored keys if the config flags are not matching between firmware updates.
	 */
	if ((keys->cfg_version != STORAGE_CFG_VERSION) ||
	    (sys_get_le24(keys->cfg_flags) != STORAGE_CFG_FLAGS)) {
		LOG_ERR("Stored keys for %s do not match current config flags or version",
			bt_addr_le_str(&addr));
		keys_psa_clear(keys);
		bt_keys_clear(keys);
		return -EINVAL;
	}

	err = keys_psa_load(keys);
	if (err != 0) {
		LOG_ERR("Unable to load the PSA keys of %s (err %d)", bt_addr_le_str(&addr), err);
		bt_keys_clear(keys);
		return -EINVAL;
	}

	/* As of Core v6.2, authenticated keys are only valid for OOB or LE SC pairing
	 * methods. This check ensures that keys are valid if a device is updated from a
	 * previous version that did not enforce this requirement.
	 */
	if ((keys->flags & BT_KEYS_AUTHENTICATED) &&
	    !(keys->flags & (BT_KEYS_OOB | BT_KEYS_SC))) {
		LOG_WRN("The keys for %s are downgraded to unauthenticated as they no longer meet "
			"authentication requirements",
			bt_addr_le_str(&addr));
		keys->flags &= ~BT_KEYS_AUTHENTICATED;
	}

	LOG_DBG("Successfully restored keys for %s", bt_addr_le_str(&addr));
#if defined(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
	if (aging_counter_val < keys->aging_counter) {
		aging_counter_val = keys->aging_counter;
	}
#endif  /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */
	return 0;
}

static void id_add(struct bt_keys *keys, void *user_data)
{
	__ASSERT_NO_MSG(keys != NULL);

	if ((keys->state & BT_KEYS_ID_ADDED) != 0U) {
		return;
	}

	/* Only mark as pending here. Keys are added to the controller RL on
	 * demand when advertising or scanning starts, avoiding bt_id_add()
	 * blocking settings_load() on HCI.
	 */
	bt_id_pending_keys_update_set(keys, BT_KEYS_ID_PENDING_ADD);
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

BT_SETTINGS_DEFINE(keys, "keys", keys_set, keys_commit);

#endif /* CONFIG_BT_SETTINGS */

#if defined(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
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

	LOG_DBG("Aging counter for %s is set to %u", bt_addr_le_str(addr), keys->aging_counter);

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
		LOG_INF("SC LTK: 0x%s", bt_hex(ltk, keys->enc_size));
	}

#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
	if (keys->keys & BT_KEYS_PERIPH_LTK) {
		sys_memcpy_swap(ltk, keys->periph_ltk.val, keys->enc_size);
		LOG_INF("Legacy LTK: 0x%s (peripheral)", bt_hex(ltk, keys->enc_size));
	}
#endif /* !CONFIG_BT_SMP_SC_PAIR_ONLY */

	if (keys->keys & BT_KEYS_LTK) {
		sys_memcpy_swap(ltk, keys->ltk.val, keys->enc_size);
		LOG_INF("Legacy LTK: 0x%s (central)", bt_hex(ltk, keys->enc_size));
	}
}
#endif /* defined(CONFIG_BT_LOG_SNIFFER_INFO) */

#ifdef ZTEST_UNITTEST
struct bt_keys *bt_keys_get_key_pool(void)
{
	return key_pool;
}

#if defined(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
uint32_t bt_keys_get_aging_counter_val(void)
{
	return aging_counter_val;
}

struct bt_keys *bt_keys_get_last_keys_updated(void)
{
	return last_keys_updated;
}
#endif /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */
#endif /* ZTEST_UNITTEST */
