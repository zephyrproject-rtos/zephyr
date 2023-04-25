/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/mesh.h>
#include "net.h"
#include "foundation.h"
#include "access.h"
#include "msg.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_priv_beacon_cli);

static struct bt_mesh_priv_beacon_cli *cli;

static int32_t msg_timeout;

static int handle_beacon_status(const struct bt_mesh_model *model,
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

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_PRIV_BEACON_STATUS, ctx->addr,
				      (void **)&rsp)) {
		rsp->enabled = beacon;
		rsp->rand_interval = rand_int;

		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->cb && cli->cb->priv_beacon_status) {
		struct bt_mesh_priv_beacon state = {
			.enabled = beacon,
			.rand_interval = rand_int,
		};

		cli->cb->priv_beacon_status(cli, ctx->addr, &state);
	}

	return 0;
}

static int handle_gatt_proxy_status(const struct bt_mesh_model *model,
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

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_PRIV_GATT_PROXY_STATUS, ctx->addr,
				      (void **)&rsp)) {
		*rsp = proxy;

		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->cb && cli->cb->priv_gatt_proxy_status) {
		cli->cb->priv_gatt_proxy_status(cli, ctx->addr, proxy);
	}

	return 0;
}

static int handle_node_id_status(const struct bt_mesh_model *model,
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

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_PRIV_NODE_ID_STATUS, ctx->addr,
				      (void **)&rsp)) {
		rsp->net_idx = net_idx;
		rsp->status = status;
		rsp->state = node_id;

		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->cb && cli->cb->priv_node_id_status) {
		struct bt_mesh_priv_node_id state = {
			.net_idx = net_idx,
			.status = status,
			.state = node_id,
		};

		cli->cb->priv_node_id_status(cli, ctx->addr, &state);
	}

	return 0;
}

const struct bt_mesh_model_op bt_mesh_priv_beacon_cli_op[] = {
	{ OP_PRIV_BEACON_STATUS, BT_MESH_LEN_EXACT(2), handle_beacon_status },
	{ OP_PRIV_GATT_PROXY_STATUS, BT_MESH_LEN_EXACT(1), handle_gatt_proxy_status },
	{ OP_PRIV_NODE_ID_STATUS, BT_MESH_LEN_EXACT(4), handle_node_id_status },
	BT_MESH_MODEL_OP_END,
};

static int priv_beacon_cli_init(const struct bt_mesh_model *model)
{
	if (!bt_mesh_model_in_primary(model)) {
		LOG_ERR("Private Beacon Client only allowed in primary element");
		return -EINVAL;
	}

	cli = model->user_data;
	cli->model = model;
	msg_timeout = 2 * MSEC_PER_SEC;
	model->keys[0] = BT_MESH_KEY_DEV_ANY;
	model->ctx->flags |= BT_MESH_MOD_DEVKEY_ONLY;

	bt_mesh_msg_ack_ctx_init(&cli->ack_ctx);

	return 0;
}

const struct bt_mesh_model_cb bt_mesh_priv_beacon_cli_cb = {
	.init = priv_beacon_cli_init,
};

int bt_mesh_priv_beacon_cli_set(uint16_t net_idx, uint16_t addr, struct bt_mesh_priv_beacon *val)
{
	struct bt_mesh_msg_ctx ctx = BT_MESH_MSG_CTX_INIT_DEV(net_idx, addr);
	const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = OP_PRIV_BEACON_STATUS,
		.user_data = val,
		.timeout = msg_timeout,
	};

	BT_MESH_MODEL_BUF_DEFINE(buf, OP_PRIV_BEACON_SET, 2);
	bt_mesh_model_msg_init(&buf, OP_PRIV_BEACON_SET);

	net_buf_simple_add_u8(&buf, val->enabled);
	if (val->rand_interval) {
		net_buf_simple_add_u8(&buf, val->rand_interval);
	}

	return bt_mesh_msg_ackd_send(cli->model, &ctx, &buf, val ? &rsp_ctx : NULL);
}

int bt_mesh_priv_beacon_cli_get(uint16_t net_idx, uint16_t addr, struct bt_mesh_priv_beacon *val)
{
	struct bt_mesh_msg_ctx ctx = BT_MESH_MSG_CTX_INIT_DEV(net_idx, addr);
	const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = OP_PRIV_BEACON_STATUS,
		.user_data = val,
		.timeout = msg_timeout,
	};

	BT_MESH_MODEL_BUF_DEFINE(buf, OP_PRIV_BEACON_GET, 0);
	bt_mesh_model_msg_init(&buf, OP_PRIV_BEACON_GET);

	return bt_mesh_msg_ackd_send(cli->model, &ctx, &buf, val ? &rsp_ctx : NULL);
}

int bt_mesh_priv_beacon_cli_gatt_proxy_set(uint16_t net_idx, uint16_t addr, uint8_t *val)
{
	struct bt_mesh_msg_ctx ctx = BT_MESH_MSG_CTX_INIT_DEV(net_idx, addr);
	const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = OP_PRIV_GATT_PROXY_STATUS,
		.user_data = val,
		.timeout = msg_timeout,
	};

	if (!val || (*val != BT_MESH_GATT_PROXY_DISABLED &&
		     *val != BT_MESH_GATT_PROXY_ENABLED)) {
		return -EINVAL;
	}

	BT_MESH_MODEL_BUF_DEFINE(buf, OP_PRIV_GATT_PROXY_SET, 1);
	bt_mesh_model_msg_init(&buf, OP_PRIV_GATT_PROXY_SET);

	net_buf_simple_add_u8(&buf, *val);

	return bt_mesh_msg_ackd_send(cli->model, &ctx, &buf, val ? &rsp_ctx : NULL);
}

int bt_mesh_priv_beacon_cli_gatt_proxy_get(uint16_t net_idx, uint16_t addr, uint8_t *val)
{
	struct bt_mesh_msg_ctx ctx = BT_MESH_MSG_CTX_INIT_DEV(net_idx, addr);
	const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = OP_PRIV_GATT_PROXY_STATUS,
		.user_data = val,
		.timeout = msg_timeout,
	};

	BT_MESH_MODEL_BUF_DEFINE(buf, OP_PRIV_GATT_PROXY_GET, 0);
	bt_mesh_model_msg_init(&buf, OP_PRIV_GATT_PROXY_GET);

	return bt_mesh_msg_ackd_send(cli->model, &ctx, &buf, val ? &rsp_ctx : NULL);
}

int bt_mesh_priv_beacon_cli_node_id_set(uint16_t net_idx, uint16_t addr,
					struct bt_mesh_priv_node_id *val)
{
	struct bt_mesh_msg_ctx ctx = BT_MESH_MSG_CTX_INIT_DEV(net_idx, addr);
	const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = OP_PRIV_NODE_ID_STATUS,
		.user_data = val,
		.timeout = msg_timeout,
	};

	if (!val || val->net_idx > 0xfff ||
	    (val->state != BT_MESH_NODE_IDENTITY_STOPPED &&
	     val->state != BT_MESH_NODE_IDENTITY_RUNNING)) {
		return -EINVAL;
	}

	BT_MESH_MODEL_BUF_DEFINE(buf, OP_PRIV_NODE_ID_SET, 3);
	bt_mesh_model_msg_init(&buf, OP_PRIV_NODE_ID_SET);

	net_buf_simple_add_le16(&buf, val->net_idx);
	net_buf_simple_add_u8(&buf, val->state);

	return bt_mesh_msg_ackd_send(cli->model, &ctx, &buf, val ? &rsp_ctx : NULL);
}

int bt_mesh_priv_beacon_cli_node_id_get(uint16_t net_idx, uint16_t addr, uint16_t key_net_idx,
					struct bt_mesh_priv_node_id *val)
{
	struct bt_mesh_msg_ctx ctx = BT_MESH_MSG_CTX_INIT_DEV(net_idx, addr);
	const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = OP_PRIV_NODE_ID_STATUS,
		.user_data = val,
		.timeout = msg_timeout,
	};

	BT_MESH_MODEL_BUF_DEFINE(buf, OP_PRIV_NODE_ID_GET, 2);
	bt_mesh_model_msg_init(&buf, OP_PRIV_NODE_ID_GET);

	net_buf_simple_add_le16(&buf, key_net_idx);

	return bt_mesh_msg_ackd_send(cli->model, &ctx, &buf, val ? &rsp_ctx : NULL);
}
