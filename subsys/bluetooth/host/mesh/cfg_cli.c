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

#include "foundation.h"

#define MSG_TIMEOUT K_SECONDS(10)

struct comp_data {
	u8_t *status;
	struct net_buf_simple *comp;
};

static struct bt_mesh_cfg_cli *cli;

static void comp_data_status(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	struct comp_data *param;
	size_t to_copy;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	if (cli->op_pending != OP_DEV_COMP_DATA_STATUS) {
		BT_WARN("Unexpected Composition Data Status");
		return;
	}

	param = cli->op_param;

	*(param->status) = net_buf_simple_pull_u8(buf);
	to_copy  = min(net_buf_simple_tailroom(param->comp), buf->len);
	net_buf_simple_add_mem(param->comp, buf->data, to_copy);

	k_sem_give(&cli->op_sync);
}

static void state_status_u8(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf,
			    u32_t expect_status)
{
	u8_t *status;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	if (cli->op_pending != expect_status) {
		BT_WARN("Unexpected Status (0x%08x != 0x%08x)",
			cli->op_pending, expect_status);
		return;
	}

	status = cli->op_param;
	*status = net_buf_simple_pull_u8(buf);

	k_sem_give(&cli->op_sync);
}

static void beacon_status(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	state_status_u8(model, ctx, buf, OP_BEACON_STATUS);
}

static void ttl_status(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	state_status_u8(model, ctx, buf, OP_DEFAULT_TTL_STATUS);
}

static void friend_status(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	state_status_u8(model, ctx, buf, OP_FRIEND_STATUS);
}

static void gatt_proxy_status(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	state_status_u8(model, ctx, buf, OP_GATT_PROXY_STATUS);
}

struct relay_param {
	u8_t *status;
	u8_t *transmit;
};

static void relay_status(struct bt_mesh_model *model,
			 struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	struct relay_param *param;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	if (cli->op_pending != OP_RELAY_STATUS) {
		BT_WARN("Unexpected Relay Status message");
		return;
	}

	param = cli->op_param;
	*param->status = net_buf_simple_pull_u8(buf);
	*param->transmit = net_buf_simple_pull_u8(buf);

	k_sem_give(&cli->op_sync);
}

struct app_key_param {
	u8_t *status;
	u16_t net_idx;
	u16_t app_idx;
};

static void app_key_status(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct app_key_param *param;
	u16_t net_idx, app_idx;
	u8_t status;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	if (cli->op_pending != OP_APP_KEY_STATUS) {
		BT_WARN("Unexpected Relay Status message");
		return;
	}

	status = net_buf_simple_pull_u8(buf);
	key_idx_unpack(buf, &net_idx, &app_idx);

	param = cli->op_param;
	if (param->net_idx != net_idx || param->app_idx != app_idx) {
		BT_WARN("App Key Status key indices did not match");
		return;
	}

	*param->status = status;

	k_sem_give(&cli->op_sync);
}

struct mod_app_param {
	u8_t *status;
	u16_t elem_addr;
	u16_t mod_app_idx;
	u16_t mod_id;
	bool vnd;
	u16_t cid;
};

static void mod_app_status(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	u16_t elem_addr, mod_app_idx, mod_id, cid;
	struct mod_app_param *param;
	u8_t status;
	bool vnd;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	if (cli->op_pending != OP_MOD_APP_STATUS) {
		BT_WARN("Unexpected Model App Status message");
		return;
	}

	status = net_buf_simple_pull_u8(buf);
	elem_addr = net_buf_simple_pull_le16(buf);
	mod_app_idx = net_buf_simple_pull_le16(buf);

	if (buf->len >= 4) {
		vnd = true;
		cid = net_buf_simple_pull_le16(buf);
	} else {
		vnd = false;
		cid = 0;
	}

	mod_id = net_buf_simple_pull_le16(buf);

	param = cli->op_param;
	if (param->elem_addr != elem_addr ||
	    param->mod_app_idx != mod_app_idx || param->vnd != vnd ||
	    param->mod_id != mod_id || param->cid != cid) {
		BT_WARN("Model App Status parameters did not match");
		return;
	}

	*param->status = status;

	k_sem_give(&cli->op_sync);
}

struct mod_sub_param {
	u8_t *status;
	u16_t elem_addr;
	u16_t sub_addr;
	u16_t mod_id;
	bool vnd;
	u16_t cid;
};

static void mod_sub_status(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	u16_t elem_addr, sub_addr, mod_id, cid;
	struct mod_sub_param *param;
	u8_t status;
	bool vnd;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	if (cli->op_pending != OP_MOD_SUB_STATUS) {
		BT_WARN("Unexpected Model Subscription Status message");
		return;
	}

	status = net_buf_simple_pull_u8(buf);
	elem_addr = net_buf_simple_pull_le16(buf);
	sub_addr = net_buf_simple_pull_le16(buf);

	if (buf->len >= 4) {
		vnd = true;
		cid = net_buf_simple_pull_le16(buf);
	} else {
		vnd = false;
		cid = 0;
	}

	mod_id = net_buf_simple_pull_le16(buf);

	param = cli->op_param;
	if (param->elem_addr != elem_addr ||
	    param->sub_addr != sub_addr || param->vnd != vnd ||
	    param->mod_id != mod_id || param->cid != cid) {
		BT_WARN("Model Subscription Status parameters did not match");
		return;
	}

	*param->status = status;

	k_sem_give(&cli->op_sync);
}

struct hb_sub_param {
	u8_t *status;
	u16_t src;
	u16_t dst;
	u8_t *period;
	u8_t *count;
	u8_t *min;
	u8_t *max;
};

static void hb_sub_status(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	u16_t src, dst;
	struct hb_sub_param *param;
	u8_t status;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	if (cli->op_pending != OP_HEARTBEAT_SUB_STATUS) {
		BT_WARN("Unexpected Heartbeat Subscription Status message");
		return;
	}

	status = net_buf_simple_pull_u8(buf);
	src = net_buf_simple_pull_le16(buf);
	dst = net_buf_simple_pull_le16(buf);

	param = cli->op_param;
	if (param->src != src || param->dst != dst) {
		BT_WARN("Heartbeat Subscription Status parameters mismatch");
		return;
	}

	*param->status = status;

	if (param->period) {
		*param->period = net_buf_simple_pull_u8(buf);
	} else {
		net_buf_simple_pull(buf, 1);
	}

	if (param->count) {
		*param->count = net_buf_simple_pull_u8(buf);
	} else {
		net_buf_simple_pull(buf, 1);
	}

	if (param->min) {
		*param->min = net_buf_simple_pull_u8(buf);
	} else {
		net_buf_simple_pull(buf, 1);
	}

	if (param->max) {
		*param->max = net_buf_simple_pull_u8(buf);
	} else {
		net_buf_simple_pull(buf, 1);
	}

	k_sem_give(&cli->op_sync);
}

struct hb_pub_param {
	u8_t   *status;
	u16_t  *dst;
	u8_t   *count;
	u8_t   *period;
	u8_t   *ttl;
	u16_t  *feat;
	u16_t  *net_idx;
};

static void hb_pub_status(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct hb_pub_param *param;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	if (cli->op_pending != OP_HEARTBEAT_PUB_STATUS) {
		BT_WARN("Unexpected Heartbeat Publication Status message");
		return;
	}

	*param->status = net_buf_simple_pull_u8(buf);
	*param->dst = net_buf_simple_pull_le16(buf);
	*param->count = net_buf_simple_pull_u8(buf);
	*param->period = net_buf_simple_pull_u8(buf);
	*param->ttl = net_buf_simple_pull_u8(buf);
	*param->feat = net_buf_simple_pull_u8(buf);
	*param->net_idx = net_buf_simple_pull_u8(buf);

	k_sem_give(&cli->op_sync);
}

const struct bt_mesh_model_op bt_mesh_cfg_cli_op[] = {
	{ OP_DEV_COMP_DATA_STATUS,   15,  comp_data_status },
	{ OP_BEACON_STATUS,          1,   beacon_status },
	{ OP_DEFAULT_TTL_STATUS,     1,   ttl_status },
	{ OP_FRIEND_STATUS,          1,   friend_status },
	{ OP_GATT_PROXY_STATUS,      1,   gatt_proxy_status },
	{ OP_RELAY_STATUS,           2,   relay_status },
	{ OP_APP_KEY_STATUS,         4,   app_key_status },
	{ OP_MOD_APP_STATUS,         7,   mod_app_status },
	{ OP_MOD_SUB_STATUS,         7,   mod_sub_status },
	{ OP_HEARTBEAT_SUB_STATUS,   9,   hb_sub_status },
	{ OP_HEARTBEAT_PUB_STATUS,   10,  hb_pub_status },
	BT_MESH_MODEL_OP_END,
};

static int check_cli(void)
{
	if (!cli) {
		BT_ERR("No available Configuration Client context!");
		return -EINVAL;
	}

	if (cli->op_pending) {
		BT_WARN("Another synchronous operation pending");
		return -EBUSY;
	}

	return 0;
}

int bt_mesh_cfg_comp_data_get(u16_t net_idx, u16_t addr, u8_t page,
			      u8_t *status, struct net_buf_simple *comp)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 1 + 4);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.addr = addr,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	struct comp_data param = {
		.status = status,
		.comp = comp,
	};
	int err;

	err = check_cli();
	if (err) {
		return err;
	}

	bt_mesh_model_msg_init(msg, OP_DEV_COMP_DATA_GET);
	net_buf_simple_add_u8(msg, page);

	err = bt_mesh_model_send(cli->model, &ctx, msg, NULL, NULL);
	if (err) {
		BT_ERR("model_send() failed (err %d)", err);
		return err;
	}

	cli->op_param = &param;
	cli->op_pending = OP_DEV_COMP_DATA_STATUS;

	err = k_sem_take(&cli->op_sync, MSG_TIMEOUT);

	cli->op_pending = 0;
	cli->op_param = NULL;

	return err;
}

static int get_state_u8(u16_t net_idx, u16_t addr, u32_t op, u32_t rsp,
			u8_t *val)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 0 + 4);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.addr = addr,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	err = check_cli();
	if (err) {
		return err;
	}

	bt_mesh_model_msg_init(msg, op);

	err = bt_mesh_model_send(cli->model, &ctx, msg, NULL, NULL);
	if (err) {
		BT_ERR("model_send() failed (err %d)", err);
		return err;
	}

	cli->op_param = val;
	cli->op_pending = rsp;

	err = k_sem_take(&cli->op_sync, MSG_TIMEOUT);

	cli->op_pending = 0;
	cli->op_param = NULL;

	return err;
}

static int set_state_u8(u16_t net_idx, u16_t addr, u32_t op, u32_t rsp,
			u8_t new_val, u8_t *val)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 1 + 4);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.addr = addr,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	err = check_cli();
	if (err) {
		return err;
	}

	bt_mesh_model_msg_init(msg, op);
	net_buf_simple_add_u8(msg, new_val);

	err = bt_mesh_model_send(cli->model, &ctx, msg, NULL, NULL);
	if (err) {
		BT_ERR("model_send() failed (err %d)", err);
		return err;
	}

	cli->op_param = val;
	cli->op_pending = rsp;

	err = k_sem_take(&cli->op_sync, MSG_TIMEOUT);

	cli->op_pending = 0;
	cli->op_param = NULL;

	return err;
}

int bt_mesh_cfg_beacon_get(u16_t net_idx, u16_t addr, u8_t *status)
{
	return get_state_u8(net_idx, addr, OP_BEACON_GET, OP_BEACON_STATUS,
			    status);
}

int bt_mesh_cfg_beacon_set(u16_t net_idx, u16_t addr, u8_t val, u8_t *status)
{
	return set_state_u8(net_idx, addr, OP_BEACON_SET, OP_BEACON_STATUS,
			    val, status);
}

int bt_mesh_cfg_ttl_get(u16_t net_idx, u16_t addr, u8_t *ttl)
{
	return get_state_u8(net_idx, addr, OP_DEFAULT_TTL_GET,
			    OP_DEFAULT_TTL_STATUS, ttl);
}

int bt_mesh_cfg_ttl_set(u16_t net_idx, u16_t addr, u8_t val, u8_t *ttl)
{
	return set_state_u8(net_idx, addr, OP_DEFAULT_TTL_SET,
			    OP_DEFAULT_TTL_STATUS, val, ttl);
}

int bt_mesh_cfg_friend_get(u16_t net_idx, u16_t addr, u8_t *status)
{
	return get_state_u8(net_idx, addr, OP_FRIEND_GET,
			    OP_FRIEND_STATUS, status);
}

int bt_mesh_cfg_friend_set(u16_t net_idx, u16_t addr, u8_t val, u8_t *status)
{
	return set_state_u8(net_idx, addr, OP_FRIEND_SET, OP_FRIEND_STATUS,
			    val, status);
}

int bt_mesh_cfg_gatt_proxy_get(u16_t net_idx, u16_t addr, u8_t *status)
{
	return get_state_u8(net_idx, addr, OP_GATT_PROXY_GET,
			    OP_GATT_PROXY_STATUS, status);
}

int bt_mesh_cfg_gatt_proxy_set(u16_t net_idx, u16_t addr, u8_t val,
			       u8_t *status)
{
	return set_state_u8(net_idx, addr, OP_GATT_PROXY_SET,
			    OP_GATT_PROXY_STATUS, val, status);
}

int bt_mesh_cfg_relay_get(u16_t net_idx, u16_t addr, u8_t *status,
			  u8_t *transmit)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 0 + 4);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.addr = addr,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	struct relay_param param = {
		.status = status,
		.transmit = transmit,
	};
	int err;

	err = check_cli();
	if (err) {
		return err;
	}

	bt_mesh_model_msg_init(msg, OP_RELAY_GET);

	err = bt_mesh_model_send(cli->model, &ctx, msg, NULL, NULL);
	if (err) {
		BT_ERR("model_send() failed (err %d)", err);
		return err;
	}

	cli->op_param = &param;
	cli->op_pending = OP_RELAY_STATUS;

	err = k_sem_take(&cli->op_sync, MSG_TIMEOUT);

	cli->op_pending = 0;
	cli->op_param = NULL;

	return err;
}

int bt_mesh_cfg_relay_set(u16_t net_idx, u16_t addr, u8_t new_relay,
			  u8_t new_transmit, u8_t *status, u8_t *transmit)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 2 + 4);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.addr = addr,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	struct relay_param param = {
		.status = status,
		.transmit = transmit,
	};
	int err;

	err = check_cli();
	if (err) {
		return err;
	}

	bt_mesh_model_msg_init(msg, OP_RELAY_SET);
	net_buf_simple_add_u8(msg, new_relay);
	net_buf_simple_add_u8(msg, new_transmit);

	err = bt_mesh_model_send(cli->model, &ctx, msg, NULL, NULL);
	if (err) {
		BT_ERR("model_send() failed (err %d)", err);
		return err;
	}

	cli->op_param = &param;
	cli->op_pending = OP_RELAY_STATUS;

	err = k_sem_take(&cli->op_sync, MSG_TIMEOUT);

	cli->op_pending = 0;
	cli->op_param = NULL;

	return err;
}

int bt_mesh_cfg_app_key_add(u16_t net_idx, u16_t addr, u16_t key_net_idx,
			    u16_t key_app_idx, const u8_t app_key[16],
			    u8_t *status)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(1 + 19 + 4);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.addr = addr,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	struct app_key_param param = {
		.status = status,
		.net_idx = key_net_idx,
		.app_idx = key_app_idx,
	};
	int err;

	err = check_cli();
	if (err) {
		return err;
	}

	bt_mesh_model_msg_init(msg, OP_APP_KEY_ADD);
	key_idx_pack(msg, key_net_idx, key_app_idx);
	net_buf_simple_add_mem(msg, app_key, 16);

	err = bt_mesh_model_send(cli->model, &ctx, msg, NULL, NULL);
	if (err) {
		BT_ERR("model_send() failed (err %d)", err);
		return err;
	}

	if (!status) {
		return 0;
	}

	cli->op_param = &param;
	cli->op_pending = OP_APP_KEY_STATUS;

	err = k_sem_take(&cli->op_sync, MSG_TIMEOUT);

	cli->op_pending = 0;
	cli->op_param = NULL;

	return err;
}

int mod_app_bind(u16_t net_idx, u16_t addr, u16_t elem_addr, u16_t mod_app_idx,
		 u16_t mod_id, bool vnd, u16_t cid, u8_t *status)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 8 + 4);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.addr = addr,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	struct mod_app_param param = {
		.status = status,
		.elem_addr = elem_addr,
		.mod_app_idx = mod_app_idx,
		.mod_id = mod_id,
		.vnd = vnd,
		.cid = cid,
	};
	int err;

	err = check_cli();
	if (err) {
		return err;
	}

	bt_mesh_model_msg_init(msg, OP_MOD_APP_BIND);
	net_buf_simple_add_le16(msg, elem_addr);
	net_buf_simple_add_le16(msg, mod_app_idx);

	if (vnd) {
		net_buf_simple_add_le16(msg, cid);
	}

	net_buf_simple_add_le16(msg, mod_id);

	err = bt_mesh_model_send(cli->model, &ctx, msg, NULL, NULL);
	if (err) {
		BT_ERR("model_send() failed (err %d)", err);
		return err;
	}

	if (!status) {
		return 0;
	}

	cli->op_param = &param;
	cli->op_pending = OP_MOD_APP_STATUS;

	err = k_sem_take(&cli->op_sync, MSG_TIMEOUT);

	cli->op_pending = 0;
	cli->op_param = NULL;

	return err;
}

int bt_mesh_cfg_mod_app_bind(u16_t net_idx, u16_t addr, u16_t elem_addr,
			     u16_t mod_app_idx, u16_t mod_id, u8_t *status)
{
	return mod_app_bind(net_idx, addr, elem_addr, mod_app_idx, mod_id,
			    false, 0, status);
}

int bt_mesh_cfg_mod_app_bind_vnd(u16_t net_idx, u16_t addr, u16_t elem_addr,
				 u16_t mod_app_idx, u16_t mod_id, u16_t cid,
				 u8_t *status)
{
	return mod_app_bind(net_idx, addr, elem_addr, mod_app_idx, mod_id,
			    true, cid, status);
}

int mod_sub_add(u16_t net_idx, u16_t addr, u16_t elem_addr, u16_t sub_addr,
		u16_t mod_id, bool vnd, u16_t cid, u8_t *status)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 8 + 4);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.addr = addr,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	struct mod_sub_param param = {
		.status = status,
		.elem_addr = elem_addr,
		.sub_addr = sub_addr,
		.mod_id = mod_id,
		.vnd = vnd,
		.cid = cid,
	};
	int err;

	err = check_cli();
	if (err) {
		return err;
	}

	bt_mesh_model_msg_init(msg, OP_MOD_SUB_ADD);
	net_buf_simple_add_le16(msg, elem_addr);
	net_buf_simple_add_le16(msg, sub_addr);

	if (vnd) {
		net_buf_simple_add_le16(msg, cid);
	}

	net_buf_simple_add_le16(msg, mod_id);

	err = bt_mesh_model_send(cli->model, &ctx, msg, NULL, NULL);
	if (err) {
		BT_ERR("model_send() failed (err %d)", err);
		return err;
	}

	if (!status) {
		return 0;
	}

	cli->op_param = &param;
	cli->op_pending = OP_MOD_SUB_STATUS;

	err = k_sem_take(&cli->op_sync, MSG_TIMEOUT);

	cli->op_pending = 0;
	cli->op_param = NULL;

	return err;
}

int bt_mesh_cfg_mod_sub_add(u16_t net_idx, u16_t addr, u16_t elem_addr,
			    u16_t sub_addr, u16_t mod_id, u8_t *status)
{
	return mod_sub_add(net_idx, addr, elem_addr, sub_addr, mod_id,
			   false, 0, status);
}

int bt_mesh_cfg_mod_sub_add_vnd(u16_t net_idx, u16_t addr, u16_t elem_addr,
				 u16_t sub_addr, u16_t mod_id, u16_t cid,
				 u8_t *status)
{
	return mod_sub_add(net_idx, addr, elem_addr, sub_addr, mod_id,
			   true, cid, status);
}

int bt_mesh_cfg_hb_sub_set(u16_t net_idx, u16_t addr, u16_t src, u16_t dst,
			   u8_t period, u8_t *status)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 5 + 4);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.addr = addr,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	struct hb_sub_param param = {
		.status = status,
		.src = src,
		.dst = dst,
	};
	int err;

	err = check_cli();
	if (err) {
		return err;
	}

	bt_mesh_model_msg_init(msg, OP_HEARTBEAT_SUB_SET);
	net_buf_simple_add_le16(msg, src);
	net_buf_simple_add_le16(msg, dst);
	net_buf_simple_add_u8(msg, period);

	err = bt_mesh_model_send(cli->model, &ctx, msg, NULL, NULL);
	if (err) {
		BT_ERR("model_send() failed (err %d)", err);
		return err;
	}

	if (!status) {
		return 0;
	}

	cli->op_param = &param;
	cli->op_pending = OP_HEARTBEAT_SUB_STATUS;

	err = k_sem_take(&cli->op_sync, MSG_TIMEOUT);

	cli->op_pending = 0;
	cli->op_param = NULL;

	return err;
}

int bt_mesh_cfg_hb_pub_set(u16_t net_idx, u16_t addr, u16_t pub_dst,
			   u8_t count, u8_t period, u8_t ttl, u16_t feat,
			   u16_t pub_net_idx, u8_t *status)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 9 + 4);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.addr = addr,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	struct hb_pub_param param = {
		.status = status,
		.dst = &pub_dst,
		.count = &count,
		.period = &period,
		.ttl = &ttl,
		.feat = &feat,
		.net_idx = &pub_net_idx,
	};
	int err;

	err = check_cli();
	if (err) {
		return err;
	}

	bt_mesh_model_msg_init(msg, OP_HEARTBEAT_PUB_SET);
	net_buf_simple_add_le16(msg, pub_dst);
	net_buf_simple_add_u8(msg, count);
	net_buf_simple_add_u8(msg, period);
	net_buf_simple_add_u8(msg, ttl);
	net_buf_simple_add_le16(msg, feat);
	net_buf_simple_add_le16(msg, pub_net_idx);

	err = bt_mesh_model_send(cli->model, &ctx, msg, NULL, NULL);
	if (err) {
		BT_ERR("model_send() failed (err %d)", err);
		return err;
	}

	if (!status) {
		return 0;
	}

	cli->op_param = &param;
	cli->op_pending = OP_HEARTBEAT_PUB_STATUS;

	err = k_sem_take(&cli->op_sync, MSG_TIMEOUT);

	cli->op_pending = 0;
	cli->op_param = NULL;

	return err;
}

int bt_mesh_cfg_cli_init(struct bt_mesh_model *model, bool primary)
{
	BT_DBG("primary %u", primary);

	if (!primary) {
		BT_ERR("Configuration Client only allowed in primary element");
		return -EINVAL;
	}

	if (!model->user_data) {
		BT_ERR("No Configuration Client context provided");
		return -EINVAL;
	}

	cli = model->user_data;
	cli->model = model;

	/* Configuration Model security is device-key based */
	model->keys[0] = BT_MESH_KEY_DEV;

	k_sem_init(&cli->op_sync, 0, 1);

	return 0;
}
