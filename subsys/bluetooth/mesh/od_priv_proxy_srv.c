/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/mesh.h>

#include "access.h"
#include "cfg.h"
#include "foundation.h"
#include "settings.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_od_priv_proxy_srv);


static const struct bt_mesh_model *od_priv_proxy_srv;
static uint8_t on_demand_state;

static int od_priv_proxy_store(bool delete)
{
	if (!IS_ENABLED(CONFIG_BT_SETTINGS)) {
		return 0;
	}

	const void *data = delete ? NULL : &on_demand_state;
	size_t len = delete ? 0 : sizeof(uint8_t);

	return bt_mesh_model_data_store(od_priv_proxy_srv, false, "pp", data, len);
}

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
	od_priv_proxy_srv = mod;

	const struct bt_mesh_model *priv_beacon_srv =
		bt_mesh_model_find(bt_mesh_model_elem(mod), BT_MESH_MODEL_ID_PRIV_BEACON_SRV);
	const struct bt_mesh_model *sol_pdu_rpl_srv =
		bt_mesh_model_find(bt_mesh_model_elem(mod), BT_MESH_MODEL_ID_SOL_PDU_RPL_SRV);

	if (priv_beacon_srv == NULL) {
		LOG_ERR("On-Demand Private Proxy server cannot extend Private Beacon server");
		return -EINVAL;
	}

	mod->keys[0] = BT_MESH_KEY_DEV_LOCAL;
	mod->rt->flags |= BT_MESH_MOD_DEVKEY_ONLY;

	bt_mesh_model_extend(mod, priv_beacon_srv);

	if (sol_pdu_rpl_srv != NULL) {
		bt_mesh_model_correspond(mod, sol_pdu_rpl_srv);
	} else {
		LOG_WRN("On-Demand Private Proxy server cannot be corresponded by Solicitation PDU "
			"RPL Configuration server");
	}

	return 0;
}

static void od_priv_proxy_srv_reset(const struct bt_mesh_model *model)
{
	on_demand_state = 0;
	od_priv_proxy_store(true);
}

#ifdef CONFIG_BT_SETTINGS
static int od_priv_proxy_srv_settings_set(const struct bt_mesh_model *model, const char *name,
					  size_t len_rd, settings_read_cb read_cb, void *cb_data)
{
	int err;

	if (len_rd == 0) {
		LOG_DBG("Cleared configuration state");
		return 0;
	}

	err = bt_mesh_settings_set(read_cb, cb_data, &on_demand_state, sizeof(uint8_t));
	if (err) {
		LOG_ERR("Failed to set OD private proxy state");
		return err;
	}

	bt_mesh_od_priv_proxy_set(on_demand_state);
	return 0;
}

static void od_priv_proxy_srv_pending_store(const struct bt_mesh_model *model)
{
	on_demand_state = bt_mesh_od_priv_proxy_get();
	od_priv_proxy_store(false);
}
#endif

const struct bt_mesh_model_cb _bt_mesh_od_priv_proxy_srv_cb = {
	.init = od_priv_proxy_srv_init,
	.reset = od_priv_proxy_srv_reset,
#ifdef CONFIG_BT_SETTINGS
	.settings_set = od_priv_proxy_srv_settings_set,
	.pending_store = od_priv_proxy_srv_pending_store,
#endif
};

void bt_mesh_od_priv_proxy_srv_store_schedule(void)
{
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_model_data_store_schedule(od_priv_proxy_srv);
	}
}
