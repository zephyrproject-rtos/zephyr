/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <misc/util.h>
#include <misc/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_MODEL)
#include "common/log.h"

#include "mesh.h"
#include "adv.h"
#include "net.h"
#include "lpn.h"
#include "transport.h"
#include "crypto.h"
#include "access.h"
#include "beacon.h"
#include "proxy.h"
#include "foundation.h"
#include "friend.h"

#define DEFAULT_TTL 7

static struct bt_mesh_cfg *conf;

static struct label {
	u16_t addr;
	u8_t  uuid[16];
} labels[CONFIG_BT_MESH_LABEL_COUNT];

static inline void key_idx_unpack(struct net_buf_simple *buf,
				  u16_t *idx1, u16_t *idx2)
{
	*idx1 = sys_get_le16(&buf->data[0]) & 0xfff;
	*idx2 = sys_get_le16(&buf->data[1]) >> 4;
	net_buf_simple_pull(buf, 3);
}

static inline void key_idx_pack(struct net_buf_simple *buf,
				u16_t idx1, u16_t idx2)
{
	net_buf_simple_add_le16(buf, idx1 | ((idx2 & 0x00f) << 12));
	net_buf_simple_add_u8(buf, idx2 >> 4);
}

static void hb_send(struct bt_mesh_model *model)
{

	struct bt_mesh_cfg *cfg = model->user_data;
	u16_t feat = 0;
	struct __packed {
		u8_t  init_ttl;
		u16_t feat;
	} hb;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = cfg->hb_pub.net_idx,
		.app_idx = BT_MESH_KEY_UNUSED,
		.addr = cfg->hb_pub.dst,
		.send_ttl = cfg->hb_pub.ttl,
	};
	struct bt_mesh_net_tx tx = {
		.sub = bt_mesh_subnet_get(cfg->hb_pub.net_idx),
		.ctx = &ctx,
		.src = model->elem->addr,
	};

	hb.init_ttl = cfg->hb_pub.ttl;

	if (bt_mesh_relay_get() == BT_MESH_RELAY_ENABLED) {
		feat |= BT_MESH_FEAT_RELAY;
	}

	if (bt_mesh_gatt_proxy_get() == BT_MESH_GATT_PROXY_ENABLED) {
		feat |= BT_MESH_FEAT_PROXY;
	}

	if (bt_mesh_friend_get() == BT_MESH_FRIEND_ENABLED) {
		feat |= BT_MESH_FEAT_FRIEND;
	}

#if defined(CONFIG_BT_MESH_LOW_POWER)
	if (bt_mesh.lpn.state != BT_MESH_LPN_DISABLED) {
		feat |= BT_MESH_FEAT_LOW_POWER;
	}
#endif

	hb.feat = sys_cpu_to_be16(feat);

	BT_DBG("InitTTL %u feat 0x%04x", cfg->hb_pub.ttl, feat);

	bt_mesh_ctl_send(&tx, TRANS_CTL_OP_HEARTBEAT, &hb, sizeof(hb),
			 NULL, NULL);
}

static int comp_add_elem(struct net_buf_simple *buf, struct bt_mesh_elem *elem,
			 bool primary)
{
	struct bt_mesh_model *mod;
	int i;

	if (net_buf_simple_tailroom(buf) <
	    4 + (elem->model_count * 2) + (elem->vnd_model_count * 2)) {
		BT_ERR("Too large device composition");
		return -E2BIG;
	}

	net_buf_simple_add_le16(buf, elem->loc);

	net_buf_simple_add_u8(buf, elem->model_count);
	net_buf_simple_add_u8(buf, elem->vnd_model_count);

	for (i = 0; i < elem->model_count; i++) {
		mod = &elem->models[i];
		net_buf_simple_add_le16(buf, mod->id);
	}

	for (i = 0; i < elem->vnd_model_count; i++) {
		mod = &elem->vnd_models[i];
		net_buf_simple_add_le16(buf, mod->vnd.company);
		net_buf_simple_add_le16(buf, mod->vnd.id);
	}

	return 0;
}

static int comp_get_page_0(struct net_buf_simple *buf)
{
	u16_t feat = 0;
	const struct bt_mesh_comp *comp;
	int i;

	comp = bt_mesh_comp_get();

	if (IS_ENABLED(CONFIG_BT_MESH_RELAY)) {
		feat |= BT_MESH_FEAT_RELAY;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY)) {
		feat |= BT_MESH_FEAT_PROXY;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_FRIEND)) {
		feat |= BT_MESH_FEAT_FRIEND;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)) {
		feat |= BT_MESH_FEAT_LOW_POWER;
	}

	net_buf_simple_add_le16(buf, comp->cid);
	net_buf_simple_add_le16(buf, comp->pid);
	net_buf_simple_add_le16(buf, comp->vid);
	net_buf_simple_add_le16(buf, CONFIG_BT_MESH_CRPL);
	net_buf_simple_add_le16(buf, feat);

	for (i = 0; i < comp->elem_count; i++) {
		int err;

		err = comp_add_elem(buf, &comp->elem[i], i == 0);
		if (err) {
			return err;
		}
	}

	return 0;
}

static void dev_comp_data_get(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	struct net_buf_simple *sdu = NET_BUF_SIMPLE(BT_MESH_TX_SDU_MAX);
	u8_t page;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	page = net_buf_simple_pull_u8(buf);
	if (page != 0) {
		BT_WARN("Composition page %u not available", page);
		page = 0;
	}

	bt_mesh_model_msg_init(sdu, OP_DEV_COMP_DATA_STATUS);

	net_buf_simple_add_u8(sdu, page);
	if (comp_get_page_0(sdu) < 0) {
		BT_ERR("Unable to get composition page 0");
		return;
	}

	if (bt_mesh_model_send(model, ctx, sdu, NULL, NULL)) {
		BT_ERR("Unable to send Device Composition Status response");
	}
}

static struct bt_mesh_model *get_model(struct bt_mesh_elem *elem,
				       struct net_buf_simple *buf, bool *vnd)
{
	if (buf->len < 4) {
		u16_t id;

		id = net_buf_simple_pull_le16(buf);

		BT_DBG("ID 0x%04x addr 0x%04x", id, elem->addr);

		*vnd = false;

		return bt_mesh_model_find(elem, id);
	} else {
		u16_t company, id;

		company = net_buf_simple_pull_le16(buf);
		id = net_buf_simple_pull_le16(buf);

		BT_DBG("Company 0x%04x ID 0x%04x addr 0x%04x", company, id,
		       elem->addr);

		*vnd = true;

		return bt_mesh_model_find_vnd(elem, company, id);
	}
}

static bool app_key_is_valid(u16_t app_idx)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(bt_mesh.app_keys); i++) {
		struct bt_mesh_app_key *key = &bt_mesh.app_keys[i];

		if (key->net_idx != BT_MESH_KEY_UNUSED &&
		    key->app_idx == app_idx) {
			return true;
		}
	}

	return false;
}

static u8_t _mod_pub_set(struct bt_mesh_model *model, u16_t pub_addr,
			 u16_t app_idx, u8_t cred_flag, u8_t ttl, u8_t period,
			 u8_t retransmit)
{
	if (!model->pub) {
		return STATUS_NVAL_PUB_PARAM;
	}

	if (!IS_ENABLED(CONFIG_BT_MESH_LOW_POWER) && cred_flag) {
		return STATUS_FEAT_NOT_SUPP;
	}

	if (!model->pub->func && period) {
		return STATUS_NVAL_PUB_PARAM;
	}

	if (pub_addr == BT_MESH_ADDR_UNASSIGNED) {
		model->pub->addr = BT_MESH_ADDR_UNASSIGNED;
		model->pub->key = 0;
		model->pub->cred = 0;
		model->pub->ttl = 0;
		model->pub->period = 0;
		model->pub->retransmit = 0;

		if (model->pub->func) {
			k_delayed_work_cancel(&model->pub->timer);
		}

		return STATUS_SUCCESS;
	}

	if (!bt_mesh_app_key_find(app_idx)) {
		return STATUS_INVALID_APPKEY;
	}

	model->pub->addr = pub_addr;
	model->pub->key = app_idx;
	model->pub->cred = cred_flag;
	model->pub->ttl = ttl;
	model->pub->period = period;
	model->pub->retransmit = retransmit;

	if (model->pub->func) {
		s32_t period_ms;

		period_ms = bt_mesh_model_pub_period_get(model);
		BT_DBG("period %u ms", period_ms);

		if (period_ms) {
			k_delayed_work_submit(&model->pub->timer, period_ms);
		} else {
			k_delayed_work_cancel(&model->pub->timer);
		}
	}

	return STATUS_SUCCESS;
}

static u8_t mod_bind(struct bt_mesh_model *model, u16_t key_idx)
{
	int i;

	BT_DBG("key_idx 0x%04x", key_idx);

	if (!app_key_is_valid(key_idx)) {
		return STATUS_INVALID_APPKEY;
	}

	for (i = 0; i < ARRAY_SIZE(model->keys); i++) {
		/* Treat existing binding as success */
		if (model->keys[i] == key_idx) {
			return STATUS_SUCCESS;
		}
	}

	for (i = 0; i < ARRAY_SIZE(model->keys); i++) {
		if (model->keys[i] == BT_MESH_KEY_UNUSED) {
			model->keys[i] = key_idx;
			return STATUS_SUCCESS;
		}
	}

	return STATUS_INSUFF_RESOURCES;
}

static u8_t mod_unbind(struct bt_mesh_model *model, u16_t key_idx)
{
	int i;

	BT_DBG("model %p key_idx 0x%04x", model, key_idx);

	if (!app_key_is_valid(key_idx)) {
		return STATUS_INVALID_APPKEY;
	}

	for (i = 0; i < ARRAY_SIZE(model->keys); i++) {
		if (model->keys[i] != key_idx) {
			continue;
		}

		model->keys[i] = BT_MESH_KEY_UNUSED;

		if (model->pub && model->pub->key == key_idx) {
			_mod_pub_set(model, BT_MESH_ADDR_UNASSIGNED,
				     BT_MESH_KEY_UNUSED, 0, 0, 0, 0);
		}

		return STATUS_SUCCESS;
	}

	return STATUS_CANNOT_BIND;
}

static struct bt_mesh_app_key *app_key_alloc(u16_t app_idx)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(bt_mesh.app_keys); i++) {
		struct bt_mesh_app_key *key = &bt_mesh.app_keys[i];

		if (key->net_idx == BT_MESH_KEY_UNUSED) {
			return key;
		}
	}

	return NULL;
}

static u8_t app_key_set(u16_t net_idx, u16_t app_idx, const u8_t val[16],
			bool update)
{
	struct bt_mesh_app_keys *keys;
	struct bt_mesh_app_key *key;
	struct bt_mesh_subnet *sub;

	BT_DBG("net_idx 0x%04x app_idx %04x update %u val %s",
	       net_idx, app_idx, update, bt_hex(val, 16));

	sub = bt_mesh_subnet_get(net_idx);
	if (!sub) {
		return STATUS_INVALID_NETKEY;
	}

	key = bt_mesh_app_key_find(app_idx);
	if (update) {
		if (!key) {
			return STATUS_INVALID_APPKEY;
		}

		if (key->net_idx != net_idx) {
			return STATUS_INVALID_BINDING;
		}

		keys = &key->keys[1];

		/* The AppKey Update message shall generate an error when node
		 * is in normal operation, Phase 2, or Phase 3 or in Phase 1
		 * when the AppKey Update message on a valid AppKeyIndex when
		 * the AppKey value is different.
		 */
		if (sub->kr_phase != BT_MESH_KR_PHASE_1) {
			return STATUS_CANNOT_UPDATE;
		}

		if (key->updated) {
			if (memcmp(keys->val, val, 16)) {
				return STATUS_CANNOT_UPDATE;
			} else {
				return STATUS_SUCCESS;
			}
		}

		key->updated = true;
	} else {
		if (key) {
			if (key->net_idx == net_idx &&
			    !memcmp(key->keys[0].val, val, 16)) {
				return STATUS_SUCCESS;
			}

			if (key->net_idx == net_idx) {
				return STATUS_IDX_ALREADY_STORED;
			} else {
				return STATUS_INVALID_NETKEY;
			}
		}

		key = app_key_alloc(app_idx);
		if (!key) {
			return STATUS_INSUFF_RESOURCES;
		}

		keys = &key->keys[0];
	}

	if (bt_mesh_app_id(val, &keys->id)) {
		if (update) {
			key->updated = false;
		}

		return STATUS_STORAGE_FAIL;
	}

	BT_DBG("app_idx 0x%04x AID 0x%02x", app_idx, keys->id);

	key->net_idx = net_idx;
	key->app_idx = app_idx;
	memcpy(keys->val, val, 16);

	return STATUS_SUCCESS;
}

static void app_key_add(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 4 + 4);
	u16_t key_net_idx, key_app_idx;
	u8_t status;

	key_idx_unpack(buf, &key_net_idx, &key_app_idx);

	BT_DBG("AppIdx 0x%04x NetIdx 0x%04x", key_app_idx, key_net_idx);

	bt_mesh_model_msg_init(msg, OP_APP_KEY_STATUS);

	status = app_key_set(key_net_idx, key_app_idx, buf->data, false);
	BT_DBG("status 0x%02x", status);
	net_buf_simple_add_u8(msg, status);

	key_idx_pack(msg, key_net_idx, key_app_idx);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send App Key Status response");
	}
}

static void app_key_update(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 4 + 4);
	u16_t key_net_idx, key_app_idx;
	u8_t status;

	key_idx_unpack(buf, &key_net_idx, &key_app_idx);

	BT_DBG("AppIdx 0x%04x NetIdx 0x%04x", key_app_idx, key_net_idx);

	bt_mesh_model_msg_init(msg, OP_APP_KEY_STATUS);

	status = app_key_set(key_net_idx, key_app_idx, buf->data, true);
	BT_DBG("status 0x%02x", status);
	net_buf_simple_add_u8(msg, status);

	key_idx_pack(msg, key_net_idx, key_app_idx);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send App Key Status response");
	}
}

static void _mod_unbind(struct bt_mesh_model *mod, struct bt_mesh_elem *elem,
			bool vnd, bool primary, void *user_data)
{
	u16_t *key_idx = user_data;

	mod_unbind(mod, *key_idx);
}

static void _app_key_del(struct bt_mesh_app_key *key)
{
	bt_mesh_model_foreach(_mod_unbind, &key->app_idx);

	key->net_idx = BT_MESH_KEY_UNUSED;
	memset(key->keys, 0, sizeof(key->keys));
}

static void app_key_del(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 4 + 4);
	u16_t key_net_idx, key_app_idx;
	struct bt_mesh_app_key *key;
	u8_t status;

	key_idx_unpack(buf, &key_net_idx, &key_app_idx);

	BT_DBG("AppIdx 0x%04x NetIdx 0x%04x", key_app_idx, key_net_idx);

	if (!bt_mesh_subnet_get(key_net_idx)) {
		status = STATUS_INVALID_NETKEY;
		goto send_status;
	}

	key = bt_mesh_app_key_find(key_app_idx);
	if (!key) {
		/* Treat as success since the client might have missed a
		 * previous response and is resending the request.
		 */
		status = STATUS_SUCCESS;
		goto send_status;
	}

	if (key->net_idx != key_net_idx) {
		status = STATUS_INVALID_BINDING;
		goto send_status;
	}

	_app_key_del(key);
	status = STATUS_SUCCESS;

send_status:
	bt_mesh_model_msg_init(msg, OP_APP_KEY_STATUS);

	net_buf_simple_add_u8(msg, status);

	key_idx_pack(msg, key_net_idx, key_app_idx);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send App Key Status response");
	}
}

/* Index list length: 3 bytes for every pair and 2 bytes for an odd idx */
#define IDX_LEN(num) (((num) / 2) * 3 + ((num) % 2) * 2)

static void app_key_get(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	struct net_buf_simple *msg =
		NET_BUF_SIMPLE(2 + 3 + 4 +
			       IDX_LEN(CONFIG_BT_MESH_APP_KEY_COUNT));
	u16_t get_idx, i, prev;
	u8_t status;

	get_idx = net_buf_simple_pull_le16(buf);
	if (get_idx > 0xfff) {
		BT_ERR("Invalid NetKeyIndex 0x%04x", get_idx);
		return;
	}

	BT_DBG("idx 0x%04x", get_idx);

	bt_mesh_model_msg_init(msg, OP_APP_KEY_LIST);

	if (!bt_mesh_subnet_get(get_idx)) {
		status = STATUS_INVALID_NETKEY;
	} else {
		status = STATUS_SUCCESS;
	}

	net_buf_simple_add_u8(msg, status);
	net_buf_simple_add_le16(msg, get_idx);

	if (status != STATUS_SUCCESS) {
		goto send_status;
	}

	prev = BT_MESH_KEY_UNUSED;
	for (i = 0; i < ARRAY_SIZE(bt_mesh.app_keys); i++) {
		struct bt_mesh_app_key *key = &bt_mesh.app_keys[i];

		if (key->net_idx != get_idx) {
			continue;
		}

		if (prev == BT_MESH_KEY_UNUSED) {
			prev = key->app_idx;
			continue;
		}

		key_idx_pack(msg, prev, key->app_idx);
		prev = BT_MESH_KEY_UNUSED;
	}

	if (prev != BT_MESH_KEY_UNUSED) {
		net_buf_simple_add_le16(msg, prev);
	}

send_status:
	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send AppKey List");
	}
}

static void beacon_get(struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	/* Needed size: opcode (2 bytes) + msg + MIC */
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 1 + 4);

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	bt_mesh_model_msg_init(msg, OP_BEACON_STATUS);
	net_buf_simple_add_u8(msg, bt_mesh_beacon_get());

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send Config Beacon Status response");
	}
}

static void beacon_set(struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	/* Needed size: opcode (2 bytes) + msg + MIC */
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 1 + 4);
	struct bt_mesh_cfg *cfg = model->user_data;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	if (!cfg) {
		BT_WARN("No Configuration Server context available");
	} else if (buf->data[0] == 0x00 || buf->data[0] == 0x01) {
		if (buf->data[0] != cfg->beacon) {
			cfg->beacon = buf->data[0];

			if (cfg->beacon) {
				bt_mesh_beacon_enable();
			} else {
				bt_mesh_beacon_disable();
			}
		}
	} else {
		BT_WARN("Invalid Config Beacon value 0x%02x", buf->data[0]);
		return;
	}

	bt_mesh_model_msg_init(msg, OP_BEACON_STATUS);
	net_buf_simple_add_u8(msg, bt_mesh_beacon_get());

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send Config Beacon Status response");
	}
}

static void default_ttl_get(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	/* Needed size: opcode (2 bytes) + msg + MIC */
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 1 + 4);

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	bt_mesh_model_msg_init(msg, OP_DEFAULT_TTL_STATUS);
	net_buf_simple_add_u8(msg, bt_mesh_default_ttl_get());

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send Default TTL Status response");
	}
}

static void default_ttl_set(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	/* Needed size: opcode (2 bytes) + msg + MIC */
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 1 + 4);
	struct bt_mesh_cfg *cfg = model->user_data;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	if (!cfg) {
		BT_WARN("No Configuration Server context available");
	} else if (buf->data[0] <= 0x7f && buf->data[0] != 0x01) {
		cfg->default_ttl = buf->data[0];
	} else {
		BT_WARN("Prohibited Default TTL value 0x%02x", buf->data[0]);
		return;
	}

	bt_mesh_model_msg_init(msg, OP_DEFAULT_TTL_STATUS);
	net_buf_simple_add_u8(msg, bt_mesh_default_ttl_get());

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send Default TTL Status response");
	}
}

static void send_gatt_proxy_status(struct bt_mesh_model *model,
				   struct bt_mesh_msg_ctx *ctx)
{
	/* Needed size: opcode (2 bytes) + msg + MIC */
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 1 + 4);

	bt_mesh_model_msg_init(msg, OP_GATT_PROXY_STATUS);
	net_buf_simple_add_u8(msg, bt_mesh_gatt_proxy_get());

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send GATT Proxy Status");
	}
}

static void gatt_proxy_get(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	send_gatt_proxy_status(model, ctx);
}

static void gatt_proxy_set(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_cfg *cfg = model->user_data;
	struct bt_mesh_subnet *sub;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	if (buf->data[0] != 0x00 && buf->data[0] != 0x01) {
		BT_WARN("Invalid GATT Proxy value 0x%02x", buf->data[0]);
		return;
	}

	if (!IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY) ||
	    bt_mesh_gatt_proxy_get() == BT_MESH_GATT_PROXY_NOT_SUPPORTED) {
		goto send_status;
	}

	if (!cfg) {
		BT_WARN("No Configuration Server context available");
		goto send_status;
	}

	BT_DBG("GATT Proxy 0x%02x -> 0x%02x", cfg->gatt_proxy, buf->data[0]);

	if (cfg->gatt_proxy == buf->data[0]) {
		goto send_status;
	}

	cfg->gatt_proxy = buf->data[0];

	sub = bt_mesh_subnet_get(cfg->hb_pub.net_idx);
	if ((cfg->hb_pub.feat & BT_MESH_FEAT_PROXY) && sub) {
		hb_send(model);
	}

send_status:
	send_gatt_proxy_status(model, ctx);
}

static void net_transmit_get(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	/* Needed size: opcode (2 bytes) + msg + MIC */
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 1 + 4);

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	bt_mesh_model_msg_init(msg, OP_NET_TRANSMIT_STATUS);
	net_buf_simple_add_u8(msg, bt_mesh_net_transmit_get());

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send Config Network Transmit Status");
	}
}

static void net_transmit_set(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	/* Needed size: opcode (2 bytes) + msg + MIC */
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 1 + 4);
	struct bt_mesh_cfg *cfg = model->user_data;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	BT_DBG("Transmit 0x%02x (count %u interval %ums)", buf->data[0],
	       TRANSMIT_COUNT(buf->data[0]), TRANSMIT_INT(buf->data[0]));

	if (!cfg) {
		BT_WARN("No Configuration Server context available");
	} else {
		cfg->net_transmit = buf->data[0];
	}

	bt_mesh_model_msg_init(msg, OP_NET_TRANSMIT_STATUS);
	net_buf_simple_add_u8(msg, bt_mesh_net_transmit_get());

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send Network Transmit Status");
	}
}

static void relay_get(struct bt_mesh_model *model,
		      struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf)
{
	/* Needed size: opcode (2 bytes) + msg + MIC */
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 2 + 4);

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	bt_mesh_model_msg_init(msg, OP_RELAY_STATUS);
	net_buf_simple_add_u8(msg, bt_mesh_relay_get());
	net_buf_simple_add_u8(msg, bt_mesh_relay_retransmit_get());

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send Config Relay Status response");
	}
}

static void relay_set(struct bt_mesh_model *model,
		      struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf)
{
	/* Needed size: opcode (2 bytes) + msg + MIC */
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 2 + 4);
	struct bt_mesh_cfg *cfg = model->user_data;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	if (!cfg) {
		BT_WARN("No Configuration Server context available");
	} else if (buf->data[0] == 0x00 || buf->data[0] == 0x01) {
		bool change = (cfg->relay != buf->data[0]);
		struct bt_mesh_subnet *sub;

		cfg->relay = buf->data[0];
		cfg->relay_retransmit = buf->data[1];

		BT_DBG("Relay 0x%02x Retransmit 0x%02x (count %u interval %u)",
		       cfg->relay, cfg->relay_retransmit,
		       TRANSMIT_COUNT(cfg->relay_retransmit),
		       TRANSMIT_INT(cfg->relay_retransmit))

		sub = bt_mesh_subnet_get(cfg->hb_pub.net_idx);
		if ((cfg->hb_pub.feat & BT_MESH_FEAT_RELAY) && sub && change) {
			hb_send(model);
		}
	} else {
		BT_WARN("Invalid Relay value 0x%02x", buf->data[0]);
		return;
	}

	bt_mesh_model_msg_init(msg, OP_RELAY_STATUS);
	net_buf_simple_add_u8(msg, bt_mesh_relay_get());
	net_buf_simple_add_u8(msg, bt_mesh_relay_retransmit_get());

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send Relay Status response");
	}
}

static void send_mod_pub_status(struct bt_mesh_model *cfg_mod,
				struct bt_mesh_msg_ctx *ctx,
				u16_t elem_addr, u16_t pub_addr,
				bool vnd, struct bt_mesh_model *mod,
				u8_t status, u8_t *mod_id)
{
	/* Needed size: opcode (2 bytes) + msg + MIC */
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 14 + 4);

	bt_mesh_model_msg_init(msg, OP_MOD_PUB_STATUS);

	net_buf_simple_add_u8(msg, status);
	net_buf_simple_add_le16(msg, elem_addr);

	if (status != STATUS_SUCCESS) {
		memset(net_buf_simple_add(msg, 7), 0, 7);
	} else {
		u16_t idx_cred;

		net_buf_simple_add_le16(msg, pub_addr);

		idx_cred = mod->pub->key | (u16_t)mod->pub->cred << 12;
		net_buf_simple_add_le16(msg, idx_cred);
		net_buf_simple_add_u8(msg, mod->pub->ttl);
		net_buf_simple_add_u8(msg, mod->pub->period);
		net_buf_simple_add_u8(msg, mod->pub->retransmit);
	}

	if (vnd) {
		memcpy(net_buf_simple_add(msg, 4), mod_id, 4);
	} else {
		memcpy(net_buf_simple_add(msg, 2), mod_id, 2);
	}

	bt_mesh_model_send(cfg_mod, ctx, msg, NULL, NULL);
}

static void mod_pub_get(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	u16_t elem_addr, pub_addr = 0;
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	u8_t *mod_id, status;
	bool vnd;

	elem_addr = net_buf_simple_pull_le16(buf);
	mod_id = buf->data;

	BT_DBG("elem_addr 0x%04x", elem_addr);

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4);
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	mod = get_model(elem, buf, &vnd);
	if (!mod) {
		status = STATUS_INVALID_MODEL;
		goto send_status;
	}

	if (!mod->pub) {
		status = STATUS_NVAL_PUB_PARAM;
		goto send_status;
	}

	pub_addr = mod->pub->addr;
	status = STATUS_SUCCESS;

send_status:
	send_mod_pub_status(model, ctx, elem_addr, pub_addr, vnd, mod,
			    status, mod_id);
}

static void mod_pub_set(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	u8_t retransmit, status, pub_ttl, pub_period, cred_flag;
	u16_t elem_addr, pub_addr, pub_app_idx;
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	u8_t *mod_id;
	bool vnd;

	elem_addr = net_buf_simple_pull_le16(buf);
	pub_addr = net_buf_simple_pull_le16(buf);
	pub_app_idx = net_buf_simple_pull_le16(buf);
	cred_flag = ((pub_app_idx >> 12) & BIT_MASK(1));
	pub_app_idx &= BIT_MASK(12);

	pub_ttl = net_buf_simple_pull_u8(buf);
	if (pub_ttl > 0x7f && pub_ttl != BT_MESH_TTL_DEFAULT) {
		BT_ERR("Invalid TTL value 0x%02x", pub_ttl);
		return;
	}

	pub_period = net_buf_simple_pull_u8(buf);
	retransmit = net_buf_simple_pull_u8(buf);
	mod_id = buf->data;

	BT_DBG("elem_addr 0x%04x pub_addr 0x%04x cred_flag %u",
	       elem_addr, pub_addr, cred_flag);
	BT_DBG("pub_app_idx 0x%03x, pub_ttl %u pub_period 0x%02x",
	       pub_app_idx, pub_ttl, pub_period);
	BT_DBG("retransmit 0x%02x (count %u interval %ums)", retransmit,
	       TRANSMIT_COUNT(retransmit), TRANSMIT_INT(retransmit));

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4);
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	mod = get_model(elem, buf, &vnd);
	if (!mod) {
		status = STATUS_INVALID_MODEL;
		goto send_status;
	}

	status = _mod_pub_set(mod, pub_addr, pub_app_idx, cred_flag, pub_ttl,
			      pub_period, retransmit);

send_status:
	send_mod_pub_status(model, ctx, elem_addr, pub_addr, vnd, mod,
			    status, mod_id);
}

#if CONFIG_BT_MESH_LABEL_COUNT > 0
static u16_t va_find(u8_t *label_uuid, struct label **free_slot)
{
	int i;

	if (free_slot) {
		*free_slot = NULL;
	}

	for (i = 0; i < ARRAY_SIZE(labels); i++) {
		if (!BT_MESH_ADDR_IS_VIRTUAL(labels[i].addr)) {
			if (free_slot) {
				*free_slot = &labels[i];
			}
			continue;
		}

		if (!memcmp(labels[i].uuid, label_uuid, 16)) {
			return labels[i].addr;
		}
	}

	return BT_MESH_ADDR_UNASSIGNED;
}

static u8_t va_add(u8_t *label_uuid, u16_t *addr)
{
	struct label *free_slot;

	*addr = va_find(label_uuid, &free_slot);
	if (*addr != BT_MESH_ADDR_UNASSIGNED) {
		return STATUS_SUCCESS;
	}

	if (!free_slot) {
		return STATUS_INSUFF_RESOURCES;
	}

	if (bt_mesh_virtual_addr(label_uuid, addr) < 0) {
		return STATUS_UNSPECIFIED;
	}

	free_slot->addr = *addr;
	memcpy(free_slot->uuid, label_uuid, 16);

	return STATUS_SUCCESS;
}

static void mod_pub_va_set(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	u8_t retransmit, status, pub_ttl, pub_period, cred_flag;
	u16_t elem_addr, pub_addr, pub_app_idx;
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	u8_t *label_uuid;
	u8_t *mod_id;
	bool vnd;

	elem_addr = net_buf_simple_pull_le16(buf);
	label_uuid = buf->data;
	net_buf_simple_pull(buf, 16);

	pub_app_idx = net_buf_simple_pull_le16(buf);
	cred_flag = ((pub_app_idx >> 12) & BIT_MASK(1));
	pub_app_idx &= BIT_MASK(12);
	pub_ttl = net_buf_simple_pull_u8(buf);
	if (pub_ttl > 0x7f && pub_ttl != BT_MESH_TTL_DEFAULT) {
		BT_ERR("Invalid TTL value 0x%02x", pub_ttl);
		return;
	}

	pub_period = net_buf_simple_pull_u8(buf);
	retransmit = net_buf_simple_pull_u8(buf);
	mod_id = buf->data;

	BT_DBG("elem_addr 0x%04x pub_addr 0x%04x cred_flag %u",
	       elem_addr, pub_addr, cred_flag);
	BT_DBG("pub_app_idx 0x%03x, pub_ttl %u pub_period 0x%02x",
	       pub_app_idx, pub_ttl, pub_period);
	BT_DBG("retransmit 0x%02x (count %u interval %ums)", retransmit,
	       TRANSMIT_COUNT(retransmit), TRANSMIT_INT(retransmit));

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4);
		pub_addr = 0;
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	mod = get_model(elem, buf, &vnd);
	if (!mod) {
		pub_addr = 0;
		status = STATUS_INVALID_MODEL;
		goto send_status;
	}

	status = va_add(label_uuid, &pub_addr);
	if (status == STATUS_SUCCESS) {
		status = _mod_pub_set(mod, pub_addr, pub_app_idx, cred_flag,
				      pub_ttl, pub_period, retransmit);
	}

send_status:
	send_mod_pub_status(model, ctx, elem_addr, pub_addr, vnd, mod,
			    status, mod_id);
}
#else
static void mod_pub_va_set(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	u8_t *mod_id, status;
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	u16_t elem_addr, pub_addr = 0;
	bool vnd;

	elem_addr = net_buf_simple_pull_le16(buf);
	net_buf_simple_pull(buf, 16);
	mod_id = net_buf_simple_pull(buf, 4);

	BT_DBG("elem_addr 0x%04x", elem_addr);

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4);
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	mod = get_model(elem, buf, &vnd);
	if (!mod) {
		status = STATUS_INVALID_MODEL;
		goto send_status;
	}

	if (!mod->pub) {
		status = STATUS_NVAL_PUB_PARAM;
		goto send_status;
	}

	pub_addr = mod->pub->addr;
	status = STATUS_INSUFF_RESOURCES;

send_status:
	send_mod_pub_status(model, ctx, elem_addr, pub_addr, vnd, mod,
			    status, mod_id);
}
#endif /* CONFIG_BT_MESH_LABEL_COUNT > 0 */

static void send_mod_sub_status(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx, u8_t status,
				u16_t elem_addr, u16_t sub_addr, u8_t *mod_id,
				bool vnd)
{
	/* Needed size: opcode (2 bytes) + msg + MIC */
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 9 + 4);

	bt_mesh_model_msg_init(msg, OP_MOD_SUB_STATUS);

	net_buf_simple_add_u8(msg, status);
	net_buf_simple_add_le16(msg, elem_addr);
	net_buf_simple_add_le16(msg, sub_addr);

	if (vnd) {
		memcpy(net_buf_simple_add(msg, 4), mod_id, 4);
	} else {
		memcpy(net_buf_simple_add(msg, 2), mod_id, 2);
	}

	bt_mesh_model_send(model, ctx, msg, NULL, NULL);
}

static void mod_sub_add(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	u16_t elem_addr, sub_addr;
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	u8_t *mod_id;
	u8_t status;
	bool vnd;
	int i;

	elem_addr = net_buf_simple_pull_le16(buf);
	sub_addr = net_buf_simple_pull_le16(buf);

	BT_DBG("elem_addr 0x%04x, sub_addr 0x%04x", elem_addr, sub_addr);

	mod_id = buf->data;

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4);
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	mod = get_model(elem, buf, &vnd);
	if (!mod) {
		status = STATUS_INVALID_MODEL;
		goto send_status;
	}

	if (!BT_MESH_ADDR_IS_GROUP(sub_addr)) {
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	if (bt_mesh_model_find_group(mod, sub_addr)) {
		/* Tried to add existing subscription */
		status = STATUS_SUCCESS;
		goto send_status;
	}

	for (i = 0; i < ARRAY_SIZE(mod->groups); i++) {
		if (mod->groups[i] == BT_MESH_ADDR_UNASSIGNED) {
			mod->groups[i] = sub_addr;
			break;
		}
	}

	if (i == ARRAY_SIZE(mod->groups)) {
		status = STATUS_INSUFF_RESOURCES;
	} else {
		status = STATUS_SUCCESS;

		if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)) {
			bt_mesh_lpn_group_add(sub_addr);
		}
	}

send_status:
	send_mod_sub_status(model, ctx, status, elem_addr, sub_addr,
			    mod_id, vnd);
}

static void mod_sub_del(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	u16_t elem_addr, sub_addr;
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	u8_t *mod_id;
	u16_t *match;
	u8_t status;
	bool vnd;

	elem_addr = net_buf_simple_pull_le16(buf);
	sub_addr = net_buf_simple_pull_le16(buf);

	BT_DBG("elem_addr 0x%04x sub_addr 0x%04x", elem_addr, sub_addr);

	mod_id = buf->data;

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4);
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	mod = get_model(elem, buf, &vnd);
	if (!mod) {
		status = STATUS_INVALID_MODEL;
		goto send_status;
	}

	if (!BT_MESH_ADDR_IS_GROUP(sub_addr)) {
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	/* An attempt to remove a non-existing address shall be treated
	 * as a success.
	 */
	status = STATUS_SUCCESS;

	if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)) {
		bt_mesh_lpn_group_del(&sub_addr, 1);
	}

	match = bt_mesh_model_find_group(mod, sub_addr);
	if (match) {
		*match = BT_MESH_ADDR_UNASSIGNED;
	}

send_status:
	send_mod_sub_status(model, ctx, status, elem_addr, sub_addr,
			    mod_id, vnd);
}

static void mod_sub_overwrite(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	u16_t elem_addr, sub_addr;
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	u8_t *mod_id;
	u8_t status;
	bool vnd;

	elem_addr = net_buf_simple_pull_le16(buf);
	sub_addr = net_buf_simple_pull_le16(buf);

	BT_DBG("elem_addr 0x%04x sub_addr 0x%04x", elem_addr, sub_addr);

	mod_id = buf->data;

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4);
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	mod = get_model(elem, buf, &vnd);
	if (!mod) {
		status = STATUS_INVALID_MODEL;
		goto send_status;
	}

	if (!BT_MESH_ADDR_IS_GROUP(sub_addr)) {
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)) {
		bt_mesh_lpn_group_del(mod->groups, ARRAY_SIZE(mod->groups));
	}

	/* Clear all subscriptions (0x0000 is the unassigned address) */
	memset(mod->groups, 0, sizeof(mod->groups));

	if (ARRAY_SIZE(mod->groups) > 0) {
		mod->groups[0] = sub_addr;
		status = STATUS_SUCCESS;

		if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)) {
			bt_mesh_lpn_group_add(sub_addr);
		}
	} else {
		status = STATUS_INSUFF_RESOURCES;
	}


send_status:
	send_mod_sub_status(model, ctx, status, elem_addr, sub_addr,
			    mod_id, vnd);
}

static void mod_sub_del_all(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	u16_t elem_addr;
	u8_t *mod_id;
	u8_t status;
	bool vnd;

	elem_addr = net_buf_simple_pull_le16(buf);

	BT_DBG("elem_addr 0x%04x", elem_addr);

	mod_id = buf->data;

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4);
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	mod = get_model(elem, buf, &vnd);
	if (!mod) {
		status = STATUS_INVALID_MODEL;
		goto send_status;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)) {
		bt_mesh_lpn_group_del(mod->groups, ARRAY_SIZE(mod->groups));
	}

	/* Clear all subscriptions (0x0000 is the unassigned address) */
	memset(mod->groups, 0, sizeof(mod->groups));

	status = STATUS_SUCCESS;

send_status:
	send_mod_sub_status(model, ctx, status, elem_addr,
			    BT_MESH_ADDR_UNASSIGNED, mod_id, vnd);
}

static void mod_sub_get(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	struct net_buf_simple *msg =
		NET_BUF_SIMPLE(2 + 5 + 4 +
			       CONFIG_BT_MESH_MODEL_GROUP_COUNT * 2);
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	u16_t addr, id;
	int i;

	addr = net_buf_simple_pull_le16(buf);
	id = net_buf_simple_pull_le16(buf);

	BT_DBG("addr 0x%04x id 0x%04x", addr, id);

	bt_mesh_model_msg_init(msg, OP_MOD_SUB_LIST);

	elem = bt_mesh_elem_find(addr);
	if (!elem) {
		net_buf_simple_add_u8(msg, STATUS_INVALID_ADDRESS);
		net_buf_simple_add_le16(msg, addr);
		net_buf_simple_add_le16(msg, id);
		goto send_list;
	}

	mod = bt_mesh_model_find(elem, id);
	if (!mod) {
		net_buf_simple_add_u8(msg, STATUS_INVALID_MODEL);
		net_buf_simple_add_le16(msg, addr);
		net_buf_simple_add_le16(msg, id);
		goto send_list;
	}

	net_buf_simple_add_u8(msg, STATUS_SUCCESS);

	net_buf_simple_add_le16(msg, addr);
	net_buf_simple_add_le16(msg, id);

	for (i = 0; i < ARRAY_SIZE(mod->groups); i++) {
		if (mod->groups[i] != BT_MESH_ADDR_UNASSIGNED) {
			net_buf_simple_add_le16(msg, mod->groups[i]);
		}
	}

send_list:
	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send Model Subscription List");
	}
}

static void mod_sub_get_vnd(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	struct net_buf_simple *msg =
		NET_BUF_SIMPLE(2 + 7 + 4 +
			       CONFIG_BT_MESH_MODEL_GROUP_COUNT * 2);
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	u16_t company, addr, id;
	int i;

	addr = net_buf_simple_pull_le16(buf);
	company = net_buf_simple_pull_le16(buf);
	id = net_buf_simple_pull_le16(buf);

	BT_DBG("addr 0x%04x company 0x%04x id 0x%04x", addr, company, id);

	bt_mesh_model_msg_init(msg, OP_MOD_SUB_LIST_VND);

	elem = bt_mesh_elem_find(addr);
	if (!elem) {
		net_buf_simple_add_u8(msg, STATUS_INVALID_ADDRESS);
		net_buf_simple_add_le16(msg, addr);
		net_buf_simple_add_le16(msg, company);
		net_buf_simple_add_le16(msg, id);
		goto send_list;
	}

	mod = bt_mesh_model_find_vnd(elem, company, id);
	if (!mod) {
		net_buf_simple_add_u8(msg, STATUS_INVALID_MODEL);
		net_buf_simple_add_le16(msg, addr);
		net_buf_simple_add_le16(msg, company);
		net_buf_simple_add_le16(msg, id);
		goto send_list;
	}

	net_buf_simple_add_u8(msg, STATUS_SUCCESS);

	net_buf_simple_add_le16(msg, addr);
	net_buf_simple_add_le16(msg, company);
	net_buf_simple_add_le16(msg, id);

	for (i = 0; i < ARRAY_SIZE(mod->groups); i++) {
		if (mod->groups[i] != BT_MESH_ADDR_UNASSIGNED) {
			net_buf_simple_add_le16(msg, mod->groups[i]);
		}
	}

send_list:
	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send Vendor Model Subscription List");
	}
}

#if CONFIG_BT_MESH_LABEL_COUNT > 0
static void mod_sub_va_add(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	u16_t elem_addr, sub_addr;
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	u8_t *label_uuid;
	u8_t *mod_id;
	u8_t status;
	bool vnd;
	int i;

	elem_addr = net_buf_simple_pull_le16(buf);
	label_uuid = buf->data;
	net_buf_simple_pull(buf, 16);

	BT_DBG("elem_addr 0x%04x", elem_addr);

	mod_id = buf->data;
	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4);
		sub_addr = BT_MESH_ADDR_UNASSIGNED;
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	mod = get_model(elem, buf, &vnd);
	if (!mod) {
		sub_addr = BT_MESH_ADDR_UNASSIGNED;
		status = STATUS_INVALID_MODEL;
		goto send_status;
	}

	status = va_add(label_uuid, &sub_addr);
	if (status != STATUS_SUCCESS) {
		goto send_status;
	}

	if (bt_mesh_model_find_group(mod, sub_addr)) {
		/* Tried to add existing subscription */
		status = STATUS_SUCCESS;
		goto send_status;
	}

	for (i = 0; i < ARRAY_SIZE(mod->groups); i++) {
		if (mod->groups[i] == BT_MESH_ADDR_UNASSIGNED) {
			mod->groups[i] = sub_addr;
			break;
		}
	}

	if (i == ARRAY_SIZE(mod->groups)) {
		status = STATUS_INSUFF_RESOURCES;
	} else {
		if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)) {
			bt_mesh_lpn_group_add(sub_addr);
		}

		status = STATUS_SUCCESS;
	}

send_status:
	send_mod_sub_status(model, ctx, status, elem_addr, sub_addr,
			    mod_id, vnd);
}

static void mod_sub_va_del(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	u16_t elem_addr, sub_addr;
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	u8_t *label_uuid;
	u8_t *mod_id;
	u16_t *match;
	u8_t status;
	bool vnd;

	elem_addr = net_buf_simple_pull_le16(buf);
	label_uuid = buf->data;
	net_buf_simple_pull(buf, 16);

	BT_DBG("elem_addr 0x%04x", elem_addr);

	mod_id = buf->data;

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4);
		sub_addr = BT_MESH_ADDR_UNASSIGNED;
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	mod = get_model(elem, buf, &vnd);
	if (!mod) {
		sub_addr = BT_MESH_ADDR_UNASSIGNED;
		status = STATUS_INVALID_MODEL;
		goto send_status;
	}

	sub_addr = va_find(label_uuid, NULL);
	if (sub_addr == BT_MESH_ADDR_UNASSIGNED) {
		status = STATUS_CANNOT_REMOVE;
		goto send_status;
	}

	match = bt_mesh_model_find_group(mod, sub_addr);
	if (match) {
		*match = BT_MESH_ADDR_UNASSIGNED;
		status = STATUS_SUCCESS;
	} else {
		status = STATUS_CANNOT_REMOVE;
	}

send_status:
	send_mod_sub_status(model, ctx, status, elem_addr, sub_addr,
			    mod_id, vnd);
}

static void mod_sub_va_overwrite(struct bt_mesh_model *model,
				 struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	u16_t elem_addr, sub_addr = BT_MESH_ADDR_UNASSIGNED;
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	u8_t *label_uuid;
	u8_t *mod_id;
	u8_t status;
	bool vnd;

	elem_addr = net_buf_simple_pull_le16(buf);
	label_uuid = buf->data;
	net_buf_simple_pull(buf, 16);

	BT_DBG("elem_addr 0x%04x", elem_addr);

	mod_id = buf->data;

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4);
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	mod = get_model(elem, buf, &vnd);
	if (!mod) {
		status = STATUS_INVALID_MODEL;
		goto send_status;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)) {
		bt_mesh_lpn_group_del(mod->groups, ARRAY_SIZE(mod->groups));
	}

	/* Clear all subscriptions (0x0000 is the unassigned address) */
	memset(mod->groups, 0, sizeof(mod->groups));

	if (ARRAY_SIZE(mod->groups) > 0) {
		status = va_add(label_uuid, &sub_addr);
		if (status == STATUS_SUCCESS) {
			mod->groups[0] = sub_addr;

			if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)) {
				bt_mesh_lpn_group_add(sub_addr);
			}
		}
	} else {
		status = STATUS_INSUFF_RESOURCES;
	}

send_status:
	send_mod_sub_status(model, ctx, status, elem_addr, sub_addr,
			    mod_id, vnd);
}
#else
static void mod_sub_va_add(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	u16_t elem_addr;
	u8_t *mod_id;
	u8_t status;
	bool vnd;

	elem_addr = net_buf_simple_pull_le16(buf);
	net_buf_simple_pull(buf, 16);

	mod_id = buf->data;

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4);
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	mod = get_model(elem, buf, &vnd);
	if (!mod) {
		status = STATUS_INVALID_MODEL;
		goto send_status;
	}

	status = STATUS_INSUFF_RESOURCES;

send_status:
	send_mod_sub_status(model, ctx, status, elem_addr,
			    BT_MESH_ADDR_UNASSIGNED, mod_id, vnd);
}

static void mod_sub_va_del(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_elem *elem;
	u16_t elem_addr;
	u8_t *mod_id;
	u8_t status;
	bool vnd;

	elem_addr = net_buf_simple_pull_le16(buf);
	net_buf_simple_pull(buf, 16);

	mod_id = buf->data;

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		vnd = (buf->len == 4);
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	if (!get_model(elem, buf, &vnd)) {
		status = STATUS_INVALID_MODEL;
		goto send_status;
	}

	status = STATUS_INSUFF_RESOURCES;

send_status:
	send_mod_sub_status(model, ctx, status, elem_addr,
			    BT_MESH_ADDR_UNASSIGNED, mod_id, vnd);
}

static void mod_sub_va_overwrite(struct bt_mesh_model *model,
				 struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	struct bt_mesh_elem *elem;
	u16_t elem_addr;
	u8_t *mod_id;
	u8_t status;
	bool vnd;

	elem_addr = net_buf_simple_pull_le16(buf);
	net_buf_simple_pull(buf, 18);

	mod_id = buf->data;

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		vnd = (buf->len == 4);
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	if (!get_model(elem, buf, &vnd)) {
		status = STATUS_INVALID_MODEL;
		goto send_status;
	}

	status = STATUS_INSUFF_RESOURCES;

send_status:
	send_mod_sub_status(model, ctx, status, elem_addr,
			    BT_MESH_ADDR_UNASSIGNED, mod_id, vnd);
}
#endif /* CONFIG_BT_MESH_LABEL_COUNT > 0 */

static void send_net_key_status(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				u16_t idx, u8_t status)
{
	/* Needed size: opcode (2 bytes) + msg + MIC */
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 3 + 4);

	bt_mesh_model_msg_init(msg, OP_NET_KEY_STATUS);

	net_buf_simple_add_u8(msg, status);
	net_buf_simple_add_le16(msg, idx);

	bt_mesh_model_send(model, ctx, msg, NULL, NULL);
}

static void net_key_add(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	struct bt_mesh_subnet *sub;
	u16_t idx;
	int err;

	idx = net_buf_simple_pull_le16(buf);
	if (idx > 0xfff) {
		BT_ERR("Invalid NetKeyIndex 0x%04x", idx);
		return;
	}

	BT_DBG("idx 0x%04x", idx);

	sub = bt_mesh_subnet_get(idx);
	if (!sub) {
		int i;

		for (sub = NULL, i = 0; i < ARRAY_SIZE(bt_mesh.sub); i++) {
			if (bt_mesh.sub[i].net_idx == BT_MESH_KEY_UNUSED) {
				sub = &bt_mesh.sub[i];
				break;
			}
		}

		if (!sub) {
			send_net_key_status(model, ctx, idx,
					    STATUS_INSUFF_RESOURCES);
			return;
		}
	}

	/* Check for already existing subnet */
	if (sub->net_idx == idx) {
		u8_t status;

		if (memcmp(buf->data, sub->keys[0].net, 16)) {
			status = STATUS_IDX_ALREADY_STORED;
		} else {
			status = STATUS_SUCCESS;
		}

		send_net_key_status(model, ctx, idx, status);
		return;
	}

	err = bt_mesh_net_keys_create(&sub->keys[0], buf->data);
	if (err) {
		send_net_key_status(model, ctx, idx, STATUS_UNSPECIFIED);
		return;
	}

	sub->net_idx = idx;

	if (IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY)) {
		sub->node_id = BT_MESH_NODE_IDENTITY_STOPPED;
		bt_mesh_proxy_beacon_send(sub);
		bt_mesh_adv_update();
	} else {
		sub->node_id = BT_MESH_NODE_IDENTITY_NOT_SUPPORTED;
	}

	send_net_key_status(model, ctx, idx, STATUS_SUCCESS);
}

static void net_key_update(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_subnet *sub;
	u16_t idx;
	int err;

	idx = net_buf_simple_pull_le16(buf);
	if (idx > 0xfff) {
		BT_ERR("Invalid NetKeyIndex 0x%04x", idx);
		return;
	}

	BT_DBG("idx 0x%04x", idx);

	sub = bt_mesh_subnet_get(idx);
	if (!sub) {
		send_net_key_status(model, ctx, idx, STATUS_INVALID_NETKEY);
		return;
	}

	/* The node shall successfully process a NetKey Update message on a
	 * valid NetKeyIndex when the NetKey value is different and the Key
	 * Refresh procedure has not been started, or when the NetKey value is
	 * the same in Phase 1. The NetKey Update message shall generate an
	 * error when the node is in Phase 2, or Phase 3.
	 */
	switch (sub->kr_phase) {
	case BT_MESH_KR_NORMAL:
		if (!memcmp(buf->data, sub->keys[0].net, 16)) {
			return;
		}
		break;
	case BT_MESH_KR_PHASE_1:
		if (!memcmp(buf->data, sub->keys[1].net, 16)) {
			send_net_key_status(model, ctx, idx, STATUS_SUCCESS);
			return;
		}
		break;
	case BT_MESH_KR_PHASE_2:
	case BT_MESH_KR_PHASE_3:
		send_net_key_status(model, ctx, idx, STATUS_CANNOT_UPDATE);
		return;
	}

	err = bt_mesh_net_keys_create(&sub->keys[1], buf->data);
	if (!err && (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER) ||
		     IS_ENABLED(CONFIG_BT_MESH_FRIEND))) {
		err = bt_mesh_friend_cred_update(ctx->net_idx, 1, buf->data);
	}

	if (err) {
		send_net_key_status(model, ctx, idx, STATUS_UNSPECIFIED);
		return;
	}

	sub->kr_phase = BT_MESH_KR_PHASE_1;

	bt_mesh_net_beacon_update(sub);

	send_net_key_status(model, ctx, idx, STATUS_SUCCESS);
}

static void hb_pub_disable(struct bt_mesh_cfg *cfg)
{
	BT_DBG("");

	cfg->hb_pub.dst = BT_MESH_ADDR_UNASSIGNED;
	cfg->hb_pub.count = 0;
	cfg->hb_pub.ttl = 0;
	cfg->hb_pub.period = 0;

	k_delayed_work_cancel(&cfg->hb_pub.timer);
}

static void net_key_del(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	struct bt_mesh_cfg *cfg = model->user_data;
	struct bt_mesh_subnet *sub;
	u16_t del_idx, i;
	u8_t status;

	del_idx = net_buf_simple_pull_le16(buf);
	if (del_idx > 0xfff) {
		BT_ERR("Invalid NetKeyIndex 0x%04x", del_idx);
		return;
	}

	BT_DBG("idx 0x%04x", del_idx);

	sub = bt_mesh_subnet_get(del_idx);
	if (!sub) {
		/* This could be a retry of a previous attempt that had its
		 * response lost, so pretend that it was a success.
		 */
		status = STATUS_SUCCESS;
		goto send_status;
	}

	/* The key that the message was encrypted with cannot be removed.
	 * The NetKey List must contain a minimum of one NetKey.
	 */
	if (ctx->net_idx == del_idx) {
		status = STATUS_CANNOT_REMOVE;
		goto send_status;
	}

	if (cfg->hb_pub.net_idx == del_idx) {
		hb_pub_disable(cfg);
	}

	/* Delete any app keys bound to this NetKey index */
	for (i = 0; i < ARRAY_SIZE(bt_mesh.app_keys); i++) {
		struct bt_mesh_app_key *key = &bt_mesh.app_keys[i];

		if (key->net_idx == del_idx) {
			_app_key_del(key);
		}
	}

	if (IS_ENABLED(CONFIG_BT_MESH_FRIEND)) {
		bt_mesh_friend_clear_net_idx(del_idx);
	}

	memset(sub, 0, sizeof(*sub));
	sub->net_idx = BT_MESH_KEY_UNUSED;

	status = STATUS_SUCCESS;

send_status:
	send_net_key_status(model, ctx, del_idx, status);
}

static void net_key_get(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	struct net_buf_simple *msg =
		NET_BUF_SIMPLE(2 + 4 + IDX_LEN(CONFIG_BT_MESH_SUBNET_COUNT));
	u16_t prev, i;

	bt_mesh_model_msg_init(msg, OP_NET_KEY_LIST);

	prev = BT_MESH_KEY_UNUSED;
	for (i = 0; i < ARRAY_SIZE(bt_mesh.sub); i++) {
		struct bt_mesh_subnet *sub = &bt_mesh.sub[i];

		if (sub->net_idx == BT_MESH_KEY_UNUSED) {
			continue;
		}

		if (prev == BT_MESH_KEY_UNUSED) {
			prev = sub->net_idx;
			continue;
		}

		key_idx_pack(msg, prev, sub->net_idx);
		prev = BT_MESH_KEY_UNUSED;
	}

	if (prev != BT_MESH_KEY_UNUSED) {
		net_buf_simple_add_le16(msg, prev);
	}

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send NetKey List");
	}
}

static void node_identity_get(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	/* Needed size: opcode (2 bytes) + msg + MIC */
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 4 + 4);
	struct bt_mesh_subnet *sub;
	u8_t node_id;
	u16_t idx;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	idx = net_buf_simple_pull_le16(buf);
	if (idx > 0xfff) {
		BT_ERR("Invalid NetKeyIndex 0x%04x", idx);
		return;
	}

	bt_mesh_model_msg_init(msg, OP_NODE_IDENTITY_STATUS);

	sub = bt_mesh_subnet_get(idx);
	if (!sub) {
		net_buf_simple_add_u8(msg, STATUS_INVALID_NETKEY);
		node_id = 0x00;
	} else {
		net_buf_simple_add_u8(msg, STATUS_SUCCESS);
		node_id = sub->node_id;
	}

	net_buf_simple_add_le16(msg, idx);
	net_buf_simple_add_u8(msg, node_id);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send Node Identity Status");
	}
}

static void node_identity_set(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	/* Needed size: opcode (2 bytes) + msg + MIC */
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 4 + 4);
	struct bt_mesh_subnet *sub;
	u8_t node_id;
	u16_t idx;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	idx = net_buf_simple_pull_le16(buf);
	if (idx > 0xfff) {
		BT_WARN("Invalid NetKeyIndex 0x%04x", idx);
		return;
	}

	node_id = net_buf_simple_pull_u8(buf);
	if (node_id != 0x00 && node_id != 0x01) {
		BT_WARN("Invalid Node ID value 0x%02x", node_id);
		return;
	}

	bt_mesh_model_msg_init(msg, OP_NODE_IDENTITY_STATUS);

	sub = bt_mesh_subnet_get(idx);
	if (!sub) {
		net_buf_simple_add_u8(msg, STATUS_INVALID_NETKEY);
		net_buf_simple_add_le16(msg, idx);
		net_buf_simple_add_u8(msg, node_id);
	} else  {
		net_buf_simple_add_u8(msg, STATUS_SUCCESS);
		net_buf_simple_add_le16(msg, idx);

		if (IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY)) {
			sub->node_id = node_id;
			bt_mesh_adv_update();
		}

		net_buf_simple_add_u8(msg, sub->node_id);
	}

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send Node Identity Status");
	}
}

static void create_mod_app_status(struct net_buf_simple *msg,
				  struct bt_mesh_model *mod, bool vnd,
				  u16_t elem_addr, u16_t app_idx,
				  u8_t status, u8_t *mod_id)
{
	bt_mesh_model_msg_init(msg, OP_MOD_APP_STATUS);

	net_buf_simple_add_u8(msg, status);
	net_buf_simple_add_le16(msg, elem_addr);
	net_buf_simple_add_le16(msg, app_idx);

	if (vnd) {
		memcpy(net_buf_simple_add(msg, 4), mod_id, 4);
	} else {
		memcpy(net_buf_simple_add(msg, 2), mod_id, 2);
	}
}

static void mod_app_bind(struct bt_mesh_model *model,
			 struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 9 + 4);
	u16_t elem_addr, key_app_idx;
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	u8_t *mod_id, status;
	bool vnd;

	elem_addr = net_buf_simple_pull_le16(buf);
	key_app_idx = net_buf_simple_pull_le16(buf);
	mod_id = buf->data;

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4);
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	mod = get_model(elem, buf, &vnd);
	if (!mod) {
		status = STATUS_INVALID_MODEL;
		goto send_status;
	}

	/* Configuration Server only allows device key based access */
	if (model == mod) {
		BT_ERR("Client tried to bind AppKey to Configuration Model");
		status = STATUS_CANNOT_BIND;
		goto send_status;
	}

	status = mod_bind(mod, key_app_idx);

send_status:
	BT_DBG("status 0x%02x", status);
	create_mod_app_status(msg, mod, vnd, elem_addr, key_app_idx, status,
			      mod_id);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send Model App Bind Status response");
	}
}

static void mod_app_unbind(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 9 + 4);
	u16_t elem_addr, key_app_idx;
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	u8_t *mod_id, status;
	bool vnd;

	elem_addr = net_buf_simple_pull_le16(buf);
	key_app_idx = net_buf_simple_pull_le16(buf);
	mod_id = buf->data;

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4);
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	mod = get_model(elem, buf, &vnd);
	if (!mod) {
		status = STATUS_INVALID_MODEL;
		goto send_status;
	}

	status = mod_unbind(mod, key_app_idx);

send_status:
	BT_DBG("status 0x%02x", status);
	create_mod_app_status(msg, mod, vnd, elem_addr, key_app_idx, status,
			      mod_id);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send Model App Unbind Status response");
	}
}

#define KEY_LIST_LEN (CONFIG_BT_MESH_MODEL_KEY_COUNT * 2)

static void mod_app_get(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 9 + KEY_LIST_LEN + 4);
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	u8_t *mod_id, status;
	u16_t elem_addr;
	bool vnd;

	elem_addr = net_buf_simple_pull_le16(buf);
	mod_id = buf->data;

	BT_DBG("elem_addr 0x%04x", elem_addr);

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4);
		status = STATUS_INVALID_ADDRESS;
		goto send_list;
	}

	mod = get_model(elem, buf, &vnd);
	if (!mod) {
		status = STATUS_INVALID_MODEL;
		goto send_list;
	}

	status = STATUS_SUCCESS;

send_list:
	if (vnd) {
		bt_mesh_model_msg_init(msg, OP_VND_MOD_APP_LIST);
	} else {
		bt_mesh_model_msg_init(msg, OP_SIG_MOD_APP_LIST);
	}

	net_buf_simple_add_u8(msg, status);
	net_buf_simple_add_le16(msg, elem_addr);

	if (vnd) {
		net_buf_simple_add_mem(msg, mod_id, 4);
	} else {
		net_buf_simple_add_mem(msg, mod_id, 2);
	}

	if (mod) {
		int i;

		for (i = 0; i < ARRAY_SIZE(mod->keys); i++) {
			if (mod->keys[i] != BT_MESH_KEY_UNUSED) {
				net_buf_simple_add_le16(msg, mod->keys[i]);
			}
		}
	}

	bt_mesh_model_send(model, ctx, msg, NULL, NULL);
}

static void node_reset(struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	/* Needed size: opcode (2 bytes) + msg + MIC */
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 0 + 4);
	struct bt_mesh_cfg *cfg = model->user_data;
	int i;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));


	bt_mesh_model_msg_init(msg, OP_NODE_RESET_STATUS);

	/* Send the response first since we wont have any keys left to
	 * send it later.
	 */
	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send Node Reset Status");
	}

	/* Delete all app keys */
	for (i = 0; i < ARRAY_SIZE(bt_mesh.app_keys); i++) {
		struct bt_mesh_app_key *key = &bt_mesh.app_keys[i];

		if (key->net_idx != BT_MESH_KEY_UNUSED) {
			_app_key_del(key);
		}
	}

	for (i = 0; i < ARRAY_SIZE(bt_mesh.sub); i++) {
		struct bt_mesh_subnet *sub = &bt_mesh.sub[i];

		if (cfg->hb_pub.net_idx == sub->net_idx) {
			hb_pub_disable(cfg);
		}

		memset(sub, 0, sizeof(*sub));
		sub->net_idx = BT_MESH_KEY_UNUSED;
	}

	memset(labels, 0, sizeof(labels));

	bt_mesh_reset();
}

static void send_friend_status(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx)
{
	/* Needed size: opcode (2 bytes) + msg + MIC */
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 1 + 4);
	struct bt_mesh_cfg *cfg = model->user_data;

	bt_mesh_model_msg_init(msg, OP_FRIEND_STATUS);
	net_buf_simple_add_u8(msg, cfg->frnd);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send Friend Status");
	}
}

static void friend_get(struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	send_friend_status(model, ctx);
}

static void friend_set(struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	struct bt_mesh_cfg *cfg = model->user_data;
	struct bt_mesh_subnet *sub;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	if (buf->data[0] != 0x00 && buf->data[0] != 0x01) {
		BT_WARN("Invalid Friend value 0x%02x", buf->data[0]);
		return;
	}

	if (!cfg) {
		BT_WARN("No Configuration Server context available");
		goto send_status;
	}

	BT_DBG("Friend 0x%02x -> 0x%02x", cfg->frnd, buf->data[0]);

	if (cfg->frnd == buf->data[0]) {
		goto send_status;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_FRIEND)) {
		cfg->frnd = buf->data[0];

		if (cfg->frnd == BT_MESH_FRIEND_DISABLED) {
			bt_mesh_friend_clear_net_idx(BT_MESH_KEY_ANY);
		}
	}

	sub = bt_mesh_subnet_get(cfg->hb_pub.net_idx);
	if ((cfg->hb_pub.feat & BT_MESH_FEAT_FRIEND) && sub) {
		hb_send(model);
	}

send_status:
	send_friend_status(model, ctx);
}

static void lpn_timeout_get(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	/* Needed size: opcode (2 bytes) + msg + MIC */
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 5 + 4);
	struct bt_mesh_friend *frnd;
	u16_t lpn_addr;
	s32_t timeout;

	lpn_addr = net_buf_simple_pull_le16(buf);

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x lpn_addr 0x%02x",
	       ctx->net_idx, ctx->app_idx, ctx->addr, lpn_addr);

	if (!BT_MESH_ADDR_IS_UNICAST(lpn_addr)) {
		BT_WARN("Invalid LPNAddress; ignoring msg");
		return;
	}

	bt_mesh_model_msg_init(msg, OP_LPN_TIMEOUT_STATUS);
	net_buf_simple_add_le16(msg, lpn_addr);

	if (!IS_ENABLED(CONFIG_BLUETOOTH_MESH_FRIEND)) {
		timeout = 0;
		goto send_rsp;
	}

	frnd = bt_mesh_friend_find(BT_MESH_KEY_ANY, lpn_addr, true);
	if (!frnd) {
		timeout = 0;
		goto send_rsp;
	}

	timeout = k_delayed_work_remaining_get(&frnd->timer) / 100;

send_rsp:
	net_buf_simple_add_u8(msg, timeout);
	net_buf_simple_add_u8(msg, timeout >> 8);
	net_buf_simple_add_u8(msg, timeout >> 16);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send LPN PollTimeout Status");
	}
}

static void send_krp_status(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    u16_t idx, u8_t phase, u8_t status)
{
	/* Needed size: opcode (2 bytes) + msg + MIC */
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 4 + 4);

	bt_mesh_model_msg_init(msg, OP_KRP_STATUS);

	net_buf_simple_add_u8(msg, status);
	net_buf_simple_add_le16(msg, idx);
	net_buf_simple_add_u8(msg, phase);

	bt_mesh_model_send(model, ctx, msg, NULL, NULL);
}

static void krp_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		    struct net_buf_simple *buf)
{
	struct bt_mesh_subnet *sub;
	u16_t idx;

	idx = net_buf_simple_pull_le16(buf);
	if (idx > 0xfff) {
		BT_ERR("Invalid NetKeyIndex 0x%04x", idx);
		return;
	}

	BT_DBG("idx 0x%04x", idx);

	sub = bt_mesh_subnet_get(idx);
	if (!sub) {
		send_krp_status(model, ctx, idx, 0x00, STATUS_INVALID_NETKEY);
	} else {
		send_krp_status(model, ctx, idx, sub->kr_phase,
				STATUS_SUCCESS);
	}
}

static void krp_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		    struct net_buf_simple *buf)
{
	struct bt_mesh_subnet *sub;
	u8_t phase;
	u16_t idx;

	idx = net_buf_simple_pull_le16(buf);
	phase = net_buf_simple_pull_u8(buf);

	if (idx > 0xfff) {
		BT_ERR("Invalid NetKeyIndex 0x%04x", idx);
		return;
	}

	BT_DBG("idx 0x%04x transition 0x%02x", idx, phase);

	sub = bt_mesh_subnet_get(idx);
	if (!sub) {
		send_krp_status(model, ctx, idx, 0x00, STATUS_INVALID_NETKEY);
		return;
	}

	BT_DBG("%u -> %u", sub->kr_phase, phase);

	if (phase < BT_MESH_KR_PHASE_2 || phase > BT_MESH_KR_PHASE_3 ||
	    (sub->kr_phase == BT_MESH_KR_NORMAL &&
	     phase == BT_MESH_KR_PHASE_2)) {
		BT_WARN("Prohibited transition %u -> %u", sub->kr_phase, phase);
		return;
	}

	if (sub->kr_phase == BT_MESH_KR_PHASE_1 &&
	    phase == BT_MESH_KR_PHASE_2) {
		sub->kr_phase = BT_MESH_KR_PHASE_2;
		sub->kr_flag = 1;
		bt_mesh_net_beacon_update(sub);
	} else if ((sub->kr_phase == BT_MESH_KR_PHASE_1 ||
		    sub->kr_phase == BT_MESH_KR_PHASE_2) &&
		   phase == BT_MESH_KR_PHASE_3) {
		bt_mesh_net_revoke_keys(sub);
		if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER) ||
		    IS_ENABLED(CONFIG_BT_MESH_FRIEND)) {
			bt_mesh_friend_cred_refresh(ctx->net_idx);
		}
		sub->kr_phase = BT_MESH_KR_NORMAL;
		sub->kr_flag = 0;
		bt_mesh_net_beacon_update(sub);
	}

	send_krp_status(model, ctx, idx, sub->kr_phase, STATUS_SUCCESS);
}

static u8_t hb_log(u16_t val)
{
	if (!val) {
		return 0x00;
	} else if (val == 0xffff) {
		return 0xff;
	} else {
		return 32 - __builtin_clz(val);
	}
}

static u8_t hb_pub_count_log(u16_t val)
{
	if (!val) {
		return 0x00;
	} else if (val == 0x01) {
		return 0x01;
	} else if (val == 0xffff) {
		return 0xff;
	} else {
		return 32 - __builtin_clz(val - 1) + 1;
	}
}

static u16_t hb_pwr2(u8_t val, u8_t sub)
{
	if (!val) {
		return 0x0000;
	} else if (val == 0xff || val == 0x11) {
		return 0xffff;
	} else {
		return (1 << (val - sub));
	}
}

struct hb_pub_param {
	u16_t dst;
	u8_t  count_log;
	u8_t  period_log;
	u8_t  ttl;
	u16_t feat;
	u16_t net_idx;
} __packed;

static void hb_pub_send_status(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx, u8_t status,
			       struct hb_pub_param *orig_msg)
{
	/* Needed size: opcode (1 byte) + msg + MIC */
	struct net_buf_simple *msg = NET_BUF_SIMPLE(1 + 10 + 4);
	struct bt_mesh_cfg *cfg = model->user_data;

	BT_DBG("src 0x%04x status 0x%02x", ctx->addr, status);

	bt_mesh_model_msg_init(msg, OP_HEARTBEAT_PUB_STATUS);

	net_buf_simple_add_u8(msg, status);

	if (orig_msg) {
		memcpy(net_buf_simple_add(msg, sizeof(*orig_msg)), orig_msg,
		       sizeof(*orig_msg));
		goto send;
	}

	net_buf_simple_add_le16(msg, cfg->hb_pub.dst);
	net_buf_simple_add_u8(msg, hb_pub_count_log(cfg->hb_pub.count));
	net_buf_simple_add_u8(msg, cfg->hb_pub.period);
	net_buf_simple_add_u8(msg, cfg->hb_pub.ttl);
	net_buf_simple_add_le16(msg, cfg->hb_pub.feat);
	net_buf_simple_add_le16(msg, cfg->hb_pub.net_idx);

send:
	bt_mesh_model_send(model, ctx, msg, NULL, NULL);
}

static void heartbeat_pub_get(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	BT_DBG("src 0x%04x", ctx->addr);

	hb_pub_send_status(model, ctx, STATUS_SUCCESS, NULL);
}

static void heartbeat_pub_set(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	struct hb_pub_param *param = (void *)buf->data;
	struct bt_mesh_cfg *cfg = model->user_data;
	u16_t dst, feat, idx;
	u8_t status;

	BT_DBG("src 0x%04x", ctx->addr);

	dst = sys_le16_to_cpu(param->dst);
	/* All other address types but virtual are valid */
	if (BT_MESH_ADDR_IS_VIRTUAL(dst)) {
		status = STATUS_INVALID_ADDRESS;
		goto failed;
	}

	if (param->count_log > 0x11 && param->count_log != 0xff) {
		status = STATUS_CANNOT_SET;
		goto failed;
	}

	if (param->period_log > 0x10) {
		status = STATUS_CANNOT_SET;
		goto failed;
	}

	if (param->ttl > 0x7f && param->ttl != BT_MESH_TTL_DEFAULT) {
		BT_ERR("Invalid TTL value 0x%02x", param->ttl);
		return;
	}

	feat = sys_le16_to_cpu(param->feat);

	idx = sys_le16_to_cpu(param->net_idx);
	if (idx > 0xfff) {
		BT_ERR("Invalid NetKeyIndex 0x%04x", idx);
		return;
	}

	if (!bt_mesh_subnet_get(idx)) {
		status = STATUS_INVALID_NETKEY;
		goto failed;
	}

	cfg->hb_pub.dst = dst;
	cfg->hb_pub.period = param->period_log;
	cfg->hb_pub.feat = feat;
	cfg->hb_pub.net_idx = idx;

	if (dst == BT_MESH_ADDR_UNASSIGNED) {
		hb_pub_disable(cfg);
	} else {
		/* 2^(n-1) */
		cfg->hb_pub.count = hb_pwr2(param->count_log, 1);
		cfg->hb_pub.ttl = param->ttl;

		BT_DBG("period %u ms", hb_pwr2(param->period_log, 1) * 1000);

		/* The first Heartbeat message shall be published as soon
		 * as possible after the Heartbeat Publication Period state
		 * has been configured for periodic publishing.
		 */
		if (param->period_log && param->count_log) {
			k_work_submit(&cfg->hb_pub.timer.work);
		} else {
			k_delayed_work_cancel(&cfg->hb_pub.timer);
		}
	}

	hb_pub_send_status(model, ctx, STATUS_SUCCESS, NULL);

	return;

failed:
	hb_pub_send_status(model, ctx, status, param);
}

static void hb_sub_send_status(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx, u8_t status)
{
	/* Needed size: opcode (2 bytes) + msg + MIC */
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 9 + 4);
	struct bt_mesh_cfg *cfg = model->user_data;
	u16_t period;
	s64_t uptime;

	BT_DBG("src 0x%04x status 0x%02x", ctx->addr, status);

	uptime = k_uptime_get();
	if (uptime > cfg->hb_sub.expiry) {
		period = 0;
	} else {
		period = (cfg->hb_sub.expiry - uptime) / 1000;
	}

	bt_mesh_model_msg_init(msg, OP_HEARTBEAT_SUB_STATUS);

	net_buf_simple_add_u8(msg, status);

	net_buf_simple_add_le16(msg, cfg->hb_sub.src);
	net_buf_simple_add_le16(msg, cfg->hb_sub.dst);

	if (cfg->hb_sub.src == BT_MESH_ADDR_UNASSIGNED ||
	    cfg->hb_sub.dst == BT_MESH_ADDR_UNASSIGNED) {
		memset(net_buf_simple_add(msg, 4), 0, 4);
	} else {
		net_buf_simple_add_u8(msg, hb_log(period));
		net_buf_simple_add_u8(msg, hb_log(cfg->hb_sub.count));
		net_buf_simple_add_u8(msg, cfg->hb_sub.min_hops);
		net_buf_simple_add_u8(msg, cfg->hb_sub.max_hops);
	}

	bt_mesh_model_send(model, ctx, msg, NULL, NULL);
}

static void heartbeat_sub_get(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	BT_DBG("src 0x%04x", ctx->addr);

	hb_sub_send_status(model, ctx, STATUS_SUCCESS);
}

static void heartbeat_sub_set(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	struct bt_mesh_cfg *cfg = model->user_data;
	u16_t sub_src, sub_dst;
	u8_t sub_period;
	s32_t period_ms;

	BT_DBG("src 0x%04x", ctx->addr);

	sub_src = net_buf_simple_pull_le16(buf);
	sub_dst = net_buf_simple_pull_le16(buf);
	sub_period = net_buf_simple_pull_u8(buf);

	BT_DBG("sub_src 0x%04x sub_dst 0x%04x period 0x%02x",
	       sub_src, sub_dst, sub_period);

	if (sub_src != BT_MESH_ADDR_UNASSIGNED &&
	    !BT_MESH_ADDR_IS_UNICAST(sub_src)) {
		BT_WARN("Prohibited source address");
		return;
	}

	if (BT_MESH_ADDR_IS_VIRTUAL(sub_dst) || BT_MESH_ADDR_IS_RFU(sub_dst) ||
	    (BT_MESH_ADDR_IS_UNICAST(sub_dst) &&
	     sub_dst != bt_mesh_primary_addr())) {
		BT_WARN("Prohibited destination address");
		return;
	}

	if (sub_period > 0x11) {
		BT_WARN("Prohibited subscription period 0x%02x", sub_period);
		return;
	}

	if (sub_src == BT_MESH_ADDR_UNASSIGNED ||
	    sub_dst == BT_MESH_ADDR_UNASSIGNED ||
	    sub_period == 0x00) {
		/* Setting the same addresses with zero period should retain
		 * the addresses according to MESH/NODE/CFG/HBS/BV-02-C.
		 */
		if (cfg->hb_sub.src != sub_src || cfg->hb_sub.dst != sub_dst) {
			cfg->hb_sub.src = BT_MESH_ADDR_UNASSIGNED;
			cfg->hb_sub.dst = BT_MESH_ADDR_UNASSIGNED;
		}

		period_ms = 0;
	} else {
		cfg->hb_sub.src = sub_src;
		cfg->hb_sub.dst = sub_dst;
		cfg->hb_sub.min_hops = 0x7f;
		cfg->hb_sub.max_hops = 0;
		cfg->hb_sub.count = 0;
		period_ms = hb_pwr2(sub_period, 1) * 1000;
	}

	BT_DBG("period_ms %u", period_ms);

	if (period_ms) {
		cfg->hb_sub.expiry = k_uptime_get() + period_ms;
	} else {
		cfg->hb_sub.expiry = 0;
	}

	hb_sub_send_status(model, ctx, STATUS_SUCCESS);
}

const struct bt_mesh_model_op bt_mesh_cfg_op[] = {
	{ OP_DEV_COMP_DATA_GET,        1,   dev_comp_data_get },
	{ OP_APP_KEY_ADD,              19,  app_key_add },
	{ OP_APP_KEY_UPDATE,           19,  app_key_update },
	{ OP_APP_KEY_DEL,              3,   app_key_del },
	{ OP_APP_KEY_GET,              2,   app_key_get },
	{ OP_BEACON_GET,               0,   beacon_get },
	{ OP_BEACON_SET,               1,   beacon_set },
	{ OP_DEFAULT_TTL_GET,          0,   default_ttl_get },
	{ OP_DEFAULT_TTL_SET,          1,   default_ttl_set },
	{ OP_GATT_PROXY_GET,           0,   gatt_proxy_get },
	{ OP_GATT_PROXY_SET,           1,   gatt_proxy_set },
	{ OP_NET_TRANSMIT_GET,         0,   net_transmit_get },
	{ OP_NET_TRANSMIT_SET,         1,   net_transmit_set },
	{ OP_RELAY_GET,                0,   relay_get },
	{ OP_RELAY_SET,                2,   relay_set },
	{ OP_MOD_PUB_GET,              4,   mod_pub_get },
	{ OP_MOD_PUB_SET,              11,  mod_pub_set },
	{ OP_MOD_PUB_VA_SET,           24,  mod_pub_va_set },
	{ OP_MOD_SUB_ADD,              6,   mod_sub_add },
	{ OP_MOD_SUB_VA_ADD,           20,  mod_sub_va_add },
	{ OP_MOD_SUB_DEL,              6,   mod_sub_del },
	{ OP_MOD_SUB_VA_DEL,           20,  mod_sub_va_del },
	{ OP_MOD_SUB_OVERWRITE,        6,   mod_sub_overwrite },
	{ OP_MOD_SUB_VA_OVERWRITE,     20,  mod_sub_va_overwrite },
	{ OP_MOD_SUB_DEL_ALL,          4,   mod_sub_del_all },
	{ OP_MOD_SUB_GET,              4,   mod_sub_get },
	{ OP_MOD_SUB_GET_VND,          6,   mod_sub_get_vnd },
	{ OP_NET_KEY_ADD,              18,  net_key_add },
	{ OP_NET_KEY_UPDATE,           18,  net_key_update },
	{ OP_NET_KEY_DEL,              2,   net_key_del },
	{ OP_NET_KEY_GET,              0,   net_key_get },
	{ OP_NODE_IDENTITY_GET,        2,   node_identity_get },
	{ OP_NODE_IDENTITY_SET,        3,   node_identity_set },
	{ OP_MOD_APP_BIND,             6,   mod_app_bind },
	{ OP_MOD_APP_UNBIND,           6,   mod_app_unbind },
	{ OP_SIG_MOD_APP_GET,          4,   mod_app_get },
	{ OP_VND_MOD_APP_GET,          6,   mod_app_get },
	{ OP_NODE_RESET,               0,   node_reset },
	{ OP_FRIEND_GET,               0,   friend_get },
	{ OP_FRIEND_SET,               1,   friend_set },
	{ OP_LPN_TIMEOUT_GET,          2,   lpn_timeout_get },
	{ OP_KRP_GET,                  2,   krp_get },
	{ OP_KRP_SET,                  3,   krp_set },
	{ OP_HEARTBEAT_PUB_GET,        0,   heartbeat_pub_get },
	{ OP_HEARTBEAT_PUB_SET,        9,   heartbeat_pub_set },
	{ OP_HEARTBEAT_SUB_GET,        0,   heartbeat_sub_get },
	{ OP_HEARTBEAT_SUB_SET,        5,   heartbeat_sub_set },
	BT_MESH_MODEL_OP_END,
};

static void hb_publish(struct k_work *work)
{
	struct bt_mesh_cfg *cfg = CONTAINER_OF(work, struct bt_mesh_cfg,
					       hb_pub.timer.work);
	struct bt_mesh_model *model = cfg->model;
	struct bt_mesh_subnet *sub;
	u16_t period_ms;

	BT_DBG("hb_pub.count: %u", cfg->hb_pub.count);

	sub = bt_mesh_subnet_get(cfg->hb_pub.net_idx);
	if (!sub) {
		BT_ERR("No matching subnet for idx 0x%02x",
		       cfg->hb_pub.net_idx);
		cfg->hb_pub.dst = BT_MESH_ADDR_UNASSIGNED;
		return;
	}

	hb_send(model);

	if (cfg->hb_pub.count == 0) {
		return;
	}

	if (cfg->hb_pub.count != 0xffff) {
		cfg->hb_pub.count--;
	}

	period_ms = hb_pwr2(cfg->hb_pub.period, 1) * 1000;
	if (period_ms) {
		k_delayed_work_submit(&cfg->hb_pub.timer, period_ms);
	}
}

static bool conf_is_valid(struct bt_mesh_cfg *cfg)
{
	if (cfg->relay > 0x02) {
		return false;
	}

	if (cfg->beacon > 0x01) {
		return false;
	}

	if (cfg->default_ttl > 0x7f) {
		return false;
	}

	return true;
}

int bt_mesh_conf_init(struct bt_mesh_model *model, bool primary)
{
	struct bt_mesh_cfg *cfg = model->user_data;

	if (!cfg) {
		BT_ERR("No Configuration Server context provided");
		return -EINVAL;
	}

	if (!conf_is_valid(cfg)) {
		BT_ERR("Invalid values in configuration");
		return -EINVAL;
	}

	/* Configuration Model security is device-key based */
	model->keys[0] = BT_MESH_KEY_DEV;

	if (!IS_ENABLED(CONFIG_BT_MESH_RELAY)) {
		cfg->relay = BT_MESH_RELAY_NOT_SUPPORTED;
	}

	if (!IS_ENABLED(CONFIG_BT_MESH_FRIEND)) {
		cfg->frnd = BT_MESH_FRIEND_NOT_SUPPORTED;
	}

	if (!IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY)) {
		cfg->gatt_proxy = BT_MESH_GATT_PROXY_NOT_SUPPORTED;
	}

	k_delayed_work_init(&cfg->hb_pub.timer, hb_publish);
	cfg->hb_sub.expiry = 0;

	cfg->model = model;

	conf = cfg;

	return 0;
}

void bt_mesh_heartbeat(u16_t src, u16_t dst, u8_t hops, u16_t feat)
{
	struct bt_mesh_cfg *cfg = conf;

	if (!cfg) {
		BT_WARN("No configuaration server context available");
		return;
	}

	if (src != cfg->hb_sub.src || dst != cfg->hb_sub.dst) {
		BT_WARN("No subscription for received heartbeat");
		return;
	}

	if (k_uptime_get() > cfg->hb_sub.expiry) {
		BT_WARN("Heartbeat subscription period expired");
		return;
	}

	cfg->hb_sub.min_hops = min(cfg->hb_sub.min_hops, hops);
	cfg->hb_sub.max_hops = max(cfg->hb_sub.max_hops, hops);

	if (cfg->hb_sub.count < 0xffff) {
		cfg->hb_sub.count++;
	}

	BT_DBG("src 0x%04x dst 0x%04x hops %u min %u max %u count %u", src,
	       dst, hops, cfg->hb_sub.min_hops, cfg->hb_sub.max_hops,
	       cfg->hb_sub.count);

	if (cfg->hb_sub.func) {
		cfg->hb_sub.func(hops, feat);
	}
}

u8_t bt_mesh_net_transmit_get(void)
{
	if (conf) {
		return conf->net_transmit;
	}

	return 0;
}

u8_t bt_mesh_relay_get(void)
{
	if (conf) {
		return conf->relay;
	}

	return BT_MESH_RELAY_NOT_SUPPORTED;
}

u8_t bt_mesh_friend_get(void)
{
	BT_DBG("conf %p conf->frnd 0x%02x", conf, conf->frnd);

	if (conf) {
		return conf->frnd;
	}

	return BT_MESH_FRIEND_NOT_SUPPORTED;
}

u8_t bt_mesh_relay_retransmit_get(void)
{
	if (conf) {
		return conf->relay_retransmit;
	}

	return 0;
}

u8_t bt_mesh_beacon_get(void)
{
	if (conf) {
		return conf->beacon;
	}

	return BT_MESH_BEACON_DISABLED;
}

u8_t bt_mesh_gatt_proxy_get(void)
{
	if (conf) {
		return conf->gatt_proxy;
	}

	return BT_MESH_GATT_PROXY_NOT_SUPPORTED;
}

u8_t bt_mesh_default_ttl_get(void)
{
	if (conf) {
		return conf->default_ttl;
	}

	return DEFAULT_TTL;
}

u8_t *bt_mesh_label_uuid_get(u16_t addr)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(labels); i++) {
		if (labels[i].addr == addr) {
			return labels[i].uuid;
		}
	}

	return NULL;
}
