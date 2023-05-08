/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @defgroup bt_mesh_large_comp_data_srv Large Composition Data Server model
 * @{
 * @brief API for the Large Composition Data Server model.
 */
#ifndef BT_MESH_LARGE_COMP_DATA_SRV_H__
#define BT_MESH_LARGE_COMP_DATA_SRV_H__

#include <zephyr/bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 *  @brief Large Composition Data Server model composition data entry.
 */
#define BT_MESH_MODEL_LARGE_COMP_DATA_SRV                                      \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LARGE_COMP_DATA_SRV,                 \
			 _bt_mesh_large_comp_data_srv_op, NULL, NULL,          \
			 &_bt_mesh_large_comp_data_srv_cb)

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_large_comp_data_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_large_comp_data_srv_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_LARGE_COMP_DATA_SRV_H__ */

/**
 * @}
 */
