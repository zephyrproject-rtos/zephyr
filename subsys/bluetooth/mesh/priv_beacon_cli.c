/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/mesh.h>
#include "net.h"
#include "foundation.h"
#include "access.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_priv_beacon_cli);

static struct bt_mesh_priv_beacon_cli *cli;

static int handle_beacon_status(struct bt_mesh_model *mod,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	struct bt_mesh_priv_beacon *rsp;
	uint8_t beacon, rand_int;

	beacon = net_buf_simple_pull_u8(buf);
	rand_int = net_buf_simple_pull_u8(buf);

	if (beacon != BT_MESH_BEACON_DISABLED &&
	    beacon != BT_MESH_BEACON_ENABLED) {
		LOG_WRN("Invalid beacon value 0x%02x", beacon);
		return -EINVAL;
	}

	LOG_DBG("0x%02x (%u s)", beacon, 10U * rand_int);

	if (!bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_PRIV_BEACON_STATUS, ctx->addr,
				       (void **)&rsp)) {
		LOG_WRN("Unexpected beacon status from 0x%04x", ctx->addr);
		return -EINVAL;
	}

	rsp->enabled = beacon;
	rsp->rand_interval = rand_int;
	bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);

	return 0;
}

static int handle_gatt_proxy_status(struct bt_mesh_model *mod,
				    struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	uint8_t *rsp;
	uint8_t proxy;

	proxy = net_buf_simple_pull_u8(buf);

	if (proxy != BT_MESH_GATT_PROXY_DISABLED &&
	    proxy != BT_MESH_GATT_PROXY_ENABLED &&
	    proxy != BT_MESH_GATT_PROXY_NOT_SUPPORTED) {
		LOG_WRN("Invalid GATT proxy value 0x%02x", proxy);
		return -EINVAL;
	}

	if (!bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_PRIV_GATT_PROXY_STATUS, ctx->addr,
				       (void **)&rsp)) {
		LOG_WRN("Unexpected proxy status from 0x%04x", ctx->addr);
		return -EINVAL;
	}

	*rsp = proxy;
	bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);

	return 0;
}

static int handle_node_id_status(struct bt_mesh_model *mod,
				 struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	struct bt_mesh_priv_node_id *rsp;
	uint8_t status, node_id;
	uint16_t net_idx;

	status = net_buf_simple_pull_u8(buf);
	net_idx = net_buf_simple_pull_le16(buf);
	node_id = net_buf_simple_pull_u8(buf);

	if (node_id != BT_MESH_NODE_IDENTITY_STOPPED &&
	    node_id != BT_MESH_NODE_IDENTITY_RUNNING &&
	    node_id != BT_MESH_NODE_IDENTITY_NOT_SUPPORTED) {
		LOG_WRN("Invalid node ID value 0x%02x", node_id);
		return -EINVAL;
	}


	if (!bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_PRIV_NODE_ID_STATUS, ctx->addr,
				       (void **)&rsp)) {
		LOG_WRN("Unexpected node ID status from 0x%04x", ctx->addr);
		return -EINVAL;
	}

	rsp->status = status;
	rsp->state = node_id;
	bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);

	return 0;
}

const struct bt_mesh_model_op bt_mesh_priv_beacon_cli_op[] = {
	{ OP_PRIV_BEACON_STATUS, BT_MESH_LEN_EXACT(2), handle_beacon_status },
	{ OP_PRIV_GATT_PROXY_STATUS, BT_MESH_LEN_EXACT(1), handle_gatt_proxy_status },
	{ OP_PRIV_NODE_ID_STATUS, BT_MESH_LEN_EXACT(4), handle_node_id_status },
	BT_MESH_MODEL_OP_END,
};

static int priv_beacon_cli_init(struct bt_mesh_model *mod)
{
	if (!bt_mesh_model_in_primary(mod)) {
		LOG_ERR("Private Beacon Client only allowed in primary element");
		return -EINVAL;
	}

	cli = mod->user_data;
	cli->mod = mod;
	cli->timeout = 2 * MSEC_PER_SEC;
	mod->keys[0] = BT_MESH_KEY_DEV_ANY;
	mod->flags |= BT_MESH_MOD_DEVKEY_ONLY;

	bt_mesh_msg_ack_ctx_init(&cli->ack_ctx);

	return 0;
}

const struct bt_mesh_model_cb bt_mesh_priv_beacon_cli_cb = {
	.init = priv_beacon_cli_init,
};

static int send(struct bt_mesh_priv_beacon_cli *cli, uint16_t net_idx,
		uint16_t addr, struct net_buf_simple *buf)
{
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net_idx,
		.app_idx = BT_MESH_KEY_DEV_REMOTE,
		.addr = addr,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};

	return bt_mesh_model_send(cli->mod, &ctx, buf, NULL, NULL);
}

int bt_mesh_priv_beacon_cli_set(uint16_t net_idx, uint16_t addr, struct bt_mesh_priv_beacon *val)
{
	int err;

	err = bt_mesh_msg_ack_ctx_prepare(&cli->ack_ctx, OP_PRIV_BEACON_STATUS, addr, val);
	if (err) {
		return err;
	}

	BT_MESH_MODEL_BUF_DEFINE(buf, OP_PRIV_BEACON_SET, 2);
	bt_mesh_model_msg_init(&buf, OP_PRIV_BEACON_SET);

	net_buf_simple_add_u8(&buf, val->enabled);
	if (val->rand_interval) {
		net_buf_simple_add_u8(&buf, val->rand_interval);
	}

	err = send(cli, net_idx, addr, &buf);
	if (err) {
		bt_mesh_msg_ack_ctx_clear(&cli->ack_ctx);
		return err;
	}

	return bt_mesh_msg_ack_ctx_wait(&cli->ack_ctx, K_MSEC(cli->timeout));
}

int bt_mesh_priv_beacon_cli_get(uint16_t net_idx, uint16_t addr, struct bt_mesh_priv_beacon *val)
{
	int err;

	err = bt_mesh_msg_ack_ctx_prepare(&cli->ack_ctx, OP_PRIV_BEACON_STATUS, addr, val);
	if (err) {
		return err;
	}

	BT_MESH_MODEL_BUF_DEFINE(buf, OP_PRIV_BEACON_GET, 0);
	bt_mesh_model_msg_init(&buf, OP_PRIV_BEACON_GET);

	err = send(cli, net_idx, addr, &buf);
	if (err) {
		bt_mesh_msg_ack_ctx_clear(&cli->ack_ctx);
		return err;
	}

	return bt_mesh_msg_ack_ctx_wait(&cli->ack_ctx, K_MSEC(cli->timeout));
}

int bt_mesh_priv_beacon_cli_gatt_proxy_set(uint16_t net_idx, uint16_t addr, uint8_t *val)
{
	int err;

	if (!val || (*val != BT_MESH_GATT_PROXY_DISABLED &&
		     *val != BT_MESH_GATT_PROXY_ENABLED)) {
		return -EINVAL;
	}

	err = bt_mesh_msg_ack_ctx_prepare(&cli->ack_ctx, OP_PRIV_GATT_PROXY_STATUS, addr, val);
	if (err) {
		return err;
	}

	BT_MESH_MODEL_BUF_DEFINE(buf, OP_PRIV_GATT_PROXY_SET, 1);
	bt_mesh_model_msg_init(&buf, OP_PRIV_GATT_PROXY_SET);

	net_buf_simple_add_u8(&buf, *val);

	err = send(cli, net_idx, addr, &buf);
	if (err) {
		bt_mesh_msg_ack_ctx_clear(&cli->ack_ctx);
		return err;
	}

	return bt_mesh_msg_ack_ctx_wait(&cli->ack_ctx, K_MSEC(cli->timeout));
}

int bt_mesh_priv_beacon_cli_gatt_proxy_get(uint16_t net_idx, uint16_t addr, uint8_t *val)
{
	int err;

	err = bt_mesh_msg_ack_ctx_prepare(&cli->ack_ctx, OP_PRIV_GATT_PROXY_STATUS, addr, val);
	if (err) {
		return err;
	}

	BT_MESH_MODEL_BUF_DEFINE(buf, OP_PRIV_GATT_PROXY_GET, 0);
	bt_mesh_model_msg_init(&buf, OP_PRIV_GATT_PROXY_GET);

	err = send(cli, net_idx, addr, &buf);
	if (err) {
		bt_mesh_msg_ack_ctx_clear(&cli->ack_ctx);
		return err;
	}

	return bt_mesh_msg_ack_ctx_wait(&cli->ack_ctx, K_MSEC(cli->timeout));
}

int bt_mesh_priv_beacon_cli_node_id_set(uint16_t net_idx, uint16_t addr,
					struct bt_mesh_priv_node_id *val)
{
	int err;

	if (!val || val->net_idx > 0xfff ||
	    (val->state != BT_MESH_NODE_IDENTITY_STOPPED &&
	     val->state != BT_MESH_NODE_IDENTITY_RUNNING)) {
		return -EINVAL;
	}

	err = bt_mesh_msg_ack_ctx_prepare(&cli->ack_ctx, OP_PRIV_NODE_ID_STATUS, addr, val);
	if (err) {
		return err;
	}

	BT_MESH_MODEL_BUF_DEFINE(buf, OP_PRIV_NODE_ID_SET, 3);
	bt_mesh_model_msg_init(&buf, OP_PRIV_NODE_ID_SET);

	net_buf_simple_add_le16(&buf, val->net_idx);
	net_buf_simple_add_u8(&buf, val->state);

	err = send(cli, net_idx, addr, &buf);
	if (err) {
		bt_mesh_msg_ack_ctx_clear(&cli->ack_ctx);
		return err;
	}

	return bt_mesh_msg_ack_ctx_wait(&cli->ack_ctx, K_MSEC(cli->timeout));
}

int bt_mesh_priv_beacon_cli_node_id_get(uint16_t net_idx, uint16_t addr, uint16_t key_net_idx,
					struct bt_mesh_priv_node_id *val)
{
	int err;

	err = bt_mesh_msg_ack_ctx_prepare(&cli->ack_ctx, OP_PRIV_NODE_ID_STATUS, addr, val);
	if (err) {
		return err;
	}

	val->net_idx = key_net_idx;

	BT_MESH_MODEL_BUF_DEFINE(buf, OP_PRIV_NODE_ID_GET, 2);
	bt_mesh_model_msg_init(&buf, OP_PRIV_NODE_ID_GET);

	net_buf_simple_add_le16(&buf, key_net_idx);

	err = send(cli, net_idx, addr, &buf);
	if (err) {
		bt_mesh_msg_ack_ctx_clear(&cli->ack_ctx);
		return err;
	}

	return bt_mesh_msg_ack_ctx_wait(&cli->ack_ctx, K_MSEC(cli->timeout));
}
