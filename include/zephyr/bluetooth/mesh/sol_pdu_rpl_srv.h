/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for the Bluetooth Mesh Solicitation PDU RPL Configuration Server model API.
 * @ingroup bt_mesh_sol_pdu_rpl_srv
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_SOL_PDU_RPL_SRV_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_SOL_PDU_RPL_SRV_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup bt_mesh_sol_pdu_rpl_srv Bluetooth Mesh Solicitation PDU RPL Server
 * @ingroup bt_mesh
 * @{
 */

/**
 *  @brief Solicitation PDU RPL Server model composition data entry.
 */
#define BT_MESH_MODEL_SOL_PDU_RPL_SRV                                          \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_SOL_PDU_RPL_SRV,                     \
			 _bt_mesh_sol_pdu_rpl_srv_op, NULL, NULL,              \
			 &_bt_mesh_sol_pdu_rpl_srv_cb)

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_sol_pdu_rpl_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_sol_pdu_rpl_srv_cb;
/** @endcond */

/** @} */

#ifdef __cplusplus
}
#endif
#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_SOL_PDU_RPL_SRV_H_ */
