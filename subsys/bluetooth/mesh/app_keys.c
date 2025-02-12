/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdlib.h>
#include <zephyr/bluetooth/mesh.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/iterable_sections.h>

#include "mesh.h"
#include "net.h"
#include "app_keys.h"
#include "rpl.h"
#include "settings.h"
#include "crypto.h"
#include "proxy.h"
#include "friend.h"
#include "foundation.h"
#include "access.h"

#include "common/bt_str.h"

#define LOG_LEVEL CONFIG_BT_MESH_KEYS_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_app_keys);

/* Tracking of what storage changes are pending for App Keys. We track this in
 * a separate array here instead of within the respective bt_mesh_app_key
 * struct itself, since once a key gets deleted its struct becomes invalid
 * and may be reused for other keys.
 */
struct app_key_update {
	uint16_t key_idx:12,    /* AppKey Index */
		 valid:1,       /* 1 if this entry is valid, 0 if not */
		 clear:1;       /* 1 if key needs clearing, 0 if storing */
};

/* AppKey information for persistent storage. */
struct app_key_val {
	uint16_t net_idx;
	bool updated;
	struct bt_mesh_key val[2];
} __packed;

/** Mesh Application Key. */
struct app_key {
	uint16_t net_idx;
	uint16_t app_idx;
	bool updated;
	struct bt_mesh_app_cred {
		uint8_t id;
		struct bt_mesh_key val;
	} keys[2];
};

static struct app_key_update app_key_updates[CONFIG_BT_MESH_APP_KEY_COUNT];

static struct app_key apps[CONFIG_BT_MESH_APP_KEY_COUNT] = {
	[0 ... (CONFIG_BT_MESH_APP_KEY_COUNT - 1)] = {
		.app_idx = BT_MESH_KEY_UNUSED,
		.net_idx = BT_MESH_KEY_UNUSED,
	}
};

static struct app_key *app_get(uint16_t app_idx)
{
	for (int i = 0; i < ARRAY_SIZE(apps); i++) {
		if (apps[i].app_idx == app_idx) {
			return &apps[i];
		}
	}

	return NULL;
}

static void clear_app_key(uint16_t app_idx)
{
	char path[20];
	int err;

	snprintk(path, sizeof(path), "bt/mesh/AppKey/%x", app_idx);
	err = settings_delete(path);
	if (err) {
		LOG_ERR("Failed to clear AppKeyIndex 0x%03x", app_idx);
	} else {
		LOG_DBG("Cleared AppKeyIndex 0x%03x", app_idx);
	}
}

static void store_app_key(uint16_t app_idx)
{
	const struct app_key *app;
	struct app_key_val key;
	char path[20];
	int err;

	snprintk(path, sizeof(path), "bt/mesh/AppKey/%x", app_idx);

	app = app_get(app_idx);
	if (!app) {
		LOG_WRN("ApKeyIndex 0x%03x not found", app_idx);
		return;
	}

	key.net_idx = app->net_idx,
	key.updated = app->updated,

	memcpy(&key.val[0], &app->keys[0].val, sizeof(struct bt_mesh_key));
	memcpy(&key.val[1], &app->keys[1].val, sizeof(struct bt_mesh_key));

	err = settings_save_one(path, &key, sizeof(key));
	if (err) {
		LOG_ERR("Failed to store AppKey %s value", path);
	} else {
		LOG_DBG("Stored AppKey %s value", path);
	}
}

static struct app_key_update *app_key_update_find(uint16_t key_idx,
					struct app_key_update **free_slot)
{
	struct app_key_update *match;
	int i;

	match = NULL;
	*free_slot = NULL;

	for (i = 0; i < ARRAY_SIZE(app_key_updates); i++) {
		struct app_key_update *update = &app_key_updates[i];

		if (!update->valid) {
			*free_slot = update;
			continue;
		}

		if (update->key_idx == key_idx) {
			match = update;
		}
	}

	return match;
}

static void update_app_key_settings(uint16_t app_idx, bool store)
{
	struct app_key_update *update, *free_slot;
	uint8_t clear = store ? 0U : 1U;

	LOG_DBG("AppKeyIndex 0x%03x", app_idx);

	update = app_key_update_find(app_idx, &free_slot);
	if (update) {
		update->clear = clear;
		bt_mesh_settings_store_schedule(
					BT_MESH_SETTINGS_APP_KEYS_PENDING);
		return;
	}

	if (!free_slot) {
		if (store) {
			store_app_key(app_idx);
		} else {
			clear_app_key(app_idx);
		}
		return;
	}

	free_slot->valid = 1U;
	free_slot->key_idx = app_idx;
	free_slot->clear = clear;

	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_APP_KEYS_PENDING);
}

static void app_key_evt(struct app_key *app, enum bt_mesh_key_evt evt)
{
	STRUCT_SECTION_FOREACH(bt_mesh_app_key_cb, cb) {
		cb->evt_handler(app->app_idx, app->net_idx, evt);
	}
}

static struct app_key *app_key_alloc(uint16_t app_idx)
{
	struct app_key *app = NULL;

	for (int i = 0; i < ARRAY_SIZE(apps); i++) {
		/* Check for already existing app_key */
		if (apps[i].app_idx == app_idx) {
			return &apps[i];
		}

		if (!app && apps[i].app_idx == BT_MESH_KEY_UNUSED) {
			app = &apps[i];
		}
	}

	return app;
}

static void app_key_del(struct app_key *app)
{
	LOG_DBG("AppIdx 0x%03x", app->app_idx);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		update_app_key_settings(app->app_idx, false);
	}

	app_key_evt(app, BT_MESH_KEY_DELETED);

	app->net_idx = BT_MESH_KEY_UNUSED;
	app->app_idx = BT_MESH_KEY_UNUSED;
	bt_mesh_key_destroy(&app->keys[0].val);
	bt_mesh_key_destroy(&app->keys[1].val);
	memset(app->keys, 0, sizeof(app->keys));
}

static void app_key_revoke(struct app_key *app)
{
	if (!app->updated) {
		return;
	}

	bt_mesh_key_destroy(&app->keys[0].val);
	memcpy(&app->keys[0], &app->keys[1], sizeof(app->keys[0]));
	memset(&app->keys[1], 0, sizeof(app->keys[1]));
	app->updated = false;

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		update_app_key_settings(app->app_idx, true);
	}

	app_key_evt(app, BT_MESH_KEY_REVOKED);
}

uint8_t bt_mesh_app_key_add(uint16_t app_idx, uint16_t net_idx,
			const uint8_t key[16])
{
	struct app_key *app;

	LOG_DBG("net_idx 0x%04x app_idx %04x val %s", net_idx, app_idx, bt_hex(key, 16));

	if (!bt_mesh_subnet_get(net_idx)) {
		return STATUS_INVALID_NETKEY;
	}

	app = app_key_alloc(app_idx);
	if (!app) {
		return STATUS_INSUFF_RESOURCES;
	}

	if (app->app_idx == app_idx) {
		if (app->net_idx != net_idx) {
			return STATUS_INVALID_NETKEY;
		}

		if (bt_mesh_key_compare(key, &app->keys[0].val)) {
			return STATUS_IDX_ALREADY_STORED;
		}

		return STATUS_SUCCESS;
	}

	if (bt_mesh_app_id(key, &app->keys[0].id)) {
		return STATUS_CANNOT_SET;
	}

	LOG_DBG("AppIdx 0x%04x AID 0x%02x", app_idx, app->keys[0].id);

	app->net_idx = net_idx;
	app->app_idx = app_idx;
	app->updated = false;
	if (bt_mesh_key_import(BT_MESH_KEY_TYPE_APP, key, &app->keys[0].val)) {
		LOG_ERR("Unable to import application key");
		return STATUS_CANNOT_SET;
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		LOG_DBG("Storing AppKey persistently");
		update_app_key_settings(app->app_idx, true);
	}

	app_key_evt(app, BT_MESH_KEY_ADDED);

	return STATUS_SUCCESS;
}

uint8_t bt_mesh_app_key_update(uint16_t app_idx, uint16_t net_idx,
			       const uint8_t key[16])
{
	struct app_key *app;
	struct bt_mesh_subnet *sub;

	LOG_DBG("net_idx 0x%04x app_idx %04x val %s", net_idx, app_idx, bt_hex(key, 16));

	app = app_get(app_idx);
	if (!app) {
		return STATUS_INVALID_APPKEY;
	}

	if (net_idx != BT_MESH_KEY_UNUSED && app->net_idx != net_idx) {
		return STATUS_INVALID_BINDING;
	}

	sub = bt_mesh_subnet_get(app->net_idx);
	if (!sub) {
		return STATUS_INVALID_NETKEY;
	}

	/* The AppKey Update message shall generate an error when node
	 * is in normal operation, Phase 2, or Phase 3 or in Phase 1
	 * when the AppKey Update message on a valid AppKeyIndex when
	 * the AppKey value is different.
	 */
	if (sub->kr_phase != BT_MESH_KR_PHASE_1) {
		return STATUS_CANNOT_UPDATE;
	}

	if (app->updated) {
		if (bt_mesh_key_compare(key, &app->keys[1].val)) {
			return STATUS_IDX_ALREADY_STORED;
		}

		return STATUS_SUCCESS;
	}

	if (bt_mesh_app_id(key, &app->keys[1].id)) {
		return STATUS_CANNOT_UPDATE;
	}

	LOG_DBG("app_idx 0x%04x AID 0x%02x", app_idx, app->keys[1].id);

	app->updated = true;
	if (bt_mesh_key_import(BT_MESH_KEY_TYPE_APP, key, &app->keys[1].val)) {
		LOG_ERR("Unable to import application key");
		return STATUS_CANNOT_UPDATE;
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		LOG_DBG("Storing AppKey persistently");
		update_app_key_settings(app->app_idx, true);
	}

	app_key_evt(app, BT_MESH_KEY_UPDATED);

	return STATUS_SUCCESS;
}

uint8_t bt_mesh_app_key_del(uint16_t app_idx, uint16_t net_idx)
{
	struct app_key *app;

	LOG_DBG("AppIdx 0x%03x", app_idx);

	if (net_idx != BT_MESH_KEY_UNUSED && !bt_mesh_subnet_get(net_idx)) {
		return STATUS_INVALID_NETKEY;
	}

	app = app_get(app_idx);
	if (!app) {
		/* This could be a retry of a previous attempt that had its
		 * response lost, so pretend that it was a success.
		 */
		return STATUS_SUCCESS;
	}

	if (net_idx != BT_MESH_KEY_UNUSED && net_idx != app->net_idx) {
		return STATUS_INVALID_BINDING;
	}

	app_key_del(app);

	return STATUS_SUCCESS;
}

static int app_id_set(struct app_key *app, int key_idx, const struct bt_mesh_key *key)
{
	uint8_t raw_key[16];
	int err;

	err = bt_mesh_key_export(raw_key, key);
	if (err) {
		return err;
	}

	err = bt_mesh_app_id(raw_key, &app->keys[key_idx].id);
	if (err) {
		return err;
	}

	bt_mesh_key_assign(&app->keys[key_idx].val, key);

	return 0;
}

int bt_mesh_app_key_set(uint16_t app_idx, uint16_t net_idx,
			const struct bt_mesh_key *old_key, const struct bt_mesh_key *new_key)
{
	struct app_key *app;

	app = app_key_alloc(app_idx);
	if (!app) {
		return -ENOMEM;
	}

	if (app->app_idx == app_idx) {
		return 0;
	}

	LOG_DBG("AppIdx 0x%04x AID 0x%02x", app_idx, app->keys[0].id);

	if (app_id_set(app, 0, old_key)) {
		return -EIO;
	}

	if (new_key != NULL && app_id_set(app, 1, new_key)) {
		return -EIO;
	}

	app->net_idx = net_idx;
	app->app_idx = app_idx;
	app->updated = !!new_key;

	return 0;
}

bool bt_mesh_app_key_exists(uint16_t app_idx)
{
	for (int i = 0; i < ARRAY_SIZE(apps); i++) {
		if (apps[i].app_idx == app_idx) {
			return true;
		}
	}

	return false;
}

ssize_t bt_mesh_app_keys_get(uint16_t net_idx, uint16_t app_idxs[], size_t max,
			     off_t skip)
{
	size_t count = 0;

	for (int i = 0; i < ARRAY_SIZE(apps); i++) {
		struct app_key *app = &apps[i];

		if (app->app_idx == BT_MESH_KEY_UNUSED) {
			continue;
		}

		if (net_idx != BT_MESH_KEY_ANY && app->net_idx != net_idx) {
			continue;
		}

		if (skip) {
			skip--;
			continue;
		}

		if (count >= max) {
			return -ENOMEM;
		}

		app_idxs[count++] = app->app_idx;
	}

	return count;
}

int bt_mesh_keys_resolve(struct bt_mesh_msg_ctx *ctx,
			 struct bt_mesh_subnet **sub,
			 const struct bt_mesh_key **app_key, uint8_t *aid)
{
	struct app_key *app = NULL;

	if (BT_MESH_IS_DEV_KEY(ctx->app_idx)) {
		/* With device keys, the application has to decide which subnet
		 * to send on.
		 */
		*sub = bt_mesh_subnet_get(ctx->net_idx);
		if (!*sub) {
			LOG_WRN("Unknown NetKey 0x%03x", ctx->net_idx);
			return -EINVAL;
		}

		if (ctx->app_idx == BT_MESH_KEY_DEV_REMOTE &&
		    !bt_mesh_has_addr(ctx->addr)) {
			struct bt_mesh_cdb_node *node;

			if (!IS_ENABLED(CONFIG_BT_MESH_CDB)) {
				LOG_WRN("No DevKey for 0x%04x", ctx->addr);
				return -EINVAL;
			}

			node = bt_mesh_cdb_node_get(ctx->addr);
			if (!node) {
				LOG_WRN("No DevKey for 0x%04x", ctx->addr);
				return -EINVAL;
			}

			*app_key = &node->dev_key;
		} else {
			*app_key = &bt_mesh.dev_key;
		}

		*aid = 0;
		return 0;
	}

	app = app_get(ctx->app_idx);
	if (!app) {
		LOG_WRN("Unknown AppKey 0x%03x", ctx->app_idx);
		return -EINVAL;
	}

	*sub = bt_mesh_subnet_get(app->net_idx);
	if (!*sub) {
		LOG_WRN("Unknown NetKey 0x%03x", app->net_idx);
		return -EINVAL;
	}

	if ((*sub)->kr_phase == BT_MESH_KR_PHASE_2 && app->updated) {
		*aid = app->keys[1].id;
		*app_key = &app->keys[1].val;
	} else {
		*aid = app->keys[0].id;
		*app_key = &app->keys[0].val;
	}

	return 0;
}

uint16_t bt_mesh_app_key_find(bool dev_key, uint8_t aid,
			      struct bt_mesh_net_rx *rx,
			      int (*cb)(struct bt_mesh_net_rx *rx,
					const struct bt_mesh_key *key, void *cb_data),
			      void *cb_data)
{
	int err, i;

	if (dev_key) {
		/* Attempt remote dev key first, as that is only available for
		 * provisioner devices, which normally don't interact with nodes
		 * that know their local dev key.
		 */
		if (IS_ENABLED(CONFIG_BT_MESH_CDB) &&
		    rx->net_if != BT_MESH_NET_IF_LOCAL) {
			struct bt_mesh_cdb_node *node;

			node = bt_mesh_cdb_node_get(rx->ctx.addr);
			if (node && !cb(rx, &node->dev_key, cb_data)) {
				return BT_MESH_KEY_DEV_REMOTE;
			}
		}

		/** MshPRTv1.1: 3.4.3:
		 *  The Device key is only valid for unicast addresses.
		 */
		if (BT_MESH_ADDR_IS_UNICAST(rx->ctx.recv_dst)) {
			err = cb(rx, &bt_mesh.dev_key, cb_data);
			if (!err) {
				return BT_MESH_KEY_DEV_LOCAL;
			}

#if defined(CONFIG_BT_MESH_RPR_SRV)
			if (atomic_test_bit(bt_mesh.flags, BT_MESH_DEVKEY_CAND)) {
				err = cb(rx, &bt_mesh.dev_key_cand, cb_data);
				if (!err) {
					/* MshPRTv1.1: 3.6.4.2:
					 * If a message is successfully decrypted using the device
					 * key candidate, the device key candidate should
					 * permanently replace the original devkey.
					 */
					bt_mesh_dev_key_cand_activate();
					return BT_MESH_KEY_DEV_LOCAL;
				}
			}
#endif
		}

		return BT_MESH_KEY_UNUSED;
	}

	for (i = 0; i < ARRAY_SIZE(apps); i++) {
		const struct app_key *app = &apps[i];
		const struct bt_mesh_app_cred *cred;

		if (app->app_idx == BT_MESH_KEY_UNUSED) {
			continue;
		}

		if (app->net_idx != rx->sub->net_idx) {
			continue;
		}

		if (rx->new_key && app->updated) {
			cred = &app->keys[1];
		} else {
			cred = &app->keys[0];
		}

		if (cred->id != aid) {
			continue;
		}

		err = cb(rx, &cred->val, cb_data);
		if (err) {
			continue;
		}

		return app->app_idx;
	}

	return BT_MESH_KEY_UNUSED;
}

static void subnet_evt(struct bt_mesh_subnet *sub, enum bt_mesh_key_evt evt)
{
	if (evt == BT_MESH_KEY_UPDATED || evt == BT_MESH_KEY_ADDED) {
		return;
	}

	for (int i = 0; i < ARRAY_SIZE(apps); i++) {
		struct app_key *app = &apps[i];

		if (app->app_idx == BT_MESH_KEY_UNUSED) {
			continue;
		}

		if (app->net_idx != sub->net_idx) {
			continue;
		}

		if (evt == BT_MESH_KEY_DELETED) {
			app_key_del(app);
		} else if (evt == BT_MESH_KEY_REVOKED) {
			app_key_revoke(app);
		} else if (evt == BT_MESH_KEY_SWAPPED && app->updated) {
			app_key_evt(app, BT_MESH_KEY_SWAPPED);
		}
	}
}

BT_MESH_SUBNET_CB_DEFINE(app_keys) = {
	.evt_handler = subnet_evt,
};

void bt_mesh_app_keys_reset(void)
{
	for (int i = 0; i < ARRAY_SIZE(apps); i++) {
		struct app_key *app = &apps[i];

		if (app->app_idx != BT_MESH_KEY_UNUSED) {
			app_key_del(app);
		}
	}
}

static int app_key_set(const char *name, size_t len_rd,
		       settings_read_cb read_cb, void *cb_arg)
{
	struct app_key_val key;
	struct bt_mesh_key val[2];
	uint16_t app_idx;
	int err;

	if (!name) {
		LOG_ERR("Insufficient number of arguments");
		return -ENOENT;
	}

	app_idx = strtol(name, NULL, 16);

	if (!len_rd) {
		return 0;
	}

	err = bt_mesh_settings_set(read_cb, cb_arg, &key, sizeof(key));
	if (err < 0) {
		return -EINVAL;
	}

	/* One extra copying since key.val array is from packed structure
	 * and might be unaligned.
	 */
	memcpy(val, key.val, sizeof(key.val));

	err = bt_mesh_app_key_set(app_idx, key.net_idx, &val[0],
			      key.updated ? &val[1] : NULL);
	if (err) {
		LOG_ERR("Failed to set \'app-key\'");
		return err;
	}

	LOG_DBG("AppKeyIndex 0x%03x recovered from storage", app_idx);

	return 0;
}

BT_MESH_SETTINGS_DEFINE(app, "AppKey", app_key_set);

void bt_mesh_app_key_pending_store(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(app_key_updates); i++) {
		struct app_key_update *update = &app_key_updates[i];

		if (!update->valid) {
			continue;
		}

		update->valid = 0U;

		if (update->clear) {
			clear_app_key(update->key_idx);
		} else {
			store_app_key(update->key_idx);
		}
	}
}
