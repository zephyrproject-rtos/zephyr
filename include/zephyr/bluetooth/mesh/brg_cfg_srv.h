/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief Bluetooth Mesh Bridge Configuration Server Model APIs.
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_BRG_CFG_SRV_H__
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_BRG_CFG_SRV_H__

#include <zephyr/bluetooth/mesh/brg_cfg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup bt_mesh_brg_cfg_srv Bridge Configuration Server Model
 * @ingroup bt_mesh
 * @{
 *  @brief API for the Bluetooth Mesh Bridge Configuration Server model
 */

/**
 *
 *  @brief Bridge Configuration Server model Composition Data entry.
 */
#define BT_MESH_MODEL_BRG_CFG_SRV                                                                  \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_BRG_CFG_SRV, _bt_mesh_brg_cfg_srv_op, NULL, NULL,        \
			 &_bt_mesh_brg_cfg_srv_cb)

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_brg_cfg_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_brg_cfg_srv_cb;
/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_BRG_CFG_SRV_H__ */
