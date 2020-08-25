/** @file
 *  @brief Bluetooth Mesh Runtime Configuration APIs.
 */

/*
 * Copyright (c) 2020 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_CFG_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_CFG_H_

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

/**
 * @brief Bluetooth Mesh Runtime Configuration API
 * @defgroup bt_mesh_cfg Bluetooth Mesh Runtime Configuration
 * @ingroup bt_mesh
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Bluetooth Mesh Feature states */
enum bt_mesh_feat_state {
	/** Feature is supported, but disabled. */
	BT_MESH_FEATURE_DISABLED,
	/** Feature is supported and enabled. */
	BT_MESH_FEATURE_ENABLED,
	/** Feature is not supported, and cannot be enabled. */
	BT_MESH_FEATURE_NOT_SUPPORTED,
};

/**
 * @brief Bluetooth Mesh Subnet Configuration
 * @defgroup bt_mesh_cfg_subnet Bluetooth Mesh Subnet Configuration
 * @{
 */

/** @brief Add a Subnet.
 *
 *  Adds a subnet with the given network index and network key to the list of
 *  known Subnets. All messages sent on the given Subnet will be processed by
 *  this node, and the node may send and receive Network Beacons on the given
 *  Subnet.
 *
 *  @param net_idx Network index.
 *  @param key     Root network key of the Subnet. All other keys are derived
 *                 from this.
 *
 *  @retval STATUS_SUCCESS The Subnet was successfully added.
 *  @retval STATUS_INSUFF_RESOURCES No room for this Subnet.
 *  @retval STATUS_UNSPECIFIED The Subnet couldn't be created for some reason.
 */
uint8_t bt_mesh_subnet_add(uint16_t net_idx, const uint8_t key[16]);

/** @brief Update the given Subnet.
 *
 *  Starts the Key Refresh procedure for this Subnet by adding a second set of
 *  encryption keys. The Subnet will continue sending with the old key (but
 *  receiving messages using both) until the Subnet enters Key Refresh phase 2.
 *
 *  This allows a network configurator to replace old network and application
 *  keys for the entire network, effectively removing access for all nodes that
 *  aren't given the new keys.
 *
 *  @param net_idx Network index.
 *  @param key     New root network key of the Subnet.
 *
 *  @retval STATUS_SUCCESS The Subnet was updated with a second key.
 *  @retval STATUS_INVALID_NETKEY The NetIdx is unknown.
 *  @retval STATUS_IDX_ALREADY_STORED The @c key value is the same as the
 *                                    current key.
 *  @retval STATUS_CANNOT_UPDATE The Subnet cannot be updated for some reason.
 */
uint8_t bt_mesh_subnet_update(uint16_t net_idx, const uint8_t key[16]);

/** @brief Delete a Subnet.
 *
 *  Removes the Subnet with the given network index from the node. The node will
 *  stop sending Network Beacons with the given Subnet, and can no longer
 *  process messages on this Subnet.
 *
 *  All Applications bound to this Subnet are also deleted.
 *
 *  @param net_idx Network index.
 *
 *  @retval STATUS_SUCCESS The Subnet was deleted.
 *  @retval STATUS_INVALID_NETKEY The NetIdx is unknown.
 */
uint8_t bt_mesh_subnet_del(uint16_t net_idx);

/** @brief Check whether a Subnet is known.
 *
 *  @param net_idx Network index
 *
 *  @return true if a Subnet with the given index exists, false otherwise.
 */
bool bt_mesh_subnet_exists(uint16_t net_idx);

/** @brief Set the Subnet's Key Refresh phase.
 *
 *  The Key Refresh procedure is started by updating the Subnet keys through
 *  @ref bt_mesh_subnet_update. This puts the Subnet in Key Refresh Phase 1.
 *  Once all nodes have received the new Subnet key, Key Refresh Phase 2 can be
 *  activated through this function to start transmitting with the new network
 *  key. Finally, to revoke the old key, set the Key Refresh Phase to 3. This
 *  removes the old keys from the node, and returns the Subnet back to normal
 *  single-key operation with the new key set.
 *
 *  @param net_idx Network index.
 *  @param phase   Pointer to the new Key Refresh phase. Will return the actual
 *                 Key Refresh phase after updating.
 *
 *  @retval STATUS_SUCCESS The Key Refresh phase of the Subnet was successfully
 *                         changed.
 *  @retval STATUS_INVALID_NETKEY The NetIdx is unknown.
 *  @retval STATUS_CANNOT_UPDATE The given phase change is invalid.
 */
uint8_t bt_mesh_subnet_kr_phase_set(uint16_t net_idx, uint8_t *phase);

/** @brief Get the Subnet's Key Refresh phase.
 *
 *  @param net_idx Network index.
 *  @param phase   Pointer to the Key Refresh variable to fill.
 *
 *  @retval STATUS_SUCCESS Successfully populated the @c phase variable.
 *  @retval STATUS_INVALID_NETKEY The NetIdx is unknown.
 */
uint8_t bt_mesh_subnet_kr_phase_get(uint16_t net_idx, uint8_t *phase);

/** @brief Set the Node Identity state of the Subnet.
 *
 *  The Node Identity state of a Subnet determines whether the Subnet advertises
 *  connectable Node Identity beacons for Proxy Clients to connect to.
 *  Once started, the Node Identity beacon runs for 60 seconds, or until it is
 *  stopped.
 *
 *  This function serves the same purpose as @ref bt_mesh_proxy_identity_enable,
 *  but only acts on a single Subnet.
 *
 *  GATT Proxy support must be enabled through
 *  @option{CONFIG_BT_MESH_GATT_PROXY}.
 *
 *  @param net_idx Network index.
 *  @param node_id New Node Identity state, must be either @ref
 *                 BT_MESH_FEATURE_ENABLED or @ref BT_MESH_FEATURE_DISABLED.
 *
 *  @retval STATUS_SUCCESS Successfully set the Node Identity state of the
 *                         Subnet.
 *  @retval STATUS_INVALID_NETKEY The NetIdx is unknown.
 *  @retval STATUS_FEAT_NOT_SUPP The Node Identity feature is not supported.
 *  @retval STATUS_CANNOT_SET Couldn't set the Node Identity state.
 */
uint8_t bt_mesh_subnet_node_id_set(uint16_t net_idx,
				   enum bt_mesh_feat_state node_id);

/** @brief Get the Node Identity state of the Subnet.
 *
 *  @param net_idx Network index.
 *  @param node_id Node Identity variable to fill.
 *
 *  @retval STATUS_SUCCESS Successfully populated the @c node_id variable.
 *  @retval STATUS_INVALID_NETKEY The NetIdx is unknown.
 */
uint8_t bt_mesh_subnet_node_id_get(uint16_t net_idx,
				   enum bt_mesh_feat_state *node_id);

/** @brief Get a list of all known Subnet indexes.
 *
 *  Builds a list of all known Subnet indexes in the @c net_idxs array.
 *  If the @c net_idxs array is smaller than the list of known Subnets, this
 *  function fills all available entries and returns @c -ENOMEM. In this
 *  case, the next @c max entries of the list can be read out by calling
 *  @code
 *  bt_mesh_subnets_get(list, max, max);
 *  @endcode
 *
 *  Note that any changes to the Subnet list between calls to this function
 *  could change the order and number of entries in the list.
 *
 *  @param net_idxs Array to fill.
 *  @param max      Max number of indexes to return.
 *  @param skip     Number of indexes to skip. Enables batched processing of the
 *                  list.
 *
 *  @return The number of indexes added to the @c net_idxs array, or @c -ENOMEM
 *          if the number of known Subnets exceeds the @c max parameter.
 */
ssize_t bt_mesh_subnets_get(uint16_t net_idxs[], size_t max, off_t skip);

/**
 * @}
 */

/**
 * @brief Bluetooth Mesh Application Configuration
 * @defgroup bt_mesh_cfg_app Bluetooth Mesh Application Configuration
 * @{
 */

/** @brief Add an Application key.
 *
 *  Adds the Application with the given index to the list of known applications.
 *  Allows the node to send and receive model messages encrypted with this
 *  Application key.
 *
 *  Every Application is bound to a specific Subnet. The node must know the
 *  Subnet the Application is bound to before it can add the Application.
 *
 *  @param app_idx Application index.
 *  @param net_idx Network index the Application is bound to.
 *  @param key     Application key value.
 *
 *  @retval STATUS_SUCCESS The Application was successfully added.
 *  @retval STATUS_INVALID_NETKEY The NetIdx is unknown.
 *  @retval STATUS_INSUFF_RESOURCES There's no room for storing this
 *                                  Application.
 *  @retval STATUS_INVALID_BINDING This AppIdx is already bound to another
 *                                 Subnet.
 *  @retval STATUS_IDX_ALREADY_STORED This AppIdx is already stored with a
 *                                    different key value.
 *  @retval STATUS_CANNOT_SET Cannot set the Application key for some reason.
 */
uint8_t bt_mesh_app_key_add(uint16_t app_idx, uint16_t net_idx,
			    const uint8_t key[16]);

/** @brief Update an Application key.
 *
 *  Update an Application with a second Application key, as part of the
 *  Key Refresh procedure of the bound Subnet. The node will continue
 *  transmitting with the old application key (but receiving on both) until the
 *  Subnet enters Key Refresh phase 2. Once the Subnet enters Key Refresh phase
 *  3, the old application key will be deleted.
 *
 *  @note The Application key can only be updated if the bound Subnet is in Key
 *        Refresh phase 1.
 *
 *  @param app_idx Application index.
 *  @param net_idx Network index the Application is bound to, or
 *                 @ref BT_MESH_KEY_ANY to skip the binding check.
 *  @param key     New key value.
 *
 *  @retval STATUS_SUCCESS The Application key was successfully updated.
 *  @retval STATUS_INVALID_NETKEY The NetIdx is unknown.
 *  @retval STATUS_INVALID_BINDING This AppIdx is not bound to the given NetIdx.
 *  @retval STATUS_CANNOT_UPDATE The Application key cannot be updated for some
 *                               reason.
 *  @retval STATUS_IDX_ALREADY_STORED This AppIdx is already updated with a
 *                                    different key value.
 */
uint8_t bt_mesh_app_key_update(uint16_t app_idx, uint16_t net_idx,
			       const uint8_t key[16]);

/** @brief Delete an Application key.
 *
 *  All models bound to this application will remove this binding.
 *  All models publishing with this application will stop publishing.
 *
 *  @param app_idx Application index.
 *  @param net_idx Network index.
 *
 *  @retval STATUS_SUCCESS The Application key was successfully deleted.
 *  @retval STATUS_INVALID_NETKEY The NetIdx is unknown.
 *  @retval STATUS_INVALID_BINDING This AppIdx is not bound to the given NetIdx.
 */
uint8_t bt_mesh_app_key_del(uint16_t app_idx, uint16_t net_idx);

/** @brief Check if an Application key is known.
 *
 *  @param app_idx Application index.
 *
 *  @return true if the Application is known, false otherwise.
 */
bool bt_mesh_app_key_exists(uint16_t app_idx);

/** @brief Get a list of all known Application key indexes.
 *
 *  Builds a list of all Application indexes for the given network index in the
 *  @c app_idxs array. If the @c app_idxs array cannot fit all bound
 *  Applications, this function fills all available entries and returns @c
 *  -ENOMEM. In this case, the next @c max entries of the list can be read out
 *  by calling
 *  @code
 *  bt_mesh_app_keys_get(net_idx, list, max, max);
 *  @endcode
 *
 *  Note that any changes to the Application key list between calls to this
 *  function could change the order and number of entries in the list.
 *
 *  @param net_idx  Network Index to get the Applications of, or @ref
 *                  BT_MESH_KEY_ANY to get all Applications.
 *  @param app_idxs Array to fill.
 *  @param max      Max number of indexes to return.
 *  @param skip     Number of indexes to skip. Enables batched processing of the
 *                  list.
 *
 *  @return The number of indexes added to the @c app_idxs array, or @c -ENOMEM
 *          if the number of known Applications exceeds the @c max parameter.
 */
ssize_t bt_mesh_app_keys_get(uint16_t net_idx, uint16_t app_idxs[], size_t max,
			     off_t skip);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_CFG_H_ */
