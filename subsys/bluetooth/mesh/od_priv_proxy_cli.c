/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/mesh.h>

#include "cfg.h"
#include "access.h"
#include "foundation.h"
#include "msg.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_od_priv_proxy_cli);

/** On-Demand Private Proxy Client Model Context */
static struct bt_mesh_od_priv_proxy_cli *cli;

static int32_t msg_timeout;

static int handle_proxy_status(struct bt_mesh_model *mod,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	uint8_t *state;
	uint8_t state_rsp;

	state_rsp = net_buf_simple_pull_u8(buf);

	LOG_DBG("On-Demand Private Proxy status received: state: %u", state_rsp);

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_OD_PRIV_PROXY_STATUS,
				       ctx->addr, (void **)&state)) {

		if (state) {
			*state = state_rsp;
		}
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}


	if (cli->od_status) {
		cli->od_status(cli, ctx->addr, state_rsp);
	}

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_od_priv_proxy_cli_op[] = {
	{ OP_OD_PRIV_PROXY_STATUS, BT_MESH_LEN_EXACT(1), handle_proxy_status },

	BT_MESH_MODEL_OP_END
};

int bt_mesh_od_priv_proxy_cli_get(uint16_t net_idx, uint16_t addr, uint8_t *val_rsp)
{
	struct bt_mesh_msg_ctx ctx = BT_MESH_MSG_CTX_INIT_DEV(net_idx, addr);
	const struct bt_mesh_msg_rsp_ctx rsp = {
		.ack = &cli->ack_ctx,
		.op = OP_OD_PRIV_PROXY_STATUS,
		.user_data = val_rsp,
		.timeout = msg_timeout,
	};

	BT_MESH_MODEL_BUF_DEFINE(msg, OP_OD_PRIV_PROXY_GET, 0);
	bt_mesh_model_msg_init(&msg, OP_OD_PRIV_PROXY_GET);

	return bt_mesh_msg_ackd_send(cli->model, &ctx, &msg, val_rsp ? &rsp : NULL);
}

int bt_mesh_od_priv_proxy_cli_set(uint16_t net_idx, uint16_t addr, uint8_t val, uint8_t *val_rsp)
{
	struct bt_mesh_msg_ctx ctx = BT_MESH_MSG_CTX_INIT_DEV(net_idx, addr);
	const struct bt_mesh_msg_rsp_ctx rsp = {
		.ack = &cli->ack_ctx,
		.op = OP_OD_PRIV_PROXY_STATUS,
		.user_data = val_rsp,
		.timeout = msg_timeout,
	};

	BT_MESH_MODEL_BUF_DEFINE(msg, OP_OD_PRIV_PROXY_SET, 1);
	bt_mesh_model_msg_init(&msg, OP_OD_PRIV_PROXY_SET);

	net_buf_simple_add_u8(&msg, val);

	return bt_mesh_msg_ackd_send(cli->model, &ctx, &msg, val_rsp ? &rsp : NULL);
}

void bt_mesh_od_priv_proxy_cli_timeout_set(int32_t timeout)
{
	msg_timeout = timeout;
}

static int on_demand_proxy_cli_init(struct bt_mesh_model *mod)
{
	if (!bt_mesh_model_in_primary(mod)) {
		LOG_ERR("On-Demand Private Proxy client not in primary element");
		return -EINVAL;
	}

	cli = mod->user_data;
	cli->model = mod;
	mod->keys[0] = BT_MESH_KEY_DEV_ANY;
	mod->flags |= BT_MESH_MOD_DEVKEY_ONLY;
	msg_timeout = CONFIG_BT_MESH_OD_PRIV_PROXY_CLI_TIMEOUT;

	bt_mesh_msg_ack_ctx_init(&cli->ack_ctx);
	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_od_priv_proxy_cli_cb = {
	.init = on_demand_proxy_cli_init,
};
