/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/mesh.h>

#include "access.h"
#include "cfg.h"
#include "foundation.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_od_priv_proxy_srv);

static int proxy_status_rsp(const struct bt_mesh_model *mod,
			    struct bt_mesh_msg_ctx *ctx)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, OP_OD_PRIV_PROXY_STATUS, 1);
	bt_mesh_model_msg_init(&buf, OP_OD_PRIV_PROXY_STATUS);

	net_buf_simple_add_u8(&buf, bt_mesh_od_priv_proxy_get());

	bt_mesh_model_send(mod, ctx, &buf, NULL, NULL);

	return 0;
}

static int handle_proxy_get(const struct bt_mesh_model *mod,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	LOG_DBG("");

	proxy_status_rsp(mod, ctx);

	return 0;
}

static int handle_proxy_set(const struct bt_mesh_model *mod,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	uint8_t state;

	LOG_DBG("");

	state = net_buf_simple_pull_u8(buf);
	LOG_DBG("state %d", state);

	bt_mesh_od_priv_proxy_set(state);
	proxy_status_rsp(mod, ctx);

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_od_priv_proxy_srv_op[] = {
	{ OP_OD_PRIV_PROXY_GET, BT_MESH_LEN_EXACT(0), handle_proxy_get },
	{ OP_OD_PRIV_PROXY_SET, BT_MESH_LEN_EXACT(1), handle_proxy_set },

	BT_MESH_MODEL_OP_END
};

static int od_priv_proxy_srv_init(const struct bt_mesh_model *mod)
{
	const struct bt_mesh_model *priv_beacon_srv = bt_mesh_model_find(
		bt_mesh_model_elem(mod), BT_MESH_MODEL_ID_PRIV_BEACON_SRV);
	const struct bt_mesh_model *sol_pdu_rpl_srv = bt_mesh_model_find(
		bt_mesh_model_elem(mod), BT_MESH_MODEL_ID_SOL_PDU_RPL_SRV);

	if (priv_beacon_srv == NULL) {
		return -EINVAL;
	}

	if (!bt_mesh_model_in_primary(mod)) {
		LOG_ERR("On-Demand Private Proxy server not in primary element");
		return -EINVAL;
	}

	mod->keys[0] = BT_MESH_KEY_DEV_LOCAL;
	mod->ctx->flags |= BT_MESH_MOD_DEVKEY_ONLY;

	if (IS_ENABLED(CONFIG_BT_MESH_MODEL_EXTENSIONS)) {
		bt_mesh_model_extend(mod, priv_beacon_srv);
		bt_mesh_model_correspond(mod, sol_pdu_rpl_srv);
	}

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_od_priv_proxy_srv_cb = {
	.init = od_priv_proxy_srv_init,
};
