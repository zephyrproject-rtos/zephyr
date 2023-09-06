/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BT_MESH_OD_PRIV_PROXY_CLI_H__
#define BT_MESH_OD_PRIV_PROXY_CLI_H__

#include <zephyr/bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup bt_mesh_od_priv_proxy_cli Bluetooth Mesh On-Demand Private GATT Proxy Client
 * @ingroup bt_mesh
 * @{
 */

/** On-Demand Private Proxy Client Model Context */
struct bt_mesh_od_priv_proxy_cli {
	/** Solicitation PDU RPL model entry pointer. */
	struct bt_mesh_model *model;

	/* Internal parameters for tracking message responses. */
	struct bt_mesh_msg_ack_ctx ack_ctx;

	/** @brief Optional callback for On-Demand Private Proxy Status messages.
	 *
	 *  Handles received On-Demand Private Proxy Status messages from a On-Demand Private Proxy
	 *  server.The @c state param represents state of On-Demand Private Proxy server.
	 *
	 *  @param cli         On-Demand Private Proxy client that received the status message.
	 *  @param addr        Address of the sender.
	 *  @param state       State value.
	 */
	void (*od_status)(struct bt_mesh_od_priv_proxy_cli *cli, uint16_t addr, uint8_t state);
};

/**
 *  @brief On-Demand Private Proxy Client model composition data entry.
 */
#define BT_MESH_MODEL_OD_PRIV_PROXY_CLI(cli_data)                                  \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_ON_DEMAND_PROXY_CLI,                     \
			 _bt_mesh_od_priv_proxy_cli_op, NULL, cli_data,            \
			 &_bt_mesh_od_priv_proxy_cli_cb)

/** @brief Get the target's On-Demand Private GATT Proxy state.
 *
 *  This method can be used asynchronously by setting @p val_rsp as NULL.
 *  This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  To process the response arguments of an async method, register
 *  the @c od_status callback in @c bt_mesh_od_priv_proxy_cli struct.
 *
 *  @param net_idx    Network index to encrypt with.
 *  @param addr       Target node address.
 *  @param val_rsp    Response buffer for On-Demand Private GATT Proxy value.
 *
 *  @return 0 on success, or (negative) error code otherwise.
 */
int bt_mesh_od_priv_proxy_cli_get(uint16_t net_idx, uint16_t addr, uint8_t *val_rsp);

/** @brief Set the target's On-Demand Private GATT Proxy state.
 *
 *  This method can be used asynchronously by setting @p val_rsp as NULL.
 *  This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  To process the response arguments of an async method, register
 *  the @c od_status callback in @c bt_mesh_od_priv_proxy_cli struct.
 *
 *  @param net_idx    Network index to encrypt with.
 *  @param addr       Target node address.
 *  @param val        On-Demand Private GATT Proxy state to be set
 *  @param val_rsp    Response buffer for On-Demand Private GATT Proxy value.
 *
 *  @return 0 on success, or (negative) error code otherwise.
 */
int bt_mesh_od_priv_proxy_cli_set(uint16_t net_idx, uint16_t addr, uint8_t val, uint8_t *val_rsp);

/** @brief Set the transmission timeout value.
 *
 *  @param timeout The new transmission timeout in milliseconds.
 */
void bt_mesh_od_priv_proxy_cli_timeout_set(int32_t timeout);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_od_priv_proxy_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_od_priv_proxy_cli_cb;
/** @endcond */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_OD_PRIV_PROXY_CLI_H__ */
