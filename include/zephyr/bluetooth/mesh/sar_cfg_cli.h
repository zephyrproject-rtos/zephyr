/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief Bluetooth Mesh SAR Configuration Client Model APIs.
 */
#ifndef BT_MESH_SAR_CFG_CLI_H__
#define BT_MESH_SAR_CFG_CLI_H__

#include <zephyr/bluetooth/mesh.h>
#include <zephyr/bluetooth/mesh/sar_cfg.h>

/**
 * @brief Bluetooth Mesh
 * @defgroup bt_mesh_sar_cfg_cli Bluetooth Mesh SAR Configuration Client Model
 * @ingroup bt_mesh
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Mesh SAR Configuration Client Model Context */
struct bt_mesh_sar_cfg_cli {
	/** Access model pointer. */
	struct bt_mesh_model *model;

	/* Publication structure instance */
	struct bt_mesh_model_pub pub;

	/* Synchronous message timeout in milliseconds. */
	int32_t timeout;

	/* Internal parameters for tracking message responses. */
	struct bt_mesh_msg_ack_ctx ack_ctx;
};

/**
 *
 * @brief SAR Configuration Client model composition data entry.
 *
 * @param[in] _cli Pointer to a @ref bt_mesh_sar_cfg_cli instance.
 */
#define BT_MESH_MODEL_SAR_CFG_CLI(_cli)                                  \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_SAR_CFG_CLI,                         \
			 _bt_mesh_sar_cfg_cli_op, _cli.pub, _cli,                  \
			 &_bt_mesh_sar_cfg_cli_cb)

/** @brief Get the SAR Transmitter state of the target node.
 *
 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param rsp     Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_sar_cfg_cli_transmitter_get(uint16_t net_idx, uint16_t addr,
					struct bt_mesh_sar_tx *rsp);

/** @brief Set the SAR Transmitter state of the target node.
 *
 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param set     New SAR Transmitter state to set on the target node.
 *  @param rsp     Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_sar_cfg_cli_transmitter_set(uint16_t net_idx, uint16_t addr,
					const struct bt_mesh_sar_tx *set,
					struct bt_mesh_sar_tx *rsp);

/** @brief Get the SAR Receiver state of the target node.
 *
 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param rsp     Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_sar_cfg_cli_receiver_get(uint16_t net_idx, uint16_t addr,
				     struct bt_mesh_sar_rx *rsp);

/** @brief Set the SAR Receiver state of the target node.
 *
 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param set     New SAR Receiver state to set on the target node.
 *  @param rsp     Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_sar_cfg_cli_receiver_set(uint16_t net_idx, uint16_t addr,
				     const struct bt_mesh_sar_rx *set,
				     struct bt_mesh_sar_rx *rsp);

/** @brief Get the current transmission timeout value.
 *
 *  @return The configured transmission timeout in milliseconds.
 */
int32_t bt_mesh_sar_cfg_cli_timeout_get(void);

/** @brief Set the transmission timeout value.
 *
 *  @param timeout The new transmission timeout.
 */
void bt_mesh_sar_cfg_cli_timeout_set(int32_t timeout);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_sar_cfg_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_sar_cfg_cli_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_SAR_CFG_CLI_H__ */

/** @} */
