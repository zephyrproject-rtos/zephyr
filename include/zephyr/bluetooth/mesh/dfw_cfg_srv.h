/*
 * Copyright (c) 2023 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BT_MESH_DFW_CFG_SRV_H__
#define BT_MESH_DFW_CFG_SRV_H__

#include <zephyr/bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup bt_mesh_dfw_cfg_srv Directed Forwarding Configuration Server model
 * @ingroup bt_mesh
 * @{
 */

/**
 *
 *  @brief Directed Forwarding Configuration Server model composition data entry.
 */
#define BT_MESH_MODEL_DFW_CFG_SRV                                      \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_DIRECTED_CFG_SRV,	       \
			 bt_mesh_dfw_cfg_srv_op, NULL, NULL,          \
			 &bt_mesh_dfw_cfg_srv_cb)

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op bt_mesh_dfw_cfg_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_dfw_cfg_srv_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_DFW_CFG_SRV_H__ */

/**
 * @}
 */
