/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @defgroup bt_mesh_op_agg_srv Opcodes Aggregator Server model
 * @{
 * @brief API for the Opcodes Aggregator Server model.
 */
#ifndef BT_MESH_OP_AGG_SRV_H__
#define BT_MESH_OP_AGG_SRV_H__

#include <zephyr/bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 *  @brief Opcodes Aggretator Server model composition data entry.
 *
 *  @note The Opcodes Aggregator Server handles aggregated messages
 *        and dispatches them to the respective models and their message
 *        handlers. Current implementation assumes that responses are sent
 *        from the same execution context as the received message and
 *        doesn't allow to send a postponed response, e.g. from workqueue.
 */
#define BT_MESH_MODEL_OP_AGG_SRV                                               \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_OP_AGG_SRV, _bt_mesh_op_agg_srv_op,  \
			 NULL, NULL, &_bt_mesh_op_agg_srv_cb)

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_op_agg_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_op_agg_srv_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_OP_AGG_SRV_H__ */

/**
 * @}
 */
