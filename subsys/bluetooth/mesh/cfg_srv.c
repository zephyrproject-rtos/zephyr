/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_MODEL)
#define LOG_MODULE_NAME bt_mesh_cfg_srv
#include "common/log.h"

#include "host/testing.h"

#include "mesh.h"
#include "adv.h"
#include "net.h"
#include "rpl.h"
#include "lpn.h"
#include "transport.h"
#include "heartbeat.h"
#include "crypto.h"
#include "access.h"
#include "beacon.h"
#include "proxy.h"
#include "foundation.h"
#include "friend.h"
#include "settings.h"
#include "cfg.h"

static void node_reset_pending_handler(struct k_work *work)
{
	bt_mesh_reset();
}

static K_WORK_DEFINE(node_reset_pending, node_reset_pending_handler);

static int comp_add_elem(struct net_buf_simple *buf, struct bt_mesh_elem *elem,
			 bool primary)
{
	struct bt_mesh_model *mod;
	int i;

	if (net_buf_simple_tailroom(buf) <
	    4 + (elem->model_count * 2U) + (elem->vnd_model_count * 4U)) {
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
	uint16_t feat = 0U;
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

static int dev_comp_data_get(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	NET_BUF_SIMPLE_DEFINE(sdu, BT_MESH_TX_SDU_MAX);
	uint8_t page;
	int err;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	page = net_buf_simple_pull_u8(buf);
	if (page != 0U) {
		BT_DBG("Composition page %u not available", page);
		page = 0U;
	}

	bt_mesh_model_msg_init(&sdu, OP_DEV_COMP_DATA_STATUS);

	net_buf_simple_add_u8(&sdu, page);
	err = comp_get_page_0(&sdu);
	if (err) {
		BT_ERR("Unable to get composition page 0");
		return err;
	}

	if (bt_mesh_model_send(model, ctx, &sdu, NULL, NULL)) {
		BT_ERR("Unable to send Device Composition Status response");
	}

	return err;
}

static struct bt_mesh_model *get_model(struct bt_mesh_elem *elem,
				       struct net_buf_simple *buf, bool *vnd)
{
	if (buf->len < 4) {
		uint16_t id;

		id = net_buf_simple_pull_le16(buf);

		BT_DBG("ID 0x%04x addr 0x%04x", id, elem->addr);

		*vnd = false;

		return bt_mesh_model_find(elem, id);
	} else {
		uint16_t company, id;

		company = net_buf_simple_pull_le16(buf);
		id = net_buf_simple_pull_le16(buf);

		BT_DBG("Company 0x%04x ID 0x%04x addr 0x%04x", company, id,
		       elem->addr);

		*vnd = true;

		return bt_mesh_model_find_vnd(elem, company, id);
	}
}

static uint8_t _mod_pub_set(struct bt_mesh_model *model, uint16_t pub_addr,
			 uint16_t app_idx, uint8_t cred_flag, uint8_t ttl, uint8_t period,
			 uint8_t retransmit, bool store)
{
	if (!model->pub) {
		return STATUS_NVAL_PUB_PARAM;
	}

	if (!IS_ENABLED(CONFIG_BT_MESH_LOW_POWER) && cred_flag) {
		return STATUS_FEAT_NOT_SUPP;
	}

	if (!model->pub->update && period) {
		return STATUS_NVAL_PUB_PARAM;
	}

	if (pub_addr == BT_MESH_ADDR_UNASSIGNED) {
		if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
			return STATUS_SUCCESS;
		}

		model->pub->addr = BT_MESH_ADDR_UNASSIGNED;
		model->pub->key = 0U;
		model->pub->cred = 0U;
		model->pub->ttl = 0U;
		model->pub->period = 0U;
		model->pub->retransmit = 0U;
		model->pub->count = 0U;

		if (model->pub->update) {
			/* If this fails, the timer will check pub->addr and
			 * exit without transmitting.
			 */
			(void)k_work_cancel_delayable(&model->pub->timer);
		}

		if (IS_ENABLED(CONFIG_BT_SETTINGS) && store) {
			bt_mesh_model_pub_store(model);
		}

		return STATUS_SUCCESS;
	}

	if (!bt_mesh_app_key_exists(app_idx) || !bt_mesh_model_has_key(model, app_idx)) {
		return STATUS_INVALID_APPKEY;
	}

#if CONFIG_BT_MESH_LABEL_COUNT > 0
	if (BT_MESH_ADDR_IS_VIRTUAL(model->pub->addr)) {
		uint8_t *uuid = bt_mesh_va_label_get(model->pub->addr);

		if (uuid) {
			bt_mesh_va_del(uuid, NULL);
		}
	}
#endif

	model->pub->addr = pub_addr;
	model->pub->key = app_idx;
	model->pub->cred = cred_flag;
	model->pub->ttl = ttl;
	model->pub->period = period;
	model->pub->retransmit = retransmit;

	if (model->pub->update) {
		int32_t period_ms;

		period_ms = bt_mesh_model_pub_period_get(model);
		BT_DBG("period %u ms", period_ms);

		if (period_ms > 0) {
			k_work_reschedule(&model->pub->timer,
					  K_MSEC(period_ms));
		} else {
			/* If this fails, publication will stop after the
			 * ongoing set of retransmits.
			 */
			(void)k_work_cancel_delayable(&model->pub->timer);
		}
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS) && store) {
		bt_mesh_model_pub_store(model);
	}

	return STATUS_SUCCESS;
}

static uint8_t mod_bind(struct bt_mesh_model *model, uint16_t key_idx)
{
	int i;

	BT_DBG("model %p key_idx 0x%03x", model, key_idx);

	if (!bt_mesh_app_key_exists(key_idx)) {
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

			if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
				bt_mesh_model_bind_store(model);
			}

			return STATUS_SUCCESS;
		}
	}

	return STATUS_INSUFF_RESOURCES;
}

static uint8_t mod_unbind(struct bt_mesh_model *model, uint16_t key_idx, bool store)
{
	int i;

	BT_DBG("model %p key_idx 0x%03x store %u", model, key_idx, store);

	if (!bt_mesh_app_key_exists(key_idx)) {
		return STATUS_INVALID_APPKEY;
	}

	for (i = 0; i < ARRAY_SIZE(model->keys); i++) {
		if (model->keys[i] != key_idx) {
			continue;
		}

		model->keys[i] = BT_MESH_KEY_UNUSED;

		if (IS_ENABLED(CONFIG_BT_SETTINGS) && store) {
			bt_mesh_model_bind_store(model);
		}

		if (model->pub && model->pub->key == key_idx) {
			_mod_pub_set(model, BT_MESH_ADDR_UNASSIGNED,
				     0, 0, 0, 0, 0, store);
		}
	}

	return STATUS_SUCCESS;
}

static int send_app_key_status(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       uint8_t status,
			       uint16_t app_idx, uint16_t net_idx)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_APP_KEY_STATUS, 4);

	bt_mesh_model_msg_init(&msg, OP_APP_KEY_STATUS);
	net_buf_simple_add_u8(&msg, status);
	key_idx_pack(&msg, net_idx, app_idx);

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		BT_ERR("Unable to send App Key Status response");
	}

	return 0;
}

static int app_key_add(struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	uint16_t key_net_idx, key_app_idx;
	uint8_t status;

	key_idx_unpack(buf, &key_net_idx, &key_app_idx);

	BT_DBG("AppIdx 0x%04x NetIdx 0x%04x", key_app_idx, key_net_idx);

	status = bt_mesh_app_key_add(key_app_idx, key_net_idx, buf->data);

	return send_app_key_status(model, ctx, status, key_app_idx, key_net_idx);
}

static int app_key_update(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	uint16_t key_net_idx, key_app_idx;
	uint8_t status;

	key_idx_unpack(buf, &key_net_idx, &key_app_idx);

	BT_DBG("AppIdx 0x%04x NetIdx 0x%04x", key_app_idx, key_net_idx);

	status = bt_mesh_app_key_update(key_app_idx, key_net_idx, buf->data);
	BT_DBG("status 0x%02x", status);

	return send_app_key_status(model, ctx, status, key_app_idx, key_net_idx);
}

static void mod_app_key_del(struct bt_mesh_model *mod,
			    struct bt_mesh_elem *elem, bool vnd, bool primary,
			    void *user_data)
{
	uint16_t *app_idx = user_data;

	mod_unbind(mod, *app_idx, true);
}

static void app_key_evt(uint16_t app_idx, uint16_t net_idx,
			enum bt_mesh_key_evt evt)
{
	if (evt == BT_MESH_KEY_DELETED) {
		bt_mesh_model_foreach(&mod_app_key_del, &app_idx);
	}
}

BT_MESH_APP_KEY_CB_DEFINE(app_key_evt);

static int app_key_del(struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	uint16_t key_net_idx, key_app_idx;
	uint8_t status;

	key_idx_unpack(buf, &key_net_idx, &key_app_idx);

	BT_DBG("AppIdx 0x%04x NetIdx 0x%04x", key_app_idx, key_net_idx);

	status = bt_mesh_app_key_del(key_app_idx, key_net_idx);

	return send_app_key_status(model, ctx, status, key_app_idx, key_net_idx);
}

/* Index list length: 3 bytes for every pair and 2 bytes for an odd idx */
#define IDX_LEN(num) (((num) / 2) * 3 + ((num) % 2) * 2)

static int app_key_get(struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_APP_KEY_LIST,
				 3 + IDX_LEN(CONFIG_BT_MESH_APP_KEY_COUNT));
	uint16_t app_idx[CONFIG_BT_MESH_APP_KEY_COUNT];
	uint16_t get_idx;
	uint8_t status;
	ssize_t count;
	int i;

	get_idx = net_buf_simple_pull_le16(buf);
	if (get_idx > 0xfff) {
		BT_ERR("Invalid NetKeyIndex 0x%04x", get_idx);
		return -EINVAL;
	}

	BT_DBG("idx 0x%04x", get_idx);

	bt_mesh_model_msg_init(&msg, OP_APP_KEY_LIST);

	if (!bt_mesh_subnet_exists(get_idx)) {
		status = STATUS_INVALID_NETKEY;
	} else {
		status = STATUS_SUCCESS;
	}

	net_buf_simple_add_u8(&msg, status);
	net_buf_simple_add_le16(&msg, get_idx);

	if (status != STATUS_SUCCESS) {
		goto send_status;
	}

	count = bt_mesh_app_keys_get(get_idx, app_idx, ARRAY_SIZE(app_idx), 0);
	if (count < 0 || count > ARRAY_SIZE(app_idx)) {
		count = ARRAY_SIZE(app_idx);
	}

	for (i = 0; i < count - 1; i += 2) {
		key_idx_pack(&msg, app_idx[i], app_idx[i + 1]);
	}

	if (i < count) {
		net_buf_simple_add_le16(&msg, app_idx[i]);
	}

send_status:
	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		BT_ERR("Unable to send AppKey List");
	}

	return 0;
}

static int beacon_get(struct bt_mesh_model *model,
		      struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_BEACON_STATUS, 1);

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	bt_mesh_model_msg_init(&msg, OP_BEACON_STATUS);
	net_buf_simple_add_u8(&msg, bt_mesh_beacon_enabled());

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		BT_ERR("Unable to send Config Beacon Status response");
	}

	return 0;
}

static int beacon_set(struct bt_mesh_model *model,
		      struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_BEACON_STATUS, 1);

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	if (buf->data[0] != 0x00 && buf->data[0] != 0x01) {
		BT_WARN("Invalid Config Beacon value 0x%02x", buf->data[0]);
		return -EINVAL;
	}

	bt_mesh_beacon_set(buf->data[0]);

	bt_mesh_model_msg_init(&msg, OP_BEACON_STATUS);
	net_buf_simple_add_u8(&msg, buf->data[0]);

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		BT_ERR("Unable to send Config Beacon Status response");
	}

	return 0;
}

static int default_ttl_get(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_DEFAULT_TTL_STATUS, 1);

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	bt_mesh_model_msg_init(&msg, OP_DEFAULT_TTL_STATUS);
	net_buf_simple_add_u8(&msg, bt_mesh_default_ttl_get());

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		BT_ERR("Unable to send Default TTL Status response");
	}

	return 0;
}

static int default_ttl_set(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_DEFAULT_TTL_STATUS, 1);
	int err;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	err = bt_mesh_default_ttl_set(buf->data[0]);
	if (err) {
		BT_WARN("Prohibited Default TTL value 0x%02x", buf->data[0]);
		return err;
	}

	bt_mesh_model_msg_init(&msg, OP_DEFAULT_TTL_STATUS);
	net_buf_simple_add_u8(&msg, buf->data[0]);

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		BT_ERR("Unable to send Default TTL Status response");
	}

	return 0;
}

static int send_gatt_proxy_status(struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_GATT_PROXY_STATUS, 1);

	bt_mesh_model_msg_init(&msg, OP_GATT_PROXY_STATUS);
	net_buf_simple_add_u8(&msg, bt_mesh_gatt_proxy_get());

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		BT_ERR("Unable to send GATT Proxy Status");
	}

	return 0;
}

static int gatt_proxy_get(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	return send_gatt_proxy_status(model, ctx);
}

static int gatt_proxy_set(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	if (buf->data[0] != 0x00 && buf->data[0] != 0x01) {
		BT_WARN("Invalid GATT Proxy value 0x%02x", buf->data[0]);
		return -EINVAL;
	}

	(void)bt_mesh_gatt_proxy_set(buf->data[0]);

	return send_gatt_proxy_status(model, ctx);
}

static int net_transmit_get(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_NET_TRANSMIT_STATUS, 1);

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	bt_mesh_model_msg_init(&msg, OP_NET_TRANSMIT_STATUS);
	net_buf_simple_add_u8(&msg, bt_mesh_net_transmit_get());

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		BT_ERR("Unable to send Config Network Transmit Status");
	}

	return 0;
}

static int net_transmit_set(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_NET_TRANSMIT_STATUS, 1);

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	BT_DBG("Transmit 0x%02x (count %u interval %ums)", buf->data[0],
	       BT_MESH_TRANSMIT_COUNT(buf->data[0]),
	       BT_MESH_TRANSMIT_INT(buf->data[0]));

	bt_mesh_net_transmit_set(buf->data[0]);

	bt_mesh_model_msg_init(&msg, OP_NET_TRANSMIT_STATUS);
	net_buf_simple_add_u8(&msg, buf->data[0]);

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		BT_ERR("Unable to send Network Transmit Status");
	}

	return 0;
}

static int relay_get(struct bt_mesh_model *model,
		     struct bt_mesh_msg_ctx *ctx,
		     struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_RELAY_STATUS, 2);

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	bt_mesh_model_msg_init(&msg, OP_RELAY_STATUS);
	net_buf_simple_add_u8(&msg, bt_mesh_relay_get());
	net_buf_simple_add_u8(&msg, bt_mesh_relay_retransmit_get());

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		BT_ERR("Unable to send Config Relay Status response");
	}

	return 0;
}

static int relay_set(struct bt_mesh_model *model,
		     struct bt_mesh_msg_ctx *ctx,
		     struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_RELAY_STATUS, 2);

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	if (buf->data[0] != 0x00 && buf->data[0] != 0x01) {
		BT_WARN("Invalid Relay value 0x%02x", buf->data[0]);
		return -EINVAL;
	}

	(void)bt_mesh_relay_set(buf->data[0], buf->data[1]);

	bt_mesh_model_msg_init(&msg, OP_RELAY_STATUS);
	net_buf_simple_add_u8(&msg, bt_mesh_relay_get());
	net_buf_simple_add_u8(&msg, bt_mesh_relay_retransmit_get());

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		BT_ERR("Unable to send Relay Status response");
	}

	return 0;
}

static int send_mod_pub_status(struct bt_mesh_model *cfg_mod,
			       struct bt_mesh_msg_ctx *ctx, uint16_t elem_addr,
			       uint16_t pub_addr, bool vnd,
			       struct bt_mesh_model *mod, uint8_t status,
			       uint8_t *mod_id)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_MOD_PUB_STATUS, 14);

	bt_mesh_model_msg_init(&msg, OP_MOD_PUB_STATUS);

	net_buf_simple_add_u8(&msg, status);
	net_buf_simple_add_le16(&msg, elem_addr);

	if (status != STATUS_SUCCESS) {
		(void)memset(net_buf_simple_add(&msg, 7), 0, 7);
	} else {
		uint16_t idx_cred;

		net_buf_simple_add_le16(&msg, pub_addr);

		idx_cred = mod->pub->key | (uint16_t)mod->pub->cred << 12;
		net_buf_simple_add_le16(&msg, idx_cred);
		net_buf_simple_add_u8(&msg, mod->pub->ttl);
		net_buf_simple_add_u8(&msg, mod->pub->period);
		net_buf_simple_add_u8(&msg, mod->pub->retransmit);
	}

	if (vnd) {
		memcpy(net_buf_simple_add(&msg, 4), mod_id, 4);
	} else {
		memcpy(net_buf_simple_add(&msg, 2), mod_id, 2);
	}

	if (bt_mesh_model_send(cfg_mod, ctx, &msg, NULL, NULL)) {
		BT_ERR("Unable to send Model Publication Status");
	}

	return 0;
}

static int mod_pub_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	uint16_t elem_addr, pub_addr = 0U;
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	uint8_t *mod_id, status;
	bool vnd;

	if ((buf->len != 4U) && (buf->len != 6U)) {
		BT_ERR("The message size for the application opcode is incorrect.");
		return -EMSGSIZE;
	}

	elem_addr = net_buf_simple_pull_le16(buf);
	if (!BT_MESH_ADDR_IS_UNICAST(elem_addr)) {
		BT_WARN("Prohibited element address");
		return -EINVAL;
	}

	mod_id = buf->data;

	BT_DBG("elem_addr 0x%04x", elem_addr);

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4U);
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
	return send_mod_pub_status(model, ctx, elem_addr, pub_addr, vnd, mod,
				   status, mod_id);
}

static int mod_pub_set(struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	uint8_t retransmit, status, pub_ttl, pub_period, cred_flag;
	uint16_t elem_addr, pub_addr, pub_app_idx;
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	uint8_t *mod_id;
	bool vnd;

	if ((buf->len != 11U) && (buf->len != 13U)) {
		BT_ERR("The message size for the application opcode is incorrect.");
		return -EMSGSIZE;
	}

	elem_addr = net_buf_simple_pull_le16(buf);
	if (!BT_MESH_ADDR_IS_UNICAST(elem_addr)) {
		BT_WARN("Prohibited element address");
		return -EINVAL;
	}

	pub_addr = net_buf_simple_pull_le16(buf);
	pub_app_idx = net_buf_simple_pull_le16(buf);
	cred_flag = ((pub_app_idx >> 12) & BIT_MASK(1));
	pub_app_idx &= BIT_MASK(12);

	pub_ttl = net_buf_simple_pull_u8(buf);
	if (pub_ttl > BT_MESH_TTL_MAX && pub_ttl != BT_MESH_TTL_DEFAULT) {
		BT_ERR("Invalid TTL value 0x%02x", pub_ttl);
		return -EINVAL;
	}

	pub_period = net_buf_simple_pull_u8(buf);
	retransmit = net_buf_simple_pull_u8(buf);
	mod_id = buf->data;

	BT_DBG("elem_addr 0x%04x pub_addr 0x%04x cred_flag %u",
	       elem_addr, pub_addr, cred_flag);
	BT_DBG("pub_app_idx 0x%03x, pub_ttl %u pub_period 0x%02x",
	       pub_app_idx, pub_ttl, pub_period);
	BT_DBG("retransmit 0x%02x (count %u interval %ums)", retransmit,
	       BT_MESH_PUB_TRANSMIT_COUNT(retransmit),
	       BT_MESH_PUB_TRANSMIT_INT(retransmit));

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4U);
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	mod = get_model(elem, buf, &vnd);
	if (!mod) {
		status = STATUS_INVALID_MODEL;
		goto send_status;
	}

	status = _mod_pub_set(mod, pub_addr, pub_app_idx, cred_flag, pub_ttl,
			      pub_period, retransmit, true);

send_status:
	return send_mod_pub_status(model, ctx, elem_addr, pub_addr, vnd, mod,
				   status, mod_id);
}

static size_t mod_sub_list_clear(struct bt_mesh_model *mod)
{
	uint8_t *label_uuid;
	size_t clear_count;
	int i;

	/* Unref stored labels related to this model */
	for (i = 0, clear_count = 0; i < ARRAY_SIZE(mod->groups); i++) {
		if (!BT_MESH_ADDR_IS_VIRTUAL(mod->groups[i])) {
			if (mod->groups[i] != BT_MESH_ADDR_UNASSIGNED) {
				mod->groups[i] = BT_MESH_ADDR_UNASSIGNED;
				clear_count++;
			}

			continue;
		}

		label_uuid = bt_mesh_va_label_get(mod->groups[i]);

		mod->groups[i] = BT_MESH_ADDR_UNASSIGNED;
		clear_count++;

		if (label_uuid) {
			bt_mesh_va_del(label_uuid, NULL);
		} else {
			BT_ERR("Label UUID not found");
		}
	}

	return clear_count;
}

static int mod_pub_va_set(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	uint8_t retransmit, status, pub_ttl, pub_period, cred_flag;
	uint16_t elem_addr, pub_addr, pub_app_idx;
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	uint8_t *label_uuid;
	uint8_t *mod_id;
	bool vnd;

	if ((buf->len != 25U) && (buf->len != 27U)) {
		BT_ERR("The message size for the application opcode is incorrect.");
		return -EMSGSIZE;
	}

	elem_addr = net_buf_simple_pull_le16(buf);
	if (!BT_MESH_ADDR_IS_UNICAST(elem_addr)) {
		BT_WARN("Prohibited element address");
		return -EINVAL;
	}

	label_uuid = net_buf_simple_pull_mem(buf, 16);
	pub_app_idx = net_buf_simple_pull_le16(buf);
	cred_flag = ((pub_app_idx >> 12) & BIT_MASK(1));
	pub_app_idx &= BIT_MASK(12);
	pub_ttl = net_buf_simple_pull_u8(buf);
	if (pub_ttl > BT_MESH_TTL_MAX && pub_ttl != BT_MESH_TTL_DEFAULT) {
		BT_ERR("Invalid TTL value 0x%02x", pub_ttl);
		return -EINVAL;
	}

	pub_period = net_buf_simple_pull_u8(buf);
	retransmit = net_buf_simple_pull_u8(buf);
	mod_id = buf->data;

	BT_DBG("elem_addr 0x%04x cred_flag %u", elem_addr, cred_flag);
	BT_DBG("pub_app_idx 0x%03x, pub_ttl %u pub_period 0x%02x",
	       pub_app_idx, pub_ttl, pub_period);
	BT_DBG("retransmit 0x%02x (count %u interval %ums)", retransmit,
	       BT_MESH_PUB_TRANSMIT_COUNT(retransmit),
	       BT_MESH_PUB_TRANSMIT_INT(retransmit));

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4U);
		pub_addr = 0U;
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	mod = get_model(elem, buf, &vnd);
	if (!mod) {
		pub_addr = 0U;
		status = STATUS_INVALID_MODEL;
		goto send_status;
	}

	status = bt_mesh_va_add(label_uuid, &pub_addr);
	if (status != STATUS_SUCCESS) {
		goto send_status;
	}

	status = _mod_pub_set(mod, pub_addr, pub_app_idx, cred_flag, pub_ttl,
			      pub_period, retransmit, true);
	if (status != STATUS_SUCCESS) {
		bt_mesh_va_del(label_uuid, NULL);
	}

send_status:
	return send_mod_pub_status(model, ctx, elem_addr, pub_addr, vnd, mod,
				   status, mod_id);
}

static int send_mod_sub_status(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx, uint8_t status,
			       uint16_t elem_addr, uint16_t sub_addr, uint8_t *mod_id,
			       bool vnd)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_MOD_SUB_STATUS, 9);

	BT_DBG("status 0x%02x elem_addr 0x%04x sub_addr 0x%04x", status,
	       elem_addr, sub_addr);

	bt_mesh_model_msg_init(&msg, OP_MOD_SUB_STATUS);

	net_buf_simple_add_u8(&msg, status);
	net_buf_simple_add_le16(&msg, elem_addr);
	net_buf_simple_add_le16(&msg, sub_addr);

	if (vnd) {
		memcpy(net_buf_simple_add(&msg, 4), mod_id, 4);
	} else {
		memcpy(net_buf_simple_add(&msg, 2), mod_id, 2);
	}

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		BT_ERR("Unable to send Model Subscription Status");
	}

	return 0;
}

static int mod_sub_add(struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	uint16_t elem_addr, sub_addr;
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	uint8_t *mod_id;
	uint8_t status;
	uint16_t *entry;
	bool vnd;

	if ((buf->len != 6U) && (buf->len != 8U)) {
		BT_ERR("The message size for the application opcode is incorrect.");
		return -EMSGSIZE;
	}

	elem_addr = net_buf_simple_pull_le16(buf);
	if (!BT_MESH_ADDR_IS_UNICAST(elem_addr)) {
		BT_WARN("Prohibited element address");
		return -EINVAL;
	}

	sub_addr = net_buf_simple_pull_le16(buf);

	BT_DBG("elem_addr 0x%04x, sub_addr 0x%04x", elem_addr, sub_addr);

	mod_id = buf->data;

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4U);
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	mod = get_model(elem, buf, &vnd);
	if (!mod) {
		status = STATUS_INVALID_MODEL;
		goto send_status;
	}

	if (!BT_MESH_ADDR_IS_GROUP(sub_addr) && !BT_MESH_ADDR_IS_FIXED_GROUP(sub_addr)) {
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	if (bt_mesh_model_find_group(&mod, sub_addr)) {
		/* Tried to add existing subscription */
		BT_DBG("found existing subscription");
		status = STATUS_SUCCESS;
		goto send_status;
	}

	entry = bt_mesh_model_find_group(&mod, BT_MESH_ADDR_UNASSIGNED);
	if (!entry) {
		status = STATUS_INSUFF_RESOURCES;
		goto send_status;
	}

	*entry = sub_addr;
	status = STATUS_SUCCESS;

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_model_sub_store(mod);
	}

	if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)) {
		bt_mesh_lpn_group_add(sub_addr);
	}


send_status:
	return send_mod_sub_status(model, ctx, status, elem_addr, sub_addr,
				   mod_id, vnd);
}

static int mod_sub_del(struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	uint16_t elem_addr, sub_addr;
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	uint8_t *mod_id;
	uint16_t *match;
	uint8_t status;
	bool vnd;

	if ((buf->len != 6U) && (buf->len != 8U)) {
		BT_ERR("The message size for the application opcode is incorrect.");
		return -EMSGSIZE;
	}

	elem_addr = net_buf_simple_pull_le16(buf);
	if (!BT_MESH_ADDR_IS_UNICAST(elem_addr)) {
		BT_WARN("Prohibited element address");
		return -EINVAL;
	}

	sub_addr = net_buf_simple_pull_le16(buf);

	BT_DBG("elem_addr 0x%04x sub_addr 0x%04x", elem_addr, sub_addr);

	mod_id = buf->data;

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4U);
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	mod = get_model(elem, buf, &vnd);
	if (!mod) {
		status = STATUS_INVALID_MODEL;
		goto send_status;
	}

	if (!BT_MESH_ADDR_IS_GROUP(sub_addr) && !BT_MESH_ADDR_IS_FIXED_GROUP(sub_addr)) {
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

	match = bt_mesh_model_find_group(&mod, sub_addr);
	if (match) {
		*match = BT_MESH_ADDR_UNASSIGNED;

		if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
			bt_mesh_model_sub_store(mod);
		}
	}

send_status:
	return send_mod_sub_status(model, ctx, status, elem_addr, sub_addr,
				   mod_id, vnd);
}

static enum bt_mesh_walk mod_sub_clear_visitor(struct bt_mesh_model *mod, void *user_data)
{
	if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)) {
		bt_mesh_lpn_group_del(mod->groups, ARRAY_SIZE(mod->groups));
	}

	mod_sub_list_clear(mod);

	return BT_MESH_WALK_CONTINUE;
}

static int mod_sub_overwrite(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	uint16_t elem_addr, sub_addr;
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	uint8_t *mod_id;
	uint8_t status;
	bool vnd;

	if ((buf->len != 6U) && (buf->len != 8U)) {
		BT_ERR("The message size for the application opcode is incorrect.");
		return -EMSGSIZE;
	}

	elem_addr = net_buf_simple_pull_le16(buf);
	if (!BT_MESH_ADDR_IS_UNICAST(elem_addr)) {
		BT_WARN("Prohibited element address");
		return -EINVAL;
	}

	sub_addr = net_buf_simple_pull_le16(buf);

	BT_DBG("elem_addr 0x%04x sub_addr 0x%04x", elem_addr, sub_addr);

	mod_id = buf->data;

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4U);
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	mod = get_model(elem, buf, &vnd);
	if (!mod) {
		status = STATUS_INVALID_MODEL;
		goto send_status;
	}

	if (!BT_MESH_ADDR_IS_GROUP(sub_addr) && !BT_MESH_ADDR_IS_FIXED_GROUP(sub_addr)) {
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}


	if (ARRAY_SIZE(mod->groups) > 0) {
		bt_mesh_model_extensions_walk(mod, mod_sub_clear_visitor, NULL);

		mod->groups[0] = sub_addr;
		status = STATUS_SUCCESS;

		if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
			bt_mesh_model_sub_store(mod);
		}

		if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)) {
			bt_mesh_lpn_group_add(sub_addr);
		}
	} else {
		status = STATUS_INSUFF_RESOURCES;
	}


send_status:
	return send_mod_sub_status(model, ctx, status, elem_addr, sub_addr,
				   mod_id, vnd);
}

static int mod_sub_del_all(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	uint16_t elem_addr;
	uint8_t *mod_id;
	uint8_t status;
	bool vnd;

	if ((buf->len != 4U) && (buf->len != 6U)) {
		BT_ERR("The message size for the application opcode is incorrect.");
		return -EMSGSIZE;
	}

	elem_addr = net_buf_simple_pull_le16(buf);
	if (!BT_MESH_ADDR_IS_UNICAST(elem_addr)) {
		BT_WARN("Prohibited element address");
		return -EINVAL;
	}

	BT_DBG("elem_addr 0x%04x", elem_addr);

	mod_id = buf->data;

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4U);
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	mod = get_model(elem, buf, &vnd);
	if (!mod) {
		status = STATUS_INVALID_MODEL;
		goto send_status;
	}

	bt_mesh_model_extensions_walk(mod, mod_sub_clear_visitor, NULL);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_model_sub_store(mod);
	}

	status = STATUS_SUCCESS;

send_status:
	return send_mod_sub_status(model, ctx, status, elem_addr,
				   BT_MESH_ADDR_UNASSIGNED, mod_id, vnd);
}

struct mod_sub_list_ctx {
	uint16_t elem_idx;
	struct net_buf_simple *msg;
};

static enum bt_mesh_walk mod_sub_list_visitor(struct bt_mesh_model *mod, void *ctx)
{
	struct mod_sub_list_ctx *visit = ctx;
	int count = 0;
	int i;

	if (mod->elem_idx != visit->elem_idx) {
		return BT_MESH_WALK_CONTINUE;
	}

	for (i = 0; i < ARRAY_SIZE(mod->groups); i++) {
		if (mod->groups[i] == BT_MESH_ADDR_UNASSIGNED) {
			continue;
		}

		if (net_buf_simple_tailroom(visit->msg) <
		    2 + BT_MESH_MIC_SHORT) {
			BT_WARN("No room for all groups");
			return BT_MESH_WALK_STOP;
		}

		net_buf_simple_add_le16(visit->msg, mod->groups[i]);
		count++;
	}

	BT_DBG("sublist: model %u:%x: %u groups", mod->elem_idx, mod->id,
	       count);

	return BT_MESH_WALK_CONTINUE;
}

static int mod_sub_get(struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	NET_BUF_SIMPLE_DEFINE(msg, BT_MESH_TX_SDU_MAX);
	struct mod_sub_list_ctx visit_ctx;
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	uint16_t addr, id;

	addr = net_buf_simple_pull_le16(buf);
	if (!BT_MESH_ADDR_IS_UNICAST(addr)) {
		BT_WARN("Prohibited element address");
		return -EINVAL;
	}

	id = net_buf_simple_pull_le16(buf);

	BT_DBG("addr 0x%04x id 0x%04x", addr, id);

	bt_mesh_model_msg_init(&msg, OP_MOD_SUB_LIST);

	elem = bt_mesh_elem_find(addr);
	if (!elem) {
		net_buf_simple_add_u8(&msg, STATUS_INVALID_ADDRESS);
		net_buf_simple_add_le16(&msg, addr);
		net_buf_simple_add_le16(&msg, id);
		goto send_list;
	}

	mod = bt_mesh_model_find(elem, id);
	if (!mod) {
		net_buf_simple_add_u8(&msg, STATUS_INVALID_MODEL);
		net_buf_simple_add_le16(&msg, addr);
		net_buf_simple_add_le16(&msg, id);
		goto send_list;
	}

	net_buf_simple_add_u8(&msg, STATUS_SUCCESS);

	net_buf_simple_add_le16(&msg, addr);
	net_buf_simple_add_le16(&msg, id);

	visit_ctx.msg = &msg;
	visit_ctx.elem_idx = mod->elem_idx;
	bt_mesh_model_extensions_walk(mod, mod_sub_list_visitor, &visit_ctx);

send_list:
	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		BT_ERR("Unable to send Model Subscription List");
	}

	return 0;
}

static int mod_sub_get_vnd(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	NET_BUF_SIMPLE_DEFINE(msg, BT_MESH_TX_SDU_MAX);
	struct mod_sub_list_ctx visit_ctx;
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	uint16_t company, addr, id;

	addr = net_buf_simple_pull_le16(buf);
	if (!BT_MESH_ADDR_IS_UNICAST(addr)) {
		BT_WARN("Prohibited element address");
		return -EINVAL;
	}

	company = net_buf_simple_pull_le16(buf);
	id = net_buf_simple_pull_le16(buf);

	BT_DBG("addr 0x%04x company 0x%04x id 0x%04x", addr, company, id);

	bt_mesh_model_msg_init(&msg, OP_MOD_SUB_LIST_VND);

	elem = bt_mesh_elem_find(addr);
	if (!elem) {
		net_buf_simple_add_u8(&msg, STATUS_INVALID_ADDRESS);
		net_buf_simple_add_le16(&msg, addr);
		net_buf_simple_add_le16(&msg, company);
		net_buf_simple_add_le16(&msg, id);
		goto send_list;
	}

	mod = bt_mesh_model_find_vnd(elem, company, id);
	if (!mod) {
		net_buf_simple_add_u8(&msg, STATUS_INVALID_MODEL);
		net_buf_simple_add_le16(&msg, addr);
		net_buf_simple_add_le16(&msg, company);
		net_buf_simple_add_le16(&msg, id);
		goto send_list;
	}

	net_buf_simple_add_u8(&msg, STATUS_SUCCESS);

	net_buf_simple_add_le16(&msg, addr);
	net_buf_simple_add_le16(&msg, company);
	net_buf_simple_add_le16(&msg, id);

	visit_ctx.msg = &msg;
	visit_ctx.elem_idx = mod->elem_idx;
	bt_mesh_model_extensions_walk(mod, mod_sub_list_visitor, &visit_ctx);

send_list:
	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		BT_ERR("Unable to send Vendor Model Subscription List");
	}

	return 0;
}

static int mod_sub_va_add(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	uint16_t elem_addr, sub_addr;
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	uint8_t *label_uuid;
	uint8_t *mod_id;
	uint16_t *entry;
	uint8_t status;
	bool vnd;

	if ((buf->len != 20U) && (buf->len != 22U)) {
		BT_ERR("The message size for the application opcode is incorrect.");
		return -EMSGSIZE;
	}

	elem_addr = net_buf_simple_pull_le16(buf);
	if (!BT_MESH_ADDR_IS_UNICAST(elem_addr)) {
		BT_WARN("Prohibited element address");
		return -EINVAL;
	}

	label_uuid = net_buf_simple_pull_mem(buf, 16);

	BT_DBG("elem_addr 0x%04x", elem_addr);

	mod_id = buf->data;
	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4U);
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

	status = bt_mesh_va_add(label_uuid, &sub_addr);
	if (status != STATUS_SUCCESS) {
		goto send_status;
	}

	if (bt_mesh_model_find_group(&mod, sub_addr)) {
		/* Tried to add existing subscription */
		status = STATUS_SUCCESS;
		bt_mesh_va_del(label_uuid, NULL);
		goto send_status;
	}


	entry = bt_mesh_model_find_group(&mod, BT_MESH_ADDR_UNASSIGNED);
	if (!entry) {
		status = STATUS_INSUFF_RESOURCES;
		bt_mesh_va_del(label_uuid, NULL);
		goto send_status;
	}

	*entry = sub_addr;

	if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)) {
		bt_mesh_lpn_group_add(sub_addr);
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_model_sub_store(mod);
	}

	status = STATUS_SUCCESS;

send_status:
	return send_mod_sub_status(model, ctx, status, elem_addr, sub_addr,
				   mod_id, vnd);
}

static int mod_sub_va_del(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	uint16_t elem_addr, sub_addr;
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	uint8_t *label_uuid;
	uint8_t *mod_id;
	uint16_t *match;
	uint8_t status;
	bool vnd;

	if ((buf->len != 20U) && (buf->len != 22U)) {
		BT_ERR("The message size for the application opcode is incorrect.");
		return -EMSGSIZE;
	}

	elem_addr = net_buf_simple_pull_le16(buf);
	if (!BT_MESH_ADDR_IS_UNICAST(elem_addr)) {
		BT_WARN("Prohibited element address");
		return -EINVAL;
	}

	label_uuid = net_buf_simple_pull_mem(buf, 16);

	BT_DBG("elem_addr 0x%04x", elem_addr);

	mod_id = buf->data;

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4U);
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

	status = bt_mesh_va_del(label_uuid, &sub_addr);
	if (sub_addr == BT_MESH_ADDR_UNASSIGNED) {
		goto send_status;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)) {
		bt_mesh_lpn_group_del(&sub_addr, 1);
	}

	match = bt_mesh_model_find_group(&mod, sub_addr);
	if (match) {
		*match = BT_MESH_ADDR_UNASSIGNED;

		if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
			bt_mesh_model_sub_store(mod);
		}

		status = STATUS_SUCCESS;
	} else {
		status = STATUS_CANNOT_REMOVE;
	}

send_status:
	return send_mod_sub_status(model, ctx, status, elem_addr, sub_addr,
				   mod_id, vnd);
}

static int mod_sub_va_overwrite(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	uint16_t elem_addr, sub_addr = BT_MESH_ADDR_UNASSIGNED;
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	uint8_t *label_uuid;
	uint8_t *mod_id;
	uint8_t status;
	bool vnd;

	if ((buf->len != 20U) && (buf->len != 22U)) {
		BT_ERR("The message size for the application opcode is incorrect.");
		return -EMSGSIZE;
	}

	elem_addr = net_buf_simple_pull_le16(buf);
	if (!BT_MESH_ADDR_IS_UNICAST(elem_addr)) {
		BT_WARN("Prohibited element address");
		return -EINVAL;
	}

	label_uuid = net_buf_simple_pull_mem(buf, 16);

	BT_DBG("elem_addr 0x%04x", elem_addr);

	mod_id = buf->data;

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4U);
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	mod = get_model(elem, buf, &vnd);
	if (!mod) {
		status = STATUS_INVALID_MODEL;
		goto send_status;
	}


	if (ARRAY_SIZE(mod->groups) > 0) {

		status = bt_mesh_va_add(label_uuid, &sub_addr);
		if (status == STATUS_SUCCESS) {
			bt_mesh_model_extensions_walk(mod, mod_sub_clear_visitor, NULL);
			mod->groups[0] = sub_addr;

			if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
				bt_mesh_model_sub_store(mod);
			}

			if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)) {
				bt_mesh_lpn_group_add(sub_addr);
			}
		}
	} else {
		status = STATUS_INSUFF_RESOURCES;
	}

send_status:
	return send_mod_sub_status(model, ctx, status, elem_addr, sub_addr,
				   mod_id, vnd);
}

static int send_net_key_status(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx, uint16_t idx,
			       uint8_t status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_NET_KEY_STATUS, 3);

	bt_mesh_model_msg_init(&msg, OP_NET_KEY_STATUS);

	net_buf_simple_add_u8(&msg, status);
	net_buf_simple_add_le16(&msg, idx);

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		BT_ERR("Unable to send NetKey Status");
	}

	return 0;
}

static int net_key_add(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	uint8_t status;
	uint16_t idx;

	idx = net_buf_simple_pull_le16(buf);
	if (idx > 0xfff) {
		BT_ERR("Invalid NetKeyIndex 0x%04x", idx);
		return -EINVAL;
	}

	BT_DBG("idx 0x%04x", idx);

	status = bt_mesh_subnet_add(idx, buf->data);

	return send_net_key_status(model, ctx, idx, status);
}

static int net_key_update(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	uint8_t status;
	uint16_t idx;

	idx = net_buf_simple_pull_le16(buf);
	if (idx > 0xfff) {
		BT_ERR("Invalid NetKeyIndex 0x%04x", idx);
		return -EINVAL;
	}

	status = bt_mesh_subnet_update(idx, buf->data);

	return send_net_key_status(model, ctx, idx, status);
}

static int net_key_del(struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	uint16_t del_idx;

	del_idx = net_buf_simple_pull_le16(buf);
	if (del_idx > 0xfff) {
		BT_ERR("Invalid NetKeyIndex 0x%04x", del_idx);
		return -EINVAL;
	}

	BT_DBG("idx 0x%04x", del_idx);

	/* The key that the message was encrypted with cannot be removed.
	 * The NetKey List must contain a minimum of one NetKey.
	 */
	if (ctx->net_idx == del_idx) {
		return send_net_key_status(model, ctx, del_idx,
					   STATUS_CANNOT_REMOVE);
	}

	(void)bt_mesh_subnet_del(del_idx);

	return send_net_key_status(model, ctx, del_idx, STATUS_SUCCESS);
}

static int net_key_get(struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_NET_KEY_LIST,
				 IDX_LEN(CONFIG_BT_MESH_SUBNET_COUNT));
	uint16_t net_idx[CONFIG_BT_MESH_SUBNET_COUNT];
	ssize_t count;
	int i;

	bt_mesh_model_msg_init(&msg, OP_NET_KEY_LIST);

	count = bt_mesh_subnets_get(net_idx, ARRAY_SIZE(net_idx), 0);
	if (count < 0 || count > ARRAY_SIZE(net_idx)) {
		count = ARRAY_SIZE(net_idx);
	}

	for (i = 0; i < count - 1; i += 2) {
		key_idx_pack(&msg, net_idx[i], net_idx[i + 1]);
	}

	if (i < count) {
		net_buf_simple_add_le16(&msg, net_idx[i]);
	}

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		BT_ERR("Unable to send NetKey List");
	}

	return 0;
}

static int send_node_id_status(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       uint8_t status,
			       uint16_t net_idx, uint8_t node_id)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_NODE_IDENTITY_STATUS, 4);

	bt_mesh_model_msg_init(&msg, OP_NODE_IDENTITY_STATUS);
	net_buf_simple_add_u8(&msg, status);
	net_buf_simple_add_le16(&msg, net_idx);
	net_buf_simple_add_u8(&msg, node_id);

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		BT_ERR("Unable to send Node Identity Status");
	}

	return 0;
}

static int node_identity_get(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	enum bt_mesh_feat_state node_id;
	uint8_t status;
	uint16_t idx;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	idx = net_buf_simple_pull_le16(buf);
	if (idx > 0xfff) {
		BT_ERR("Invalid NetKeyIndex 0x%04x", idx);
		return -EINVAL;
	}

	status = bt_mesh_subnet_node_id_get(idx, &node_id);

	return send_node_id_status(model, ctx, status, idx, node_id);
}

static int node_identity_set(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	uint8_t node_id, status;
	uint16_t idx;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	idx = net_buf_simple_pull_le16(buf);
	if (idx > 0xfff) {
		BT_WARN("Invalid NetKeyIndex 0x%04x", idx);
		return -EINVAL;
	}

	node_id = net_buf_simple_pull_u8(buf);
	if (node_id != 0x00 && node_id != 0x01) {
		BT_WARN("Invalid Node ID value 0x%02x", node_id);
		return -EINVAL;
	}

	status = bt_mesh_subnet_node_id_set(idx, node_id);
	if (status == STATUS_INVALID_NETKEY) {
		return send_node_id_status(model, ctx, status, idx,
					   BT_MESH_NODE_IDENTITY_STOPPED);
	}

	if (status == STATUS_FEAT_NOT_SUPP) {
		/* Should return success, even if feature isn't supported: */
		return send_node_id_status(model, ctx, STATUS_SUCCESS, idx,
					   BT_MESH_NODE_IDENTITY_NOT_SUPPORTED);
	}

	return send_node_id_status(model, ctx, status, idx, node_id);
}

static void create_mod_app_status(struct net_buf_simple *msg,
				  struct bt_mesh_model *mod, bool vnd,
				  uint16_t elem_addr, uint16_t app_idx,
				  uint8_t status, uint8_t *mod_id)
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

static int mod_app_bind(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_MOD_APP_STATUS, 9);
	uint16_t elem_addr, key_app_idx;
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	uint8_t *mod_id, status;
	bool vnd;

	if ((buf->len != 6U) && (buf->len != 8U)) {
		BT_ERR("The message size for the application opcode is incorrect.");
		return -EMSGSIZE;
	}

	elem_addr = net_buf_simple_pull_le16(buf);
	if (!BT_MESH_ADDR_IS_UNICAST(elem_addr)) {
		BT_WARN("Prohibited element address");
		return -EINVAL;
	}

	key_app_idx = net_buf_simple_pull_le16(buf);
	mod_id = buf->data;

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4U);
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

	if (IS_ENABLED(CONFIG_BT_TESTING) && status == STATUS_SUCCESS) {
		bt_test_mesh_model_bound(ctx->addr, mod, key_app_idx);
	}

send_status:
	BT_DBG("status 0x%02x", status);
	create_mod_app_status(&msg, mod, vnd, elem_addr, key_app_idx, status,
			      mod_id);

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		BT_ERR("Unable to send Model App Bind Status response");
	}

	return 0;
}

static int mod_app_unbind(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_MOD_APP_STATUS, 9);
	uint16_t elem_addr, key_app_idx;
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	uint8_t *mod_id, status;
	bool vnd;

	if ((buf->len != 6U) && (buf->len != 8U)) {
		BT_ERR("The message size for the application opcode is incorrect.");
		return -EMSGSIZE;
	}

	elem_addr = net_buf_simple_pull_le16(buf);
	if (!BT_MESH_ADDR_IS_UNICAST(elem_addr)) {
		BT_WARN("Prohibited element address");
		return -EINVAL;
	}

	key_app_idx = net_buf_simple_pull_le16(buf);
	mod_id = buf->data;

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4U);
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	mod = get_model(elem, buf, &vnd);
	if (!mod) {
		status = STATUS_INVALID_MODEL;
		goto send_status;
	}

	status = mod_unbind(mod, key_app_idx, true);

	if (IS_ENABLED(CONFIG_BT_TESTING) && status == STATUS_SUCCESS) {
		bt_test_mesh_model_unbound(ctx->addr, mod, key_app_idx);
	}

send_status:
	BT_DBG("status 0x%02x", status);
	create_mod_app_status(&msg, mod, vnd, elem_addr, key_app_idx, status,
			      mod_id);

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		BT_ERR("Unable to send Model App Unbind Status response");
	}

	return 0;
}

#define KEY_LIST_LEN (CONFIG_BT_MESH_MODEL_KEY_COUNT * 2)

static int mod_app_get(struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	NET_BUF_SIMPLE_DEFINE(msg,
			      MAX(BT_MESH_MODEL_BUF_LEN(OP_VND_MOD_APP_LIST,
							9 + KEY_LIST_LEN),
				  BT_MESH_MODEL_BUF_LEN(OP_SIG_MOD_APP_LIST,
							9 + KEY_LIST_LEN)));
	struct bt_mesh_model *mod;
	struct bt_mesh_elem *elem;
	uint8_t *mod_id, status;
	uint16_t elem_addr;
	bool vnd;

	if ((buf->len != 4U) && (buf->len != 6U)) {
		BT_ERR("The message size for the application opcode is incorrect.");
		return -EMSGSIZE;
	}

	elem_addr = net_buf_simple_pull_le16(buf);
	if (!BT_MESH_ADDR_IS_UNICAST(elem_addr)) {
		BT_WARN("Prohibited element address");
		return -EINVAL;
	}

	mod_id = buf->data;

	BT_DBG("elem_addr 0x%04x", elem_addr);

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		mod = NULL;
		vnd = (buf->len == 4U);
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
		bt_mesh_model_msg_init(&msg, OP_VND_MOD_APP_LIST);
	} else {
		bt_mesh_model_msg_init(&msg, OP_SIG_MOD_APP_LIST);
	}

	net_buf_simple_add_u8(&msg, status);
	net_buf_simple_add_le16(&msg, elem_addr);

	if (vnd) {
		net_buf_simple_add_mem(&msg, mod_id, 4);
	} else {
		net_buf_simple_add_mem(&msg, mod_id, 2);
	}

	if (mod) {
		int i;

		for (i = 0; i < ARRAY_SIZE(mod->keys); i++) {
			if (mod->keys[i] != BT_MESH_KEY_UNUSED) {
				net_buf_simple_add_le16(&msg, mod->keys[i]);
			}
		}
	}

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		BT_ERR("Unable to send Model Application List message");
	}

	return 0;
}

static void reset_send_start(uint16_t duration, int err, void *cb_data)
{
	if (err) {
		BT_ERR("Sending Node Reset Status failed (err %d)", err);
		k_work_submit(&node_reset_pending);
	}
}

static void reset_send_end(int err, void *cb_data)
{
	k_work_submit(&node_reset_pending);
}

static int node_reset(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf)
{
	static const struct bt_mesh_send_cb reset_cb = {
		.start = reset_send_start,
		.end = reset_send_end,
	};

	BT_MESH_MODEL_BUF_DEFINE(msg, OP_NODE_RESET_STATUS, 0);

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	bt_mesh_model_msg_init(&msg, OP_NODE_RESET_STATUS);

	if (bt_mesh_model_send(model, ctx, &msg, &reset_cb, NULL)) {
		BT_ERR("Unable to send Node Reset Status");
	}

	return 0;
}

static int send_friend_status(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_FRIEND_STATUS, 1);

	bt_mesh_model_msg_init(&msg, OP_FRIEND_STATUS);
	net_buf_simple_add_u8(&msg, bt_mesh_friend_get());

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		BT_ERR("Unable to send Friend Status");
	}

	return 0;
}

static int friend_get(struct bt_mesh_model *model,
		      struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf)
{
	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	return send_friend_status(model, ctx);
}

static int friend_set(struct bt_mesh_model *model,
		      struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf)
{
	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	if (buf->data[0] != 0x00 && buf->data[0] != 0x01) {
		BT_WARN("Invalid Friend value 0x%02x", buf->data[0]);
		return -EINVAL;
	}

	(void)bt_mesh_friend_set(buf->data[0]);

	return send_friend_status(model, ctx);
}

static int lpn_timeout_get(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_LPN_TIMEOUT_STATUS, 5);
	struct bt_mesh_friend *frnd;
	int32_t timeout_steps;
	uint16_t lpn_addr;

	lpn_addr = net_buf_simple_pull_le16(buf);

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x lpn_addr 0x%02x",
	       ctx->net_idx, ctx->app_idx, ctx->addr, lpn_addr);

	if (!BT_MESH_ADDR_IS_UNICAST(lpn_addr)) {
		BT_WARN("Invalid LPNAddress; ignoring msg");
		return -EINVAL;
	}

	bt_mesh_model_msg_init(&msg, OP_LPN_TIMEOUT_STATUS);
	net_buf_simple_add_le16(&msg, lpn_addr);

	if (!IS_ENABLED(CONFIG_BT_MESH_FRIEND)) {
		timeout_steps = 0;
		goto send_rsp;
	}

	frnd = bt_mesh_friend_find(BT_MESH_KEY_ANY, lpn_addr, true, true);
	if (!frnd) {
		timeout_steps = 0;
		goto send_rsp;
	}

	/* PollTimeout should be reported in steps of 100ms. */
	timeout_steps = frnd->poll_to / 100;

send_rsp:
	net_buf_simple_add_le24(&msg, timeout_steps);

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		BT_ERR("Unable to send LPN PollTimeout Status");
	}

	return 0;
}

static int send_krp_status(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx, uint16_t idx,
			   uint8_t phase, uint8_t status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_KRP_STATUS, 4);

	bt_mesh_model_msg_init(&msg, OP_KRP_STATUS);

	net_buf_simple_add_u8(&msg, status);
	net_buf_simple_add_le16(&msg, idx);
	net_buf_simple_add_u8(&msg, phase);

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		BT_ERR("Unable to send Key Refresh State Status");
	}

	return 0;
}

static int krp_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		    struct net_buf_simple *buf)
{
	uint8_t kr_phase, status;
	uint16_t idx;

	idx = net_buf_simple_pull_le16(buf);
	if (idx > 0xfff) {
		BT_ERR("Invalid NetKeyIndex 0x%04x", idx);
		return -EINVAL;
	}

	BT_DBG("idx 0x%04x", idx);

	status = bt_mesh_subnet_kr_phase_get(idx, &kr_phase);

	return send_krp_status(model, ctx, idx, kr_phase, status);
}

static int krp_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		   struct net_buf_simple *buf)
{
	uint8_t phase, status;
	uint16_t idx;

	idx = net_buf_simple_pull_le16(buf);
	phase = net_buf_simple_pull_u8(buf);

	if (idx > 0xfff) {
		BT_ERR("Invalid NetKeyIndex 0x%04x", idx);
		return -EINVAL;
	}

	status = bt_mesh_subnet_kr_phase_set(idx, &phase);
	if (status == STATUS_CANNOT_UPDATE) {
		BT_ERR("Invalid kr phase transition 0x%02x", phase);
		return -EINVAL;
	}

	return send_krp_status(model, ctx, idx, phase, status);
}

static uint8_t hb_pub_count_log(uint16_t val)
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

struct hb_pub_param {
	uint16_t dst;
	uint8_t  count_log;
	uint8_t  period_log;
	uint8_t  ttl;
	uint16_t feat;
	uint16_t net_idx;
} __packed;

static int hb_pub_send_status(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx, uint8_t status,
			      const struct bt_mesh_hb_pub *pub)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_HEARTBEAT_PUB_STATUS, 10);

	BT_DBG("src 0x%04x status 0x%02x", ctx->addr, status);

	bt_mesh_model_msg_init(&msg, OP_HEARTBEAT_PUB_STATUS);

	net_buf_simple_add_u8(&msg, status);

	net_buf_simple_add_le16(&msg, pub->dst);
	net_buf_simple_add_u8(&msg, hb_pub_count_log(pub->count));
	net_buf_simple_add_u8(&msg, bt_mesh_hb_log(pub->period));
	net_buf_simple_add_u8(&msg, pub->ttl);
	net_buf_simple_add_le16(&msg, pub->feat);
	net_buf_simple_add_le16(&msg, pub->net_idx);

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		BT_ERR("Unable to send Heartbeat Publication Status");
	}

	return 0;
}

static int heartbeat_pub_get(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	struct bt_mesh_hb_pub pub;

	BT_DBG("src 0x%04x", ctx->addr);

	bt_mesh_hb_pub_get(&pub);

	return hb_pub_send_status(model, ctx, STATUS_SUCCESS, &pub);
}

static int heartbeat_pub_set(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	struct hb_pub_param *param = (void *)buf->data;
	struct bt_mesh_hb_pub pub;
	uint8_t status;

	BT_DBG("src 0x%04x", ctx->addr);

	pub.dst = sys_le16_to_cpu(param->dst);
	pub.count = bt_mesh_hb_pwr2(param->count_log);
	pub.period = bt_mesh_hb_pwr2(param->period_log);
	pub.ttl = param->ttl;
	pub.feat = sys_le16_to_cpu(param->feat);
	pub.net_idx = sys_le16_to_cpu(param->net_idx);

	/* All other address types but virtual are valid */
	if (BT_MESH_ADDR_IS_VIRTUAL(pub.dst)) {
		status = STATUS_INVALID_ADDRESS;
		goto rsp;
	}

	if (param->count_log > 0x11 && param->count_log != 0xff) {
		status = STATUS_CANNOT_SET;
		goto rsp;
	}

	if (param->period_log > 0x10) {
		status = STATUS_CANNOT_SET;
		goto rsp;
	}

	if (param->ttl > BT_MESH_TTL_MAX && param->ttl != BT_MESH_TTL_DEFAULT) {
		BT_ERR("Invalid TTL value 0x%02x", param->ttl);
		return -EINVAL;
	}

	if (pub.net_idx > 0xfff) {
		BT_ERR("Invalid NetKeyIndex 0x%04x", pub.net_idx);
		return -EINVAL;
	}

	status = bt_mesh_hb_pub_set(&pub);

rsp:
	return hb_pub_send_status(model, ctx, status, &pub);
}

static int hb_sub_send_status(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      const struct bt_mesh_hb_sub *sub)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_HEARTBEAT_SUB_STATUS, 9);

	BT_DBG("src 0x%04x ", ctx->addr);

	bt_mesh_model_msg_init(&msg, OP_HEARTBEAT_SUB_STATUS);

	net_buf_simple_add_u8(&msg, STATUS_SUCCESS);
	net_buf_simple_add_le16(&msg, sub->src);
	net_buf_simple_add_le16(&msg, sub->dst);
	net_buf_simple_add_u8(&msg, bt_mesh_hb_log(sub->remaining));
	net_buf_simple_add_u8(&msg, bt_mesh_hb_log(sub->count));
	net_buf_simple_add_u8(&msg, sub->min_hops);
	net_buf_simple_add_u8(&msg, sub->max_hops);

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		BT_ERR("Unable to send Heartbeat Subscription Status");
	}

	return 0;
}

static int heartbeat_sub_get(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	struct bt_mesh_hb_sub sub;

	BT_DBG("src 0x%04x", ctx->addr);

	bt_mesh_hb_sub_get(&sub);

	return hb_sub_send_status(model, ctx, &sub);
}

static int heartbeat_sub_set(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	uint8_t period_log, status;
	struct bt_mesh_hb_sub sub;
	uint16_t sub_src, sub_dst;
	uint32_t period;
	int err;

	BT_DBG("src 0x%04x", ctx->addr);

	sub_src = net_buf_simple_pull_le16(buf);
	sub_dst = net_buf_simple_pull_le16(buf);
	period_log = net_buf_simple_pull_u8(buf);

	BT_DBG("sub_src 0x%04x sub_dst 0x%04x period 0x%02x",
	       sub_src, sub_dst, period_log);

	if (period_log > 0x11) {
		BT_WARN("Prohibited subscription period 0x%02x", period_log);
		return -EINVAL;
	}

	period = bt_mesh_hb_pwr2(period_log);

	status = bt_mesh_hb_sub_set(sub_src, sub_dst, period);
	if (status != STATUS_SUCCESS) {
		/* All errors are caused by invalid packets, which should be
		 * ignored.
		 */
		return -EINVAL;
	}

	bt_mesh_hb_sub_get(&sub);

	/* MESH/NODE/CFG/HBS/BV-01-C expects the MinHops to be 0x7f after
	 * disabling subscription, but 0x00 for subsequent Get requests.
	 */
	if (sub.src == BT_MESH_ADDR_UNASSIGNED || !period_log) {
		sub.min_hops = BT_MESH_TTL_MAX;
	}

	err = hb_sub_send_status(model, ctx, &sub);
	if (err) {
		return err;
	}

	/* MESH/NODE/CFG/HBS/BV-02-C expects us to return previous
	 * count value and then reset it to 0.
	 */
	if (sub.src != BT_MESH_ADDR_UNASSIGNED &&
	    sub.dst != BT_MESH_ADDR_UNASSIGNED && !period) {
		bt_mesh_hb_sub_reset_count();
	}

	return 0;
}

const struct bt_mesh_model_op bt_mesh_cfg_srv_op[] = {
	{ OP_DEV_COMP_DATA_GET,        BT_MESH_LEN_EXACT(1),   dev_comp_data_get },
	{ OP_APP_KEY_ADD,              BT_MESH_LEN_EXACT(19),  app_key_add },
	{ OP_APP_KEY_UPDATE,           BT_MESH_LEN_EXACT(19),  app_key_update },
	{ OP_APP_KEY_DEL,              BT_MESH_LEN_EXACT(3),   app_key_del },
	{ OP_APP_KEY_GET,              BT_MESH_LEN_EXACT(2),   app_key_get },
	{ OP_BEACON_GET,               BT_MESH_LEN_EXACT(0),   beacon_get },
	{ OP_BEACON_SET,               BT_MESH_LEN_EXACT(1),   beacon_set },
	{ OP_DEFAULT_TTL_GET,          BT_MESH_LEN_EXACT(0),   default_ttl_get },
	{ OP_DEFAULT_TTL_SET,          BT_MESH_LEN_EXACT(1),   default_ttl_set },
	{ OP_GATT_PROXY_GET,           BT_MESH_LEN_EXACT(0),   gatt_proxy_get },
	{ OP_GATT_PROXY_SET,           BT_MESH_LEN_EXACT(1),   gatt_proxy_set },
	{ OP_NET_TRANSMIT_GET,         BT_MESH_LEN_EXACT(0),   net_transmit_get },
	{ OP_NET_TRANSMIT_SET,         BT_MESH_LEN_EXACT(1),   net_transmit_set },
	{ OP_RELAY_GET,                BT_MESH_LEN_EXACT(0),   relay_get },
	{ OP_RELAY_SET,                BT_MESH_LEN_EXACT(2),   relay_set },
	{ OP_MOD_PUB_GET,              BT_MESH_LEN_MIN(4),     mod_pub_get },
	{ OP_MOD_PUB_SET,              BT_MESH_LEN_MIN(11),    mod_pub_set },
	{ OP_MOD_PUB_VA_SET,           BT_MESH_LEN_MIN(25),    mod_pub_va_set },
	{ OP_MOD_SUB_ADD,              BT_MESH_LEN_MIN(6),     mod_sub_add },
	{ OP_MOD_SUB_VA_ADD,           BT_MESH_LEN_MIN(20),    mod_sub_va_add },
	{ OP_MOD_SUB_DEL,              BT_MESH_LEN_MIN(6),     mod_sub_del },
	{ OP_MOD_SUB_VA_DEL,           BT_MESH_LEN_MIN(20),    mod_sub_va_del },
	{ OP_MOD_SUB_OVERWRITE,        BT_MESH_LEN_MIN(6),     mod_sub_overwrite },
	{ OP_MOD_SUB_VA_OVERWRITE,     BT_MESH_LEN_MIN(20),    mod_sub_va_overwrite },
	{ OP_MOD_SUB_DEL_ALL,          BT_MESH_LEN_MIN(4),     mod_sub_del_all },
	{ OP_MOD_SUB_GET,              BT_MESH_LEN_EXACT(4),   mod_sub_get },
	{ OP_MOD_SUB_GET_VND,          BT_MESH_LEN_EXACT(6),   mod_sub_get_vnd },
	{ OP_NET_KEY_ADD,              BT_MESH_LEN_EXACT(18),  net_key_add },
	{ OP_NET_KEY_UPDATE,           BT_MESH_LEN_EXACT(18),  net_key_update },
	{ OP_NET_KEY_DEL,              BT_MESH_LEN_EXACT(2),   net_key_del },
	{ OP_NET_KEY_GET,              BT_MESH_LEN_EXACT(0),   net_key_get },
	{ OP_NODE_IDENTITY_GET,        BT_MESH_LEN_EXACT(2),   node_identity_get },
	{ OP_NODE_IDENTITY_SET,        BT_MESH_LEN_EXACT(3),   node_identity_set },
	{ OP_MOD_APP_BIND,             BT_MESH_LEN_MIN(6),     mod_app_bind },
	{ OP_MOD_APP_UNBIND,           BT_MESH_LEN_MIN(6),     mod_app_unbind },
	{ OP_SIG_MOD_APP_GET,          BT_MESH_LEN_MIN(4),     mod_app_get },
	{ OP_VND_MOD_APP_GET,          BT_MESH_LEN_MIN(6),     mod_app_get },
	{ OP_NODE_RESET,               BT_MESH_LEN_EXACT(0),   node_reset },
	{ OP_FRIEND_GET,               BT_MESH_LEN_EXACT(0),   friend_get },
	{ OP_FRIEND_SET,               BT_MESH_LEN_EXACT(1),   friend_set },
	{ OP_LPN_TIMEOUT_GET,          BT_MESH_LEN_EXACT(2),   lpn_timeout_get },
	{ OP_KRP_GET,                  BT_MESH_LEN_EXACT(2),   krp_get },
	{ OP_KRP_SET,                  BT_MESH_LEN_EXACT(3),   krp_set },
	{ OP_HEARTBEAT_PUB_GET,        BT_MESH_LEN_EXACT(0),   heartbeat_pub_get },
	{ OP_HEARTBEAT_PUB_SET,        BT_MESH_LEN_EXACT(9),   heartbeat_pub_set },
	{ OP_HEARTBEAT_SUB_GET,        BT_MESH_LEN_EXACT(0),   heartbeat_sub_get },
	{ OP_HEARTBEAT_SUB_SET,        BT_MESH_LEN_EXACT(5),   heartbeat_sub_set },
	BT_MESH_MODEL_OP_END,
};

static int cfg_srv_init(struct bt_mesh_model *model)
{
	if (!bt_mesh_model_in_primary(model)) {
		BT_ERR("Configuration Server only allowed in primary element");
		return -EINVAL;
	}

	/*
	 * Configuration Model security is device-key based and only the local
	 * device-key is allowed to access this model.
	 */
	model->keys[0] = BT_MESH_KEY_DEV_LOCAL;

	return 0;
}

const struct bt_mesh_model_cb bt_mesh_cfg_srv_cb = {
	.init = cfg_srv_init,
};

static void mod_reset(struct bt_mesh_model *mod, struct bt_mesh_elem *elem,
		      bool vnd, bool primary, void *user_data)
{
	size_t clear_count;

	/* Clear model state that isn't otherwise cleared. E.g. AppKey
	 * binding and model publication is cleared as a consequence
	 * of removing all app keys, however model subscription and user data
	 * clearing must be taken care of here.
	 */

	clear_count = mod_sub_list_clear(mod);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		if (clear_count) {
			bt_mesh_model_sub_store(mod);
		}
	}

	if (mod->cb && mod->cb->reset) {
		mod->cb->reset(mod);
	}
}

void bt_mesh_model_reset(void)
{
	bt_mesh_model_foreach(mod_reset, NULL);
}
