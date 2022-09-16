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

/** @def BT_MESH_SHELL_HEALTH_PUB_DEFINE
 *
 *  A helper to define a health publication context for shell with the shell's
 *  maximum number of faults the element can have.
 *
 *  @param _name Name given to the publication context variable.
 */
#define BT_MESH_SHELL_HEALTH_PUB_DEFINE(_name)                                 \
		BT_MESH_HEALTH_PUB_DEFINE(_name,                               \
					  BT_MESH_SHELL_CUR_FAULTS_MAX);

/** @brief External reference to health server */
extern struct bt_mesh_health_srv bt_mesh_shell_health_srv;

/** @brief External reference to health client */
extern struct bt_mesh_health_cli bt_mesh_shell_health_cli;

/** @brief External reference to provisioning handler. */
extern struct bt_mesh_prov bt_mesh_shell_prov;

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_SHELL_H_ */
