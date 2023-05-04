/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @defgroup bt_mesh_priv_beacon_srv Bluetooth Mesh Private Beacon Server
 * @{
 * @brief
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_PRIV_BEACON_SRV_H__
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_PRIV_BEACON_SRV_H__

#include <zephyr/bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 *  @brief Private Beacon Server model composition data entry.
 */
#define BT_MESH_MODEL_PRIV_BEACON_SRV                                          \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_PRIV_BEACON_SRV,                     \
			 bt_mesh_priv_beacon_srv_op, NULL, NULL,               \
			 &bt_mesh_priv_beacon_srv_cb)

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op bt_mesh_priv_beacon_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_priv_beacon_srv_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_PRIV_BEACON_SRV_H__ */

/** @} */
