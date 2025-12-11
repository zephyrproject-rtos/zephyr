/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_PRIV_BEACON_CLI_H__
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_PRIV_BEACON_CLI_H__

#include <zephyr/bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup bt_mesh_priv_beacon_cli Bluetooth Mesh Private Beacon Client
 * @ingroup bt_mesh
 * @{
 */

/**
 *
 *  @brief Private Beacon Client model composition data entry.
 *
 *  @param cli_data Pointer to a @ref bt_mesh_priv_beacon_cli instance.
 */
#define BT_MESH_MODEL_PRIV_BEACON_CLI(cli_data)                                \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_PRIV_BEACON_CLI,                     \
			 bt_mesh_priv_beacon_cli_op, NULL, cli_data,           \
			 &bt_mesh_priv_beacon_cli_cb)

struct bt_mesh_priv_beacon_cli;

/** Private Beacon */
struct bt_mesh_priv_beacon {
	/** Private beacon is enabled */
	uint8_t enabled;
	/** Random refresh interval (in 10 second steps), or 0 to keep current
	 *  value.
	 */
	uint8_t rand_interval;
};

/** Private Node Identity */
struct bt_mesh_priv_node_id {
	/** Index of the NetKey. */
	uint16_t net_idx;
	/** Private Node Identity state */
	uint8_t state;
	/** Response status code. */
	uint8_t status;
};

/** Private Beacon Client Status messages callbacks */
struct bt_mesh_priv_beacon_cli_cb {
	/** @brief Optional callback for Private Beacon Status message.
	 *
	 *  Handles received Private Beacon Status messages from a Private Beacon server.
	 *
	 *  @param cli         Private Beacon client context.
	 *  @param addr        Address of the sender.
	 *  @param priv_beacon Mesh Private Beacon state received from the server.
	 */
	void (*priv_beacon_status)(struct bt_mesh_priv_beacon_cli *cli, uint16_t addr,
				   struct bt_mesh_priv_beacon *priv_beacon);

	/** @brief Optional callback for Private GATT Proxy Status message.
	 *
	 *  Handles received Private GATT Proxy Status messages from a Private Beacon server.
	 *
	 *  @param cli         Private Beacon client context.
	 *  @param addr        Address of the sender.
	 *  @param gatt_proxy  Private GATT Proxy state received from the server.
	 */
	void (*priv_gatt_proxy_status)(struct bt_mesh_priv_beacon_cli *cli, uint16_t addr,
				       uint8_t gatt_proxy);

	/** @brief Optional callback for Private Node Identity Status message.
	 *
	 *  Handles received Private Node Identity Status messages from a Private Beacon server.
	 *
	 *  @param cli           Private Beacon client context.
	 *  @param addr          Address of the sender.
	 *  @param priv_node_id  Private Node Identity state received from the server.
	 */
	void (*priv_node_id_status)(struct bt_mesh_priv_beacon_cli *cli, uint16_t addr,
				    struct bt_mesh_priv_node_id *priv_node_id);
};

/** Mesh Private Beacon Client model */
struct bt_mesh_priv_beacon_cli {
	const struct bt_mesh_model *model;

	/* Internal parameters for tracking message responses. */
	struct bt_mesh_msg_ack_ctx ack_ctx;

	/** Optional callback for Private Beacon Client Status messages. */
	const struct bt_mesh_priv_beacon_cli_cb *cb;
};

/** @brief Set the target's Private Beacon state.
 *
 *  This method can be used asynchronously by setting @p rsp as NULL.
 *  This way the method will not wait for response and will return
 *  immediately after sending the command.

 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param val     New Private Beacon value.
 *  @param rsp     If set, returns response status on success.
 *
 *  @return 0 on success, or (negative) error code otherwise.
 */
int bt_mesh_priv_beacon_cli_set(uint16_t net_idx, uint16_t addr,
				struct bt_mesh_priv_beacon *val,
				struct bt_mesh_priv_beacon *rsp);

/** @brief Get the target's Private Beacon state.
 *
 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param val     Response buffer for Private Beacon value.
 *
 *  @return 0 on success, or (negative) error code otherwise.
 */
int bt_mesh_priv_beacon_cli_get(uint16_t net_idx, uint16_t addr,
				struct bt_mesh_priv_beacon *val);

/** @brief Set the target's Private GATT Proxy state.
 *
 *  This method can be used asynchronously by setting @p rsp as NULL.
 *  This way the method will not wait for response and will return
 *  immediately after sending the command.
 *
 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param val     New Private GATT Proxy value.
 *  @param rsp     If set, returns response status on success.
 *
 *  @return 0 on success, or (negative) error code otherwise.
 */
int bt_mesh_priv_beacon_cli_gatt_proxy_set(uint16_t net_idx, uint16_t addr,
					   uint8_t val, uint8_t *rsp);

/** @brief Get the target's Private GATT Proxy state.
 *
 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param val     Response buffer for Private GATT Proxy value.
 *
 *  @return 0 on success, or (negative) error code otherwise.
 */
int bt_mesh_priv_beacon_cli_gatt_proxy_get(uint16_t net_idx, uint16_t addr,
					   uint8_t *val);

/** @brief Set the target's Private Node Identity state.
 *
 *  This method can be used asynchronously by setting @p rsp as NULL.
 *  This way the method will not wait for response and will return
 *  immediately after sending the command.
 *
 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param val     New Private Node Identity value.
 *  @param rsp     If set, returns response status on success.
 *
 *  @return 0 on success, or (negative) error code otherwise.
 */
int bt_mesh_priv_beacon_cli_node_id_set(uint16_t net_idx, uint16_t addr,
					struct bt_mesh_priv_node_id *val,
					struct bt_mesh_priv_node_id *rsp);

/** @brief Get the target's Private Node Identity state.
 *
 *  @param net_idx     Network index to encrypt with.
 *  @param addr        Target node address.
 *  @param key_net_idx Network index to get the Private Node Identity state of.
 *  @param val         Response buffer for Private Node Identity value.
 *
 *  @return 0 on success, or (negative) error code otherwise.
 */
int bt_mesh_priv_beacon_cli_node_id_get(uint16_t net_idx, uint16_t addr,
					uint16_t key_net_idx,
					struct bt_mesh_priv_node_id *val);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op bt_mesh_priv_beacon_cli_op[];
extern const struct bt_mesh_model_cb bt_mesh_priv_beacon_cli_cb;
/** @endcond */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_PRIV_BEACON_CLI_H__ */
