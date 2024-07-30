/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BT_MESH_RPR_SRV_H__
#define ZEPHYR_INCLUDE_BT_MESH_RPR_SRV_H__

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/mesh/access.h>
#include <zephyr/bluetooth/mesh/rpr.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup bt_mesh_rpr_srv Remote provisioning server
 * @ingroup bt_mesh
 * @{
 */

/**
 *
 * @brief Remote Provisioning Server model composition data entry.
 */
#define BT_MESH_MODEL_RPR_SRV                                                  \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_REMOTE_PROV_SRV,                     \
			 _bt_mesh_rpr_srv_op, NULL, NULL,                      \
			 &_bt_mesh_rpr_srv_cb)

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_rpr_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_rpr_srv_cb;
/** @endcond */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BT_MESH_RPR_SRV_H__ */
