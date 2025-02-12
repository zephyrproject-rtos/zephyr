/*
 * Copyright (c) 2019 Tobias Svehagen
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_CDB_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_CDB_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/sys/atomic.h>
#include <zephyr/bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_BT_MESH_CDB)
#define NODE_COUNT    CONFIG_BT_MESH_CDB_NODE_COUNT
#define SUBNET_COUNT  CONFIG_BT_MESH_CDB_SUBNET_COUNT
#define APP_KEY_COUNT CONFIG_BT_MESH_CDB_APP_KEY_COUNT
#else
#define NODE_COUNT    0
#define SUBNET_COUNT  0
#define APP_KEY_COUNT 0
#endif

enum {
	BT_MESH_CDB_NODE_CONFIGURED,

	BT_MESH_CDB_NODE_FLAG_COUNT
};

struct bt_mesh_cdb_node {
	uint8_t  uuid[16];
	uint16_t addr;
	uint16_t net_idx;
	uint8_t  num_elem;
	struct bt_mesh_key dev_key;

	ATOMIC_DEFINE(flags, BT_MESH_CDB_NODE_FLAG_COUNT);
};

struct bt_mesh_cdb_subnet {
	uint16_t net_idx;

	uint8_t kr_phase;

	struct {
		struct bt_mesh_key net_key;
	} keys[2];
};

struct bt_mesh_cdb_app_key {
	uint16_t net_idx;
	uint16_t app_idx;

	struct {
		struct bt_mesh_key app_key;
	} keys[2];
};

enum {
	BT_MESH_CDB_VALID,
	BT_MESH_CDB_SUBNET_PENDING,
	BT_MESH_CDB_KEYS_PENDING,
	BT_MESH_CDB_NODES_PENDING,
	BT_MESH_CDB_IVU_IN_PROGRESS,

	BT_MESH_CDB_FLAG_COUNT,
};

struct bt_mesh_cdb {
	uint32_t iv_index;
	uint16_t lowest_avail_addr;

	ATOMIC_DEFINE(flags, BT_MESH_CDB_FLAG_COUNT);

	struct bt_mesh_cdb_node nodes[NODE_COUNT];
	struct bt_mesh_cdb_subnet subnets[SUBNET_COUNT];
	struct bt_mesh_cdb_app_key app_keys[APP_KEY_COUNT];
};

extern struct bt_mesh_cdb bt_mesh_cdb;

/** @brief Create the Mesh Configuration Database.
 *
 *  Create and initialize the Mesh Configuration Database. A primary subnet,
 *  ie one with NetIdx 0, will be added and the provided key will be used as
 *  NetKey for that subnet.
 *
 *  @param key The NetKey to be used for the primary subnet.
 *
 *  @return 0 on success or negative error code on failure.
 */
int bt_mesh_cdb_create(const uint8_t key[16]);

/** @brief Clear the Mesh Configuration Database.
 *
 *  Remove all nodes, subnets and app-keys stored in the database and mark
 *  the database as invalid. The data will be cleared from persistent storage
 *  if CONFIG_BT_SETTINGS is enabled.
 */
void bt_mesh_cdb_clear(void);

/** @brief Set and store the IV Index and IV Update flag.
 *
 *  The IV Index stored in the CDB will be the one used during provisioning
 *  of new nodes. This function is generally only used from inside the stack.
 *
 *  This function will store the data to persistent storage if
 *  CONFIG_BT_SETTINGS is enabled.
 *
 *  @param iv_index The new IV Index to use.
 *  @param iv_update True if there is an ongoing IV Update procedure.
 */
void bt_mesh_cdb_iv_update(uint32_t iv_index, bool iv_update);

/** @brief Allocate a node.
 *
 *  Allocate a new node in the CDB.
 *
 *  If @c addr is 0, @ref bt_mesh_cdb_free_addr_get will be used to allocate
 *  a free address.
 *
 *  @param uuid UUID of the node.
 *  @param addr Address of the node's primary element. If 0, the lowest
 *              possible address available will be assigned to the node.
 *  @param num_elem Number of elements that the node has.
 *  @param net_idx NetIdx that the node was provisioned to.
 *
 *  @return The new node or NULL if CDB has already allocated
 *  :kconfig:option:`CONFIG_BT_MESH_CDB_NODE_COUNT` nodes, or reached the
 *  end of the unicast address range, or if @c addr is non-zero and less
 *  than the lowest available address or collide with the allocated addresses.
 */
struct bt_mesh_cdb_node *bt_mesh_cdb_node_alloc(const uint8_t uuid[16], uint16_t addr,
						uint8_t num_elem, uint16_t net_idx);

/** @brief Get the first available address for the given element count.
 *
 *  @param num_elem Number of elements to accommodate.
 *
 *  @return The first unicast address in an address range that allows a node
 *          with the given number of elements to fit.
 */
uint16_t bt_mesh_cdb_free_addr_get(uint8_t num_elem);

/** @brief Delete a node.
 *
 *  Delete a node from the CDB. When deleting the node and the address of the
 *  last element of the deleted node is greater than the lowest available
 *  address, CDB will update the lowest available address. The lowest
 *  available address is reset and the deleted addresses can be reused only
 *  after IV Index update.
 *
 *  @param node The node to be deleted.
 *  @param store If true, the node will be cleared from persistent storage.
 */
void bt_mesh_cdb_node_del(struct bt_mesh_cdb_node *node, bool store);

/** @brief Update a node.
 *
 *  Assigns the node a new address and clears the previous persistent storage
 *  entry.
 *
 *  @param node The node to be deleted.
 *  @param addr New unicast address for the node.
 *  @param num_elem Updated number of elements in the node.
 */
void bt_mesh_cdb_node_update(struct bt_mesh_cdb_node *node, uint16_t addr,
			     uint8_t num_elem);

/** @brief Get a node by address.
 *
 *  Try to find the node that has the provided address assigned to one of its
 *  elements.
 *
 *  @param addr Address of the element to look for.
 *
 *  @return The node that has an element with address addr or NULL if no such
 *          node exists.
 */
struct bt_mesh_cdb_node *bt_mesh_cdb_node_get(uint16_t addr);

/** @brief Store node to persistent storage.
 *
 *  @param node Node to be stored.
 */
void bt_mesh_cdb_node_store(const struct bt_mesh_cdb_node *node);

/** @brief Import device key for selected node.
 *
 *  Using security library with PSA implementation access to the key by pointer
 *  will not give a valid value since the key is hidden in the library.
 *  The application has to import the key.
 *
 *  @param node Selected node.
 *  @param in key value.
 *
 *  @return 0 on success or negative error code on failure.
 */
int bt_mesh_cdb_node_key_import(struct bt_mesh_cdb_node *node, const uint8_t in[16]);

/** @brief Export device key from selected node.
 *
 *  Using security library with PSA implementation access to the key by pointer
 *  will not give a valid value since the key is hidden in the library.
 *  The application has to export the key.
 *
 *  @param node Selected node.
 *  @param out key value.
 *
 *  @return 0 on success or negative error code on failure.
 */
int bt_mesh_cdb_node_key_export(const struct bt_mesh_cdb_node *node, uint8_t out[16]);

enum {
	BT_MESH_CDB_ITER_STOP = 0,
	BT_MESH_CDB_ITER_CONTINUE,
};

/** @typedef bt_mesh_cdb_node_func_t
 *  @brief Node iterator callback.
 *
 *  @param node Node found.
 *  @param user_data Data given.
 *
 *  @return BT_MESH_CDB_ITER_CONTINUE to continue to iterate through the nodes
 *          or BT_MESH_CDB_ITER_STOP to stop.
 */
typedef uint8_t (*bt_mesh_cdb_node_func_t)(struct bt_mesh_cdb_node *node,
					   void *user_data);

/** @brief Node iterator.
 *
 *  Iterate nodes in the Mesh Configuration Database. The callback function
 *  will only be called for valid, ie allocated, nodes.
 *
 *  @param func Callback function.
 *  @param user_data Data to pass to the callback.
 */
void bt_mesh_cdb_node_foreach(bt_mesh_cdb_node_func_t func, void *user_data);

/** @brief Allocate a subnet.
 *
 *  Allocate a new subnet in the CDB.
 *
 *  @param net_idx NetIdx of the subnet.
 *
 *  @return The new subnet or NULL if it cannot be allocated due to
 *          lack of resources or the subnet has been already allocated.
 */
struct bt_mesh_cdb_subnet *bt_mesh_cdb_subnet_alloc(uint16_t net_idx);

/** @brief Delete a subnet.
 *
 *  Delete a subnet from the CDB.
 *
 *  @param sub The subnet to be deleted.
 *  @param store If true, the subnet will be cleared from persistent storage.
 */
void bt_mesh_cdb_subnet_del(struct bt_mesh_cdb_subnet *sub, bool store);

/** @brief Get a subnet by NetIdx
 *
 *  Try to find the subnet with the specified NetIdx.
 *
 *  @param net_idx NetIdx of the subnet to look for.
 *
 *  @return The subnet with the specified NetIdx or NULL if no such subnet
 *          exists.
 */
struct bt_mesh_cdb_subnet *bt_mesh_cdb_subnet_get(uint16_t net_idx);

/** @brief Store subnet to persistent storage.
 *
 *  @param sub Subnet to be stored.
 */
void bt_mesh_cdb_subnet_store(const struct bt_mesh_cdb_subnet *sub);

/** @brief Get the flags for a subnet
 *
 *  @param sub The subnet to get flags for.
 *
 *  @return The flags for the subnet.
 */
uint8_t bt_mesh_cdb_subnet_flags(const struct bt_mesh_cdb_subnet *sub);

/** @brief Import network key for selected subnetwork.
 *
 *  Using security library with PSA implementation access to the key by pointer
 *  will not give a valid value since the key is hidden in the library.
 *  The application has to import the key.
 *
 *  @param sub Selected subnetwork.
 *  @param key_idx 0 or 1. If Key Refresh procedure is in progress then two keys are available.
 *                 The old key has an index 0 and the new one has an index 1.
 *                 Otherwise, the only key with index 0 exists.
 *  @param in key value.
 *
 *  @return 0 on success or negative error code on failure.
 */
int bt_mesh_cdb_subnet_key_import(struct bt_mesh_cdb_subnet *sub, int key_idx,
				  const uint8_t in[16]);

/** @brief Export network key from selected subnetwork.
 *
 *  Using security library with PSA implementation access to the key by pointer
 *  will not give a valid value since the key is hidden in the library.
 *  The application has to export the key.
 *
 *  @param sub Selected subnetwork.
 *  @param key_idx 0 or 1. If Key Refresh procedure is in progress then two keys are available.
 *                 The old key has an index 0 and the new one has an index 1.
 *                 Otherwise, the only key with index 0 exists.
 *  @param out key value.
 *
 *  @return 0 on success or negative error code on failure.
 */
int bt_mesh_cdb_subnet_key_export(const struct bt_mesh_cdb_subnet *sub, int key_idx,
				  uint8_t out[16]);

/** @brief Allocate an application key.
 *
 *  Allocate a new application key in the CDB.
 *
 *  @param net_idx NetIdx of NetKey that the application key is bound to.
 *  @param app_idx AppIdx of the application key.
 *
 *  @return The new application key or NULL if it cannot be allocated due to
 *          lack of resources or the key has been already allocated.
 */
struct bt_mesh_cdb_app_key *bt_mesh_cdb_app_key_alloc(uint16_t net_idx,
						      uint16_t app_idx);

/** @brief Delete an application key.
 *
 *  Delete an application key from the CDB.
 *
 *  @param key The application key to be deleted.
 *  @param store If true, the key will be cleared from persistent storage.
 */
void bt_mesh_cdb_app_key_del(struct bt_mesh_cdb_app_key *key, bool store);

/** @brief Get an application key by AppIdx
 *
 *  Try to find the application key with the specified AppIdx.
 *
 *  @param app_idx AppIdx of the application key to look for.
 *
 *  @return The application key with the specified AppIdx or NULL if no such key
 *          exists.
 */
struct bt_mesh_cdb_app_key *bt_mesh_cdb_app_key_get(uint16_t app_idx);

/** @brief Store application key to persistent storage.
 *
 *  @param key Application key to be stored.
 */
void bt_mesh_cdb_app_key_store(const struct bt_mesh_cdb_app_key *key);

/** @brief Import application key.
 *
 *  Using security library with PSA implementation access to the key by pointer
 *  will not give a valid value since the key is hidden in the library.
 *  The application has to import the key.
 *
 *  @param key cdb application key structure.
 *  @param key_idx 0 or 1. If Key Refresh procedure is in progress then two keys are available.
 *                 The old key has an index 0 and the new one has an index 1.
 *                 Otherwise, the only key with index 0 exists.
 *  @param in key value.
 *
 *  @return 0 on success or negative error code on failure.
 */
int bt_mesh_cdb_app_key_import(struct bt_mesh_cdb_app_key *key, int key_idx, const uint8_t in[16]);

/** @brief Export application key.
 *
 *  Using security library with PSA implementation access to the key by pointer
 *  will not give a valid value since the key is hidden in the library.
 *  The application has to export the key.
 *
 *  @param key cdb application key structure.
 *  @param key_idx 0 or 1. If Key Refresh procedure is in progress then two keys are available.
 *                 The old key has an index 0 and the new one has an index 1.
 *                 Otherwise, the only key with index 0 exists.
 *  @param out key value.
 *
 *  @return 0 on success or negative error code on failure.
 */
int bt_mesh_cdb_app_key_export(const struct bt_mesh_cdb_app_key *key, int key_idx, uint8_t out[16]);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_CDB_H_ */
