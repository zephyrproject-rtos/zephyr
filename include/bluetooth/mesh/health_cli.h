/** @file
 *  @brief Bluetooth Mesh Health Client Model APIs.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_MESH_HEALTH_CLI_H
#define __BT_MESH_HEALTH_CLI_H

/**
 * @brief Bluetooth Mesh
 * @defgroup bt_mesh_health_cli Bluetooth Mesh Health Client Model
 * @ingroup bt_mesh
 * @{
 */

/** Mesh Health Client Model Context */
struct bt_mesh_health_cli {
	struct bt_mesh_model *model;

	struct k_sem          op_sync;
	u32_t                 op_pending;
	void                 *op_param;
};

extern const struct bt_mesh_model_op bt_mesh_health_cli_op[];

#define BT_MESH_MODEL_HEALTH_CLI(cli_data)                                   \
		BT_MESH_MODEL(BT_MESH_MODEL_ID_HEALTH_CLI,                   \
			      bt_mesh_health_cli_op, NULL, cli_data)

int bt_mesh_health_cli_set(struct bt_mesh_model *model);

s32_t bt_mesh_health_cli_timeout_get(void);
void bt_mesh_health_cli_timeout_set(s32_t timeout);

/**
 * @}
 */

#endif /* __BT_MESH_HEALTH_CLI_H */
