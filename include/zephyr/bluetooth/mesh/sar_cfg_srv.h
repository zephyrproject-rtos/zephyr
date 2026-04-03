/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief Bluetooth Mesh SAR Configuration Server Model APIs.
 */
#ifndef BT_MESH_SAR_CFG_SRV_H__
#define BT_MESH_SAR_CFG_SRV_H__

#include <zephyr/bluetooth/mesh.h>
#include <zephyr/bluetooth/mesh/sar_cfg.h>

/**
 * @brief Bluetooth Mesh
 * @defgroup bt_mesh_sar_cfg_srv Bluetooth Mesh SAR Configuration Server Model
 * @ingroup bt_mesh
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 *  @brief Transport SAR Configuration Server model composition data entry.
 */
#define BT_MESH_MODEL_SAR_CFG_SRV                                              \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_SAR_CFG_SRV, bt_mesh_sar_cfg_srv_op, \
			 NULL, NULL, &bt_mesh_sar_cfg_srv_cb)

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op bt_mesh_sar_cfg_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_sar_cfg_srv_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_SAR_CFG_SRV_H__ */

/**
 * @}
 */
