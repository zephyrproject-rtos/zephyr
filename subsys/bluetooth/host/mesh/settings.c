/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <misc/util.h>
#include <misc/byteorder.h>

#include <settings/settings.h>

#include <net/buf.h>

#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_SETTINGS)
#include "common/log.h"

#include "../settings.h"
#include "mesh.h"
#include "net.h"
#include "crypto.h"
#include "transport.h"
#include "access.h"
#include "foundation.h"
#include "proxy.h"
#include "settings.h"

/* Mesh network storage information */
struct net_val {
	u16_t primary_addr;
	u8_t  dev_key[16];
} __packed;

/* Sequence number storage */
struct seq_val {
	u8_t val[3];
} __packed;

/* IV Index & IV Update storage */
struct iv_val {
	u32_t iv_index;
	bool  iv_update;
} __packed;

/* Replay Protection List storage */
struct rpl_val {
	u32_t seq:24,
	      old_iv:1;
};

/* NetKey storage information */
struct net_key_val {
	u8_t kr_flag:1,
	     kr_phase:7;
	u8_t val[2][16];
} __packed;

/* AppKey storage information */
struct app_key_val {
	u16_t net_idx;
	bool  updated;
	u8_t  val[2][16];
} __packed;

static int net_set(int argc, char **argv, char *val)
{
	struct net_val net;
	int len, err;

	BT_DBG("val %s", val ? val : "(null)");

	if (!val) {
		bt_mesh_comp_unprovision();
		memset(bt_mesh.dev_key, 0, sizeof(bt_mesh.dev_key));
		return 0;
	}

	len = sizeof(net);
	err = settings_bytes_from_str(val, &net, &len);
	if (err) {
		BT_ERR("Failed to decode value %s (err %d)", val, err);
		return err;
	}

	if (len != sizeof(net)) {
		BT_ERR("Unexpected value length (%d != %zu)", len, sizeof(net));
		return -EINVAL;
	}

	memcpy(bt_mesh.dev_key, net.dev_key, sizeof(bt_mesh.dev_key));
	bt_mesh_comp_provision(net.primary_addr);

	BT_DBG("Provisioned with primary address 0x%04x", net.primary_addr);
	BT_DBG("Recovered DevKey %s", bt_hex(bt_mesh.dev_key, 16));

	return 0;
}

static int iv_set(int argc, char **argv, char *val)
{
	struct iv_val iv;
	int len, err;

	BT_DBG("val %s", val ? val : "(null)");

	if (!val) {
		bt_mesh.iv_index = 0;
		bt_mesh.iv_update = 0;
		return 0;
	}

	len = sizeof(iv);
	err = settings_bytes_from_str(val, &iv, &len);
	if (err) {
		BT_ERR("Failed to decode value %s (err %d)", val, err);
		return err;
	}

	if (len != sizeof(iv)) {
		BT_ERR("Unexpected value length (%d != %zu)", len, sizeof(iv));
		return -EINVAL;
	}

	bt_mesh.iv_index = iv.iv_index;
	bt_mesh.iv_update = iv.iv_update;

	BT_DBG("IV Index 0x%04x (IV Update Flag %u)", bt_mesh.iv_index,
	       bt_mesh.iv_update);

	return 0;
}

static int seq_set(int argc, char **argv, char *val)
{
	struct seq_val seq;
	int len, err;

	BT_DBG("val %s", val ? val : "(null)");

	if (!val) {
		bt_mesh.seq = 0;
		return 0;
	}

	len = sizeof(seq);
	err = settings_bytes_from_str(val, &seq, &len);
	if (err) {
		BT_ERR("Failed to decode value %s (err %d)", val, err);
		return err;
	}

	if (len != sizeof(seq)) {
		BT_ERR("Unexpected value length (%d != %zu)", len, sizeof(seq));
		return -EINVAL;
	}

	bt_mesh.seq = ((u32_t)seq.val[0] | ((u32_t)seq.val[1] << 8) |
		       ((u32_t)seq.val[2] << 16));

	if (CONFIG_BT_MESH_SEQ_STORE_RATE > 0) {
		/* Make sure we have a large enough sequence number. We
		 * subtract 1 so that the first transmission causes a write
		 * to the settings storage.
		 */
		bt_mesh.seq += CONFIG_BT_MESH_SEQ_STORE_RATE - 1;
	}

	BT_DBG("Sequence Number 0x%06x", bt_mesh.seq);

	return 0;
}

static struct bt_mesh_rpl *rpl_find(u16_t src)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(bt_mesh.rpl); i++) {
		if (bt_mesh.rpl[i].src == src) {
			return &bt_mesh.rpl[i];
		}
	}

	return NULL;
}

static struct bt_mesh_rpl *rpl_alloc(u16_t src)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(bt_mesh.rpl); i++) {
		if (!bt_mesh.rpl[i].src) {
			bt_mesh.rpl[i].src = src;
			return &bt_mesh.rpl[i];
		}
	}

	return NULL;
}

static int rpl_set(int argc, char **argv, char *val)
{
	struct bt_mesh_rpl *entry;
	struct rpl_val rpl;
	int len, err;
	u16_t src;

	if (argc < 1) {
		BT_ERR("Invalid argc (%d)", argc);
		return -ENOENT;
	}

	BT_DBG("argv[0] %s val %s", argv[0], val ? val : "(null)");

	src = strtol(argv[0], NULL, 16);
	entry = rpl_find(src);

	if (!val) {
		if (entry) {
			memset(entry, 0, sizeof(*entry));
		} else {
			BT_WARN("Unable to find RPL entry for 0x%04x", src);
		}

		return 0;
	}

	if (!entry) {
		entry = rpl_alloc(src);
		if (!entry) {
			BT_ERR("Unable to allocate RPL entry for 0x%04x", src);
			return -ENOMEM;
		}
	}

	len = sizeof(rpl);
	err = settings_bytes_from_str(val, &rpl, &len);
	if (err) {
		BT_ERR("Failed to decode value %s (err %d)", val, err);
		return err;
	}

	if (len != sizeof(rpl)) {
		BT_ERR("Unexpected value length (%d != %zu)", len, sizeof(rpl));
		return -EINVAL;
	}

	entry->seq = rpl.seq;
	entry->old_iv = rpl.old_iv;

	BT_DBG("RPL entry for 0x%04x: Seq 0x%06x old_iv %u", entry->src,
	       entry->seq, entry->old_iv);

	return 0;
}

static int net_key_set(int argc, char **argv, char *val)
{
	struct bt_mesh_subnet *sub;
	struct net_key_val key;
	int len, i, err;
	u16_t net_idx;

	BT_DBG("argv[0] %s val %s", argv[0], val ? val : "(null)");

	net_idx = strtol(argv[0], NULL, 16);
	sub = bt_mesh_subnet_get(net_idx);

	if (!val) {
		if (!sub) {
			BT_ERR("No subnet with NetKeyIndex 0x%03x", net_idx);
			return -ENOENT;
		}

		BT_DBG("Deleting NetKeyIndex 0x%03x", net_idx);
		bt_mesh_subnet_del(sub);
		return 0;
	}

	len = sizeof(key);
	err = settings_bytes_from_str(val, &key, &len);
	if (err) {
		BT_ERR("Failed to decode value %s (err %d)", val, err);
		return err;
	}

	if (len != sizeof(key)) {
		BT_ERR("Unexpected value length (%d != %zu)", len, sizeof(key));
		return -EINVAL;
	}

	if (sub) {
		BT_DBG("Updating existing NetKeyIndex 0x%03x", net_idx);

		sub->kr_flag = key.kr_flag;
		sub->kr_phase = key.kr_phase;
		memcpy(sub->keys[0].net, &key.val[0], 16);
		memcpy(sub->keys[1].net, &key.val[1], 16);

		return 0;
	}

	for (i = 0; i < ARRAY_SIZE(bt_mesh.sub); i++) {
		if (bt_mesh.sub[i].net_idx == BT_MESH_KEY_UNUSED) {
			sub = &bt_mesh.sub[i];
			break;
		}
	}

	if (!sub) {
		BT_ERR("No space to allocate a new subnet");
		return -ENOMEM;
	}

	sub->net_idx = net_idx;
	sub->kr_flag = key.kr_flag;
	sub->kr_phase = key.kr_phase;
	memcpy(sub->keys[0].net, &key.val[0], 16);
	memcpy(sub->keys[1].net, &key.val[1], 16);

	BT_DBG("NetKeyIndex 0x%03x recovered from storage", net_idx);

	return 0;
}

static int app_key_set(int argc, char **argv, char *val)
{
	struct bt_mesh_app_key *app;
	struct bt_mesh_subnet *sub;
	struct app_key_val key;
	u16_t app_idx;
	int len, err;

	BT_DBG("argv[0] %s val %s", argv[0], val ? val : "(null)");

	app_idx = strtol(argv[0], NULL, 16);

	if (!val) {
		BT_DBG("Deleting AppKeyIndex 0x%03x", app_idx);

		app = bt_mesh_app_key_find(app_idx);
		if (app) {
			bt_mesh_app_key_del(app);
		}

		return 0;
	}

	len = sizeof(key);
	err = settings_bytes_from_str(val, &key, &len);
	if (err) {
		BT_ERR("Failed to decode value %s (err %d)", val, err);
		return err;
	}

	if (len != sizeof(key)) {
		BT_ERR("Unexpected value length (%d != %zu)", len, sizeof(key));
		return -EINVAL;
	}

	sub = bt_mesh_subnet_get(key.net_idx);
	if (!sub) {
		BT_ERR("Failed to find subnet 0x%03x", key.net_idx);
		return -ENOENT;
	}

	app = bt_mesh_app_key_find(app_idx);
	if (!app) {
		app = bt_mesh_app_key_alloc(app_idx);
	}

	if (!app) {
		BT_ERR("No space for a new app key");
		return -ENOMEM;
	}

	app->net_idx = key.net_idx;
	app->app_idx = app_idx;
	app->updated = key.updated;
	memcpy(app->keys[0].val, key.val[0], 16);
	memcpy(app->keys[1].val, key.val[1], 16);

	bt_mesh_app_id(app->keys[0].val, &app->keys[0].id);
	bt_mesh_app_id(app->keys[1].val, &app->keys[1].id);

	BT_DBG("AppKeyIndex 0x%03x recovered from storage", app_idx);

	return 0;
}

const struct mesh_setting {
	const char *name;
	int (*func)(int argc, char **argv, char *val);
} settings[] = {
	{ "Net", net_set },
	{ "IV", iv_set },
	{ "Seq", seq_set },
	{ "RPL", rpl_set },
	{ "NetKey", net_key_set },
	{ "AppKey", app_key_set },
};

static int mesh_set(int argc, char **argv, char *val)
{
	int i;

	if (argc < 1) {
		BT_ERR("Insufficient number of arguments");
		return -EINVAL;
	}

	BT_DBG("argv[0] %s val %s", argv[0], val ? val : "(null)");

	for (i = 0; i < ARRAY_SIZE(settings); i++) {
		if (!strcmp(settings[i].name, argv[0])) {
			argc--;
			argv++;

			return settings[i].func(argc, argv, val);
		}
	}

	BT_WARN("No matching handler for key %s", argv[0]);

	return -ENOENT;
}

static int subnet_init(struct bt_mesh_subnet *sub)
{
	int err;

	err = bt_mesh_net_keys_create(&sub->keys[0], sub->keys[0].net);
	if (err) {
		BT_ERR("Unable to generate keys for subnet");
		return -EIO;
	}

	if (sub->kr_phase != BT_MESH_KR_NORMAL) {
		err = bt_mesh_net_keys_create(&sub->keys[1], sub->keys[1].net);
		if (err) {
			BT_ERR("Unable to generate keys for subnet");
			memset(&sub->keys[0], 0, sizeof(sub->keys[0]));
			return -EIO;
		}
	}

	if (IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY)) {
		sub->node_id = BT_MESH_NODE_IDENTITY_STOPPED;
	} else {
		sub->node_id = BT_MESH_NODE_IDENTITY_NOT_SUPPORTED;
	}

	/* Make sure we have valid beacon data to be sent */
	bt_mesh_net_beacon_update(sub);

	return 0;
}

static int mesh_commit(void)
{
	int i;

	BT_DBG("sub[0].net_idx 0x%03x", bt_mesh.sub[0].net_idx);

	if (bt_mesh.sub[0].net_idx == BT_MESH_KEY_UNUSED) {
		/* Nothing to do since we're not yet provisioned */
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT)) {
		bt_mesh_proxy_prov_disable();
	}

	for (i = 0; i < ARRAY_SIZE(bt_mesh.sub); i++) {
		struct bt_mesh_subnet *sub = &bt_mesh.sub[i];
		int err;

		if (sub->net_idx == BT_MESH_KEY_UNUSED) {
			continue;
		}

		err = subnet_init(sub);
		if (err) {
			BT_ERR("Failed to init subnet 0x%03x", sub->net_idx);
		}
	}

	/* Set initial IV Update procedure state time-stamp */
	bt_mesh.last_update = BT_MESH_NET_IVU_UNKNOWN;

	/* Set a timer to transition back to normal mode */
	if (bt_mesh.iv_update) {
		k_delayed_work_submit(&bt_mesh.ivu_complete,
				      BT_MESH_NET_IVU_TIMEOUT);
	}

	bt_mesh.valid = 1;

	bt_mesh_net_start();

	return 0;
}

BT_SETTINGS_DEFINE(mesh, mesh_set, mesh_commit, NULL);

void bt_mesh_store_net(u16_t primary_addr, const u8_t dev_key[16])
{
	char buf[BT_SETTINGS_SIZE(sizeof(struct net_val))];
	struct net_val net;
	char *str;

	BT_DBG("addr 0x%04x DevKey %s", primary_addr, bt_hex(dev_key, 16));

	net.primary_addr = primary_addr;
	memcpy(net.dev_key, dev_key, 16);

	str = settings_str_from_bytes(&net, sizeof(net), buf, sizeof(buf));
	if (!str) {
		BT_ERR("Unable to encode Network as value");
		return;
	}

	BT_DBG("Saving Network as value %s", str);
	settings_save_one("bt/mesh/Net", str);
}

void bt_mesh_store_iv(void)
{
	char buf[BT_SETTINGS_SIZE(sizeof(struct iv_val))];
	struct iv_val iv;
	char *str;

	iv.iv_index = bt_mesh.iv_index;
	iv.iv_update = bt_mesh.iv_update;

	str = settings_str_from_bytes(&iv, sizeof(iv), buf, sizeof(buf));
	if (!str) {
		BT_ERR("Unable to encode IV as value");
		return;
	}

	BT_DBG("Saving IV as value %s", str);
	settings_save_one("bt/mesh/IV", str);
}

void bt_mesh_store_seq(void)
{
	char buf[BT_SETTINGS_SIZE(sizeof(struct seq_val))];
	struct seq_val seq;
	char *str;

	if (CONFIG_BT_MESH_SEQ_STORE_RATE &&
	    (bt_mesh.seq % CONFIG_BT_MESH_SEQ_STORE_RATE)) {
		return;
	}

	seq.val[0] = bt_mesh.seq;
	seq.val[1] = bt_mesh.seq >> 8;
	seq.val[2] = bt_mesh.seq >> 16;

	str = settings_str_from_bytes(&seq, sizeof(seq), buf, sizeof(buf));
	if (!str) {
		BT_ERR("Unable to encode Seq as value");
		return;
	}

	BT_DBG("Saving Seq as value %s", str);
	settings_save_one("bt/mesh/Seq", str);
}

static void store_rpl(struct bt_mesh_rpl *entry)
{
	char buf[BT_SETTINGS_SIZE(sizeof(struct rpl_val))];
	struct rpl_val rpl;
	char path[18];
	char *str;

	BT_DBG("src 0x%04x seq 0x%06x old_iv %u", entry->src, entry->seq,
	       entry->old_iv);

	rpl.seq = entry->seq;
	rpl.old_iv = entry->old_iv;

	str = settings_str_from_bytes(&rpl, sizeof(rpl), buf, sizeof(buf));
	if (!str) {
		BT_ERR("Unable to encode RPL as value");
		return;
	}

	snprintk(path, sizeof(path), "bt/mesh/RPL/%x", entry->src);

	BT_DBG("Saving RPL %s as value %s", path, str);
	settings_save_one(path, str);
}

#if CONFIG_BT_MESH_RPL_STORE_TIMEOUT > 0
static struct k_delayed_work rpl_store;

static void rpl_store_timeout(struct k_work *work)
{
	int i;

	BT_DBG("");

	for (i = 0; i < ARRAY_SIZE(bt_mesh.rpl); i++) {
		struct bt_mesh_rpl *rpl = &bt_mesh.rpl[i];

		if (rpl->store) {
			rpl->store = false;
			store_rpl(rpl);
		}
	}
}
#endif

void bt_mesh_store_rpl(struct bt_mesh_rpl *entry)
{
#if CONFIG_BT_MESH_RPL_STORE_TIMEOUT > 0
	entry->store = true;
	k_delayed_work_submit(&rpl_store,
			      K_SECONDS(CONFIG_BT_MESH_RPL_STORE_TIMEOUT));
	BT_DBG("Waiting %d seconds", CONFIG_BT_MESH_RPL_STORE_TIMEOUT);
#else
	store_rpl(entry);
#endif
}

void bt_mesh_store_subnet(struct bt_mesh_subnet *sub)
{
	char buf[BT_SETTINGS_SIZE(sizeof(struct net_key_val))];
	struct net_key_val key;
	char path[20];
	char *str;

	BT_DBG("NetKeyIndex 0x%03x NetKey %s", sub->net_idx,
	       bt_hex(sub->keys[0].net, 16));

	memcpy(&key.val[0], sub->keys[0].net, 16);
	memcpy(&key.val[1], sub->keys[1].net, 16);
	key.kr_flag = sub->kr_flag;
	key.kr_phase = sub->kr_phase;

	str = settings_str_from_bytes(&key, sizeof(key), buf, sizeof(buf));
	if (!str) {
		BT_ERR("Unable to encode NetKey as value");
		return;
	}

	snprintk(path, sizeof(path), "bt/mesh/NetKey/%x", sub->net_idx);

	BT_DBG("Saving NetKey %s as value %s", path, str);
	settings_save_one(path, str);
}

void bt_mesh_store_app_key(struct bt_mesh_app_key *app)
{
	char buf[BT_SETTINGS_SIZE(sizeof(struct app_key_val))];
	struct app_key_val key;
	char path[20];
	char *str;

	key.net_idx = app->net_idx;
	key.updated = app->updated;
	memcpy(key.val[0], app->keys[0].val, 16);
	memcpy(key.val[1], app->keys[1].val, 16);

	str = settings_str_from_bytes(&key, sizeof(key), buf, sizeof(buf));
	if (!str) {
		BT_ERR("Unable to encode AppKey as value");
		return;
	}

	snprintk(path, sizeof(path), "bt/mesh/AppKey/%x", app->app_idx);

	BT_DBG("Saving AppKey %s as value %s", path, str);
	settings_save_one(path, str);
}

void bt_mesh_clear_net(void)
{
	BT_DBG("");

	settings_save_one("bt/mesh/IV", NULL);
	settings_save_one("bt/mesh/Net", NULL);
}

void bt_mesh_clear_subnet(struct bt_mesh_subnet *sub)
{
	char path[20];

	BT_DBG("NetKeyIndex 0x%03x", sub->net_idx);

	snprintk(path, sizeof(path), "bt/mesh/NetKey/%x", sub->net_idx);
	settings_save_one(path, NULL);
}

void bt_mesh_clear_app_key(struct bt_mesh_app_key *key)
{
	char path[20];

	BT_DBG("AppKeyIndex 0x%03x", key->app_idx);

	snprintk(path, sizeof(path), "bt/mesh/AppKey/%x", key->app_idx);
	settings_save_one(path, NULL);
}

void bt_mesh_clear_rpl(void)
{
	int i;

	BT_DBG("");

	for (i = 0; i < ARRAY_SIZE(bt_mesh.rpl); i++) {
		struct bt_mesh_rpl *rpl = &bt_mesh.rpl[i];
		char path[18];

		if (!rpl->src) {
			continue;
		}

		snprintk(path, sizeof(path), "bt/mesh/RPL/%x", rpl->src);
		settings_save_one(path, NULL);
	}
}

void bt_mesh_settings_init(void)
{
#if CONFIG_BT_MESH_RPL_STORE_TIMEOUT > 0
	k_delayed_work_init(&rpl_store, rpl_store_timeout);
#endif
}
