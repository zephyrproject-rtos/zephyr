/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/mesh.h>

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_brg_cfg_cli);

const struct bt_mesh_model_op _bt_mesh_brg_cfg_cli_op[] = {
	BT_MESH_MODEL_OP_END,
};

static int brg_cfg_cli_init(const struct bt_mesh_model *model)
{
	if (!bt_mesh_model_in_primary(model)) {
		LOG_ERR("Bridge Configuration Client only allowed in primary element");
		return -EINVAL;
	}

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_brg_cfg_cli_cb = {
	.init = brg_cfg_cli_init,
};

int bt_mesh_brg_cfg_cli_subnet_bridge_get(uint16_t net_idx, uint16_t addr,
					  enum bt_mesh_subnet_bridge_state *status)
{
	return 0;
}

int bt_mesh_brg_cfg_cli_subnet_bridge_set(uint16_t net_idx, uint16_t addr,
					  enum bt_mesh_subnet_bridge_state val,
					  enum bt_mesh_subnet_bridge_state *status)
{
	return 0;
}

int bt_mesh_brg_cfg_cli_bridging_table_size_get(uint16_t net_idx, uint16_t addr, uint16_t *size)
{
	return 0;
}

int bt_mesh_brg_cfg_cli_bridging_table_add(uint16_t net_idx, uint16_t addr, uint8_t directions,
					   uint16_t net_idx1, uint16_t net_idx2,
					   uint16_t addr1, uint16_t addr2,
					   struct bt_mesh_bridging_table_status *rsp)
{
	return 0;
}

int bt_mesh_brg_cfg_cli_bridging_table_remove(uint16_t net_idx, uint16_t addr, uint16_t net_idx1,
					      uint16_t net_idx2, uint16_t addr1, uint16_t addr2,
					      struct bt_mesh_bridging_table_status *rsp)
{
	return 0;
}

int bt_mesh_brg_cfg_cli_bridged_subnets_get(uint16_t net_idx, uint16_t addr,
					    struct bt_mesh_filter_netkey filter_net_idx,
					    uint8_t start_idx,
					    struct bt_mesh_bridged_subnets_list *rsp)
{
	return 0;
}

int bt_mesh_brg_cfg_cli_bridging_table_get(uint16_t net_idx, uint16_t addr, uint16_t net_idx1,
					   uint16_t net_idx2, uint16_t start_idx,
					   struct bt_mesh_bridging_table_list *rsp)
{
	return 0;
}
