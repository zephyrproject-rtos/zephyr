/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_SHELL_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_SHELL_H_

#include <zephyr/bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of faults the health server can have. */
#define BT_MESH_SHELL_CUR_FAULTS_MAX 4

/**
 *  A helper to define a health publication context for shell with the shell's
 *  maximum number of faults the element can have.
 *
 *  @param _name Name given to the publication context variable.
 */
#define BT_MESH_SHELL_HEALTH_PUB_DEFINE(_name)                                 \
		BT_MESH_HEALTH_PUB_DEFINE(_name,                               \
					  BT_MESH_SHELL_CUR_FAULTS_MAX);

/** Target context for the mesh shell */
struct bt_mesh_shell_target {
	/* Current destination address */
	uint16_t dst;
	/* Current net index */
	uint16_t net_idx;
	/* Current app index */
	uint16_t app_idx;
};

/** @brief External reference to health server */
extern struct bt_mesh_health_srv bt_mesh_shell_health_srv;

/** @brief External reference to health client */
extern struct bt_mesh_health_cli bt_mesh_shell_health_cli;

/** @brief External reference to Firmware Update Server */
extern struct bt_mesh_dfu_srv bt_mesh_shell_dfu_srv;

/** @brief External reference to Firmware Update Client */
extern struct bt_mesh_dfu_cli bt_mesh_shell_dfu_cli;

/** @brief External reference to BLOB Transfer Server */
extern struct bt_mesh_blob_srv bt_mesh_shell_blob_srv;

/** @brief External reference to BLOB Transfer Client */
extern struct bt_mesh_blob_cli bt_mesh_shell_blob_cli;

/** @brief External reference to Remote Provisioning Client */
extern struct bt_mesh_rpr_cli bt_mesh_shell_rpr_cli;

/** @brief External reference to provisioning handler. */
extern struct bt_mesh_prov bt_mesh_shell_prov;

/** @brief External reference to shell target context. */
extern struct bt_mesh_shell_target bt_mesh_shell_target_ctx;

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_SHELL_H_ */
