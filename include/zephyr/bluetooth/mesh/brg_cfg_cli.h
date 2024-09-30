/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_BRG_CFG_CLI_H__
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_BRG_CFG_CLI_H__

#include <zephyr/bluetooth/mesh/brg_cfg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup bt_mesh_brg_cfg_cli Bridge Configuration Client Model
 * @ingroup bt_mesh
 * @{
 *  @brief API for the Bluetooth Mesh Bridge Configuration Client model
 */

struct bt_mesh_brg_cfg_cli;

/**
 *  @brief Bridge Configuration Client model Composition Data entry.
 *
 *  @param _cli Pointer to a @ref bt_mesh_brg_cfg_cli instance.
 */
#define BT_MESH_MODEL_BRG_CFG_CLI(_cli)                                                            \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_BRG_CFG_CLI, _bt_mesh_brg_cfg_cli_op, NULL, _cli,        \
			 &_bt_mesh_brg_cfg_cli_cb)

/** Mesh Bridge Configuration Client Status messages callback */
struct bt_mesh_brg_cfg_cli_cb {
	/** @brief Optional callback for Subnet Bridge Status message.
	 *
	 *  Handles received Subnet Bridge Status messages from a Bridge
	 *  Configuration Server.

	 *  @param cli      Bridge Configuration Client context.
	 *  @param addr     Address of the sender.
	 *  @param status   Status received from the server.
	 */
	void (*bridge_status)(struct bt_mesh_brg_cfg_cli *cli, uint16_t addr,
			      enum bt_mesh_brg_cfg_state status);

	/** @brief Optional callback for Bridging Table Size Status message.
	 *
	 *  Handles received Bridging Table Size Status messages from a Bridge
	 *  Configuration Server.
	 *
	 *  @param cli      Bridge Configuration Client context.
	 *  @param addr     Address of the sender.
	 *  @param size     Size received from the server.
	 */
	void (*table_size_status)(struct bt_mesh_brg_cfg_cli *cli, uint16_t addr, uint16_t size);

	/** @brief Optional callback for Bridging Table Status message.
	 *
	 *  Handles received Bridging Table status messages from a Bridge
	 *  Configuration Server.
	 *
	 *  @param cli      Bridge Configuration Client context.
	 *  @param addr     Address of the sender.
	 *  @param rsp      Response received from the Bridging Configuration Server.
	 */
	void (*table_status)(struct bt_mesh_brg_cfg_cli *cli, uint16_t addr,
			     struct bt_mesh_brg_cfg_table_status *rsp);

	/** @brief Optional callback for Bridged Subnets List message.
	 *
	 *  Handles received Bridged Subnets List messages from a Bridge
	 *  Configuration Server.
	 *
	 *  @param cli      Bridge Configuration Client context.
	 *  @param addr     Address of the sender.
	 *  @param rsp      Response received from the Bridging Configuration Server.
	 */
	void (*subnets_list)(struct bt_mesh_brg_cfg_cli *cli, uint16_t addr,
			     struct bt_mesh_brg_cfg_subnets_list *rsp);

	/** @brief Optional callback for Bridging Table List message.
	 *
	 *  Handles received Bridging Table List messages from a Bridge
	 *  Configuration Server.
	 *
	 *  @param cli      Bridge Configuration Client context.
	 *  @param addr     Address of the sender.
	 *  @param rsp      Response received from the Bridging Configuration Server.
	 */
	void (*table_list)(struct bt_mesh_brg_cfg_cli *cli, uint16_t addr,
			   struct bt_mesh_brg_cfg_table_list *rsp);
};

/** Bridge Configuration Client Model Context */
struct bt_mesh_brg_cfg_cli {
	/** Bridge Configuration model entry pointer */
	const struct bt_mesh_model *model;

	/** Event handler callbacks */
	const struct bt_mesh_brg_cfg_cli_cb *cb;

	/* Internal parameters for tracking message responses. */
	struct bt_mesh_msg_ack_ctx ack_ctx;
};

/** @brief Sends a Subnet Bridge Get message to the given destination address
 *
 *  This function sends a Subnet Bridge Get message to the given destination
 *  address to query the value of the Subnet Bridge state of a subnet. The
 *  Subnet Bridge state indicates whether the subnet bridged feature is enabled
 *  or not. The function expects a Subnet Bridge Status message as a response
 *  from the destination node.
 *
 *  This method can be used asynchronously by setting @p status as NULL. This
 *  way the method will not wait for response and will return immediately after
 *  sending the command.
 *
 *  @param net_idx Network index to encrypt the message with.
 *  @param addr    Target node address.
 *  @param status  Status response parameter, returns one of
 *                 @ref BT_MESH_BRG_CFG_DISABLED or
 *                 @ref BT_MESH_BRG_CFG_ENABLED on success.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_brg_cfg_cli_get(uint16_t net_idx, uint16_t addr, enum bt_mesh_brg_cfg_state *status);

/** @brief Sends a Subnet Bridge Set message to the given destination address
 *  with the given parameters
 *
 *  This function sends a Subnet Bridge Set message to the given destination
 *  address with the given parameters to set the value of the Subnet Bridge
 *  state of a subnet. The Subnet Bridge state indicates whether the subnet
 *  bridge feature is enabled or not. The function expects a Subnet Bridge
 *  Status message as a response from the destination node.
 *
 *  This method can be used asynchronously by setting @p status as NULL. This
 *  way the method will not wait for response and will return immediately after
 *  sending the command.
 *
 *  @param net_idx Network index to encrypt the message with.
 *  @param addr    Target node address.
 *  @param val     Value to set the Subnet Bridge state to. Must be one of
 *                 @ref BT_MESH_BRG_CFG_DISABLED or
 *                 @ref BT_MESH_BRG_CFG_ENABLED.
 *  @param status  Status response parameter, returns one of
 *                 @ref BT_MESH_BRG_CFG_DISABLED or
 *                 @ref BT_MESH_BRG_CFG_ENABLED on success.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_brg_cfg_cli_set(uint16_t net_idx, uint16_t addr, enum bt_mesh_brg_cfg_state val,
			    enum bt_mesh_brg_cfg_state *status);

/** @brief Sends a Bridging Table Size Get message to the given destination
 *  address with the given parameters
 *
 *  This function sends a Bridging Table Size Get message to the given
 *  destination address with the given parameters to get the size of the Bridging
 *  Table of the node. The Bridging Table size indicates the maximum number of
 *  entries that can be stored in the Bridging Table. The function expects a
 *  Bridging Table Size Status message as a response from the destination node.
 *
 *  This method can be used asynchronously by setting @p size as NULL. This way
 *  the method will not wait for response and will return immediately after
 *  sending the command.
 *
 *  @param net_idx Network index to encrypt the message with.
 *  @param addr    Target node address.
 *  @param size    Bridging Table size response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_brg_cfg_cli_table_size_get(uint16_t net_idx, uint16_t addr, uint16_t *size);

/** @brief Sends a Bridging Table Add message to the given destination address
 *  with the given parameters
 *
 *  This function sends a Bridging Table Add message to the given destination
 *  address with the given parameters to add an entry to the Bridging Table. The
 *  Bridging Table contains the net keys and addresses that are authorized to be
 *  bridged by the node. The function expects a Bridging Table Status message as
 *  a response from the destination node.
 *
 *  This method can be used asynchronously by setting @p rsp as NULL. This way
 *  the method will not wait for response and will return immediately after
 *  sending the command.
 *
 *  @param net_idx    Network index to encrypt the message with.
 *  @param addr       Target node address.
 *  @param entry      Pointer to bridging Table entry to add.
 *  @param rsp        Status response parameter
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_brg_cfg_cli_table_add(uint16_t net_idx, uint16_t addr,
				  struct bt_mesh_brg_cfg_table_entry *entry,
				  struct bt_mesh_brg_cfg_table_status *rsp);

/** @brief Sends a Bridging Table Remove message to the given destination
 *  address with the given parameters
 *
 *  This function sends a Bridging Table Remove message to the given destination
 *  address with the given parameters to remove an entry from the Bridging
 *  Table. The Bridging Table contains the net keys and addresses that are
 *  authorized to be bridged by the node. The function expects a Bridging Table
 *  Status message as a response from the destination node.
 *
 *  This method can be used asynchronously by setting @p rsp as NULL. This way
 *  the method will not wait for response and will return immediately after
 *  sending the command.
 *
 *  @param net_idx   Network index to encrypt the message with.
 *  @param addr      Target node address.
 *  @param net_idx1  NetKey Index of the first subnet
 *  @param net_idx2  NetKey Index of the second subnet
 *  @param addr1     Address of the node in the first subnet
 *  @param addr2     Address of the node in the second subnet
 *  @param rsp       Pointer to a struct storing the received response from the
 *                   server, or NULL to not wait for a response.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_brg_cfg_cli_table_remove(uint16_t net_idx, uint16_t addr, uint16_t net_idx1,
				     uint16_t net_idx2, uint16_t addr1, uint16_t addr2,
				     struct bt_mesh_brg_cfg_table_status *rsp);

/** @brief Sends a Bridged Subnets Get message to the given destination address
 *  with the given parameters
 *
 *  This function sends a Bridged Subnets Get message to the given destination
 *  address with the given parameters to get the list of subnets that are
 *  bridged by the node. The function expects a Bridged Subnets List message as
 *  a response from the destination node.
 *
 *  This method can be used asynchronously by setting @p rsp as NULL. This way
 *  the method will not wait for response and will return immediately after
 *  sending the command.
 *
 *  When @c rsp is set, the user is responsible for providing a buffer for the
 *  filtered set of N pairs of NetKey Indexes in
 *  @ref bt_mesh_brg_cfg_subnets_list::list. If a buffer is not provided, the
 *  bridged subnets won't be copied.

 *  @param net_idx          Network index to encrypt the message with.
 *  @param addr             Target node address.
 *  @param filter_net_idx   Filter and NetKey Index used for filtering
 *  @param start_idx        Start offset to read in units of Bridging Table state entries
 *  @param rsp              Pointer to a struct storing the received response
 *                          from the server, or NULL to not wait for a response.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_brg_cfg_cli_subnets_get(uint16_t net_idx, uint16_t addr,
				    struct bt_mesh_brg_cfg_filter_netkey filter_net_idx,
				    uint8_t start_idx, struct bt_mesh_brg_cfg_subnets_list *rsp);

/** @brief Sends a Bridging Table Get message to the given destination address
 *  with the given parameters
 *
 *  This function sends a Bridging Table Get message to the given destination
 *  address with the given parameters to get the contents of the Bridging Table.
 *  The Bridging Table contains the addresses that are authorized to be bridged
 *  by the node. The function expects a Bridging Table List message as a
 *  response from the destination node.
 *
 *  This method can be used asynchronously by setting @p rsp as NULL. This way
 *  the method will not wait for response and will return immediately after
 *  sending the command.
 *
 *  When @c rsp is set, the user is responsible for providing a buffer for the
 *   filtered set of N pairs of NetKey Indexes in
 *  @ref bt_mesh_brg_cfg_table_list::list. If a buffer is not provided,
 *  the bridged addresses won't be copied. If a buffer size is shorter than
 *  received list, only those many entries that fit in the buffer will be copied
 *  from the list, and rest will be discarded.
 *
 *  @param net_idx   Network index to encrypt the message with.
 *  @param addr      Target node address.
 *  @param net_idx1  NetKey Index of the first subnet.
 *  @param net_idx2  NetKey Index of the second subnet.
 *  @param start_idx Start offset to read in units of Bridging Table state entries.
 *  @param rsp       Pointer to a struct storing the received response from the
 *                   server, or NULL to not wait for a response.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_brg_cfg_cli_table_get(uint16_t net_idx, uint16_t addr, uint16_t net_idx1,
				  uint16_t net_idx2, uint16_t start_idx,
				  struct bt_mesh_brg_cfg_table_list *rsp);

/** @brief Get the current transmission timeout value.
 *
 *  @return The configured transmission timeout in milliseconds.
 */
int32_t bt_mesh_brg_cfg_cli_timeout_get(void);

/** @brief Set the transmission timeout value.
 *
 *  @param timeout The new transmission timeout.
 */
void bt_mesh_brg_cfg_cli_timeout_set(int32_t timeout);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_brg_cfg_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_brg_cfg_cli_cb;
/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_BRG_CFG_CLI_H__ */
