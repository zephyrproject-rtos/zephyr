/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/mesh.h>
#include "brg_cfg.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_brg_cfg_srv);

const struct bt_mesh_model_op _bt_mesh_brg_cfg_srv_op[] = {
	BT_MESH_MODEL_OP_END,
};

static int brg_cfg_srv_init(const struct bt_mesh_model *model)
{
	const struct bt_mesh_model *config_srv =
		bt_mesh_model_find(bt_mesh_model_elem(model), BT_MESH_MODEL_ID_CFG_SRV);

	if (config_srv == NULL) {
		LOG_ERR("Bridge Configuration Server only allowed in primary element");
		return -EINVAL;
	}


	bt_mesh_model_extend(model, config_srv);

	return 0;
}

void brg_cfg_srv_reset(const struct bt_mesh_model *model)
{
	bt_mesh_brg_cfg_tbl_reset();
}

const struct bt_mesh_model_cb _bt_mesh_brg_cfg_srv_cb = {
	.init = brg_cfg_srv_init,
	.reset = brg_cfg_srv_reset,
};
