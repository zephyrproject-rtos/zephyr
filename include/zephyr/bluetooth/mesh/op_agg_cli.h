/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @defgroup bt_mesh_op_agg_cli Opcodes Aggregator Client model
 * @{
 * @brief API for the Opcodes Aggregator Client model.
 */
#ifndef BT_MESH_OP_AGG_CLI_H__
#define BT_MESH_OP_AGG_CLI_H__

#include <zephyr/bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 *  @brief Opcodes Aggregator Client model composition data entry.
 */
#define BT_MESH_MODEL_OP_AGG_CLI                                               \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_OP_AGG_CLI, _bt_mesh_op_agg_cli_op,  \
			 NULL, NULL, &_bt_mesh_op_agg_cli_cb)

/** @brief Configure Opcodes Aggregator context.
 *
 *  @param net_idx           NetKey index to encrypt with.
 *  @param app_idx           AppKey index to encrypt with.
 *  @param dst               Target Opcodes Aggregator Server address.
 *  @param elem_addr         Target node element address for the sequence message.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_op_agg_cli_seq_start(uint16_t net_idx, uint16_t app_idx, uint16_t dst,
				 uint16_t elem_addr);

/** @brief Opcodes Aggregator message send.
 *
 *  Uses previously configured context and sends aggregated message
 *  to target node.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_op_agg_cli_seq_send(void);

/** @brief Abort Opcodes Aggregator context.
 */
void bt_mesh_op_agg_cli_seq_abort(void);

/** @brief Check if Opcodes Aggregator Sequence context is started.
 *
 *  @return true if it is started, otherwise false.
 */
bool bt_mesh_op_agg_cli_seq_is_started(void);

/** @brief Get Opcodes Aggregator context tailroom.
 *
 *  @return Remaning tailroom of Opcodes Aggregator SDU.
 */
size_t bt_mesh_op_agg_cli_seq_tailroom(void);

/** @brief Get the current transmission timeout value.
 *
 *  @return The configured transmission timeout in milliseconds.
 */
int32_t bt_mesh_op_agg_cli_timeout_get(void);

/** @brief Set the transmission timeout value.
 *
 *  @param timeout The new transmission timeout.
 */
void bt_mesh_op_agg_cli_timeout_set(int32_t timeout);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_op_agg_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_op_agg_cli_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_OP_AGG_CLI_H__ */

/**
 * @}
 */
