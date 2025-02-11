/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_MESH_SUBNET_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_MESH_SUBNET_H_

#include <stdint.h>
#include <sys/types.h>
#include <zephyr/net/buf.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>

#define BT_MESH_NET_FLAG_KR       BIT(0)
#define BT_MESH_NET_FLAG_IVU      BIT(1)

#define BT_MESH_KR_NORMAL         0x00
#define BT_MESH_KR_PHASE_1        0x01
#define BT_MESH_KR_PHASE_2        0x02
#define BT_MESH_KR_PHASE_3        0x03

/** Which of the two subnet.keys should be used for sending. */
#define SUBNET_KEY_TX_IDX(sub) ((sub)->kr_phase == BT_MESH_KR_PHASE_2)

struct bt_mesh_net_rx;
enum bt_mesh_key_evt;

/** Network message encryption credentials */
struct bt_mesh_net_cred {
	uint8_t nid;                /* NID */
	struct bt_mesh_key enc;     /* EncKey */
	struct bt_mesh_key privacy; /* PrivacyKey */
};

struct bt_mesh_beacon {
	uint32_t sent;        /* Timestamp of last sent beacon */
	uint32_t recv;        /* Timestamp of last received beacon */
	uint8_t  last;       /* Number of beacons during last
				      * observation window
				      */
	uint8_t  cur;        /* Number of beacons observed during
				      * currently ongoing window.
				      */
	uint8_t  cache[8];   /* Cached last beacon auth value */
	uint8_t  auth[8];    /* Beacon Authentication Value */
};

/** Subnet instance. */
struct bt_mesh_subnet {
	uint16_t net_idx;            /* NetKeyIndex */

	uint8_t  kr_phase;           /* Key Refresh Phase */

	uint8_t  node_id;            /* Node Identity State */
	uint32_t node_id_start;      /* Node Identity started timestamp */

	struct bt_mesh_beacon secure_beacon;

#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
	struct bt_mesh_beacon priv_beacon;
	struct {
		uint16_t idx;        /* Private beacon random index */
		bool node_id;        /* Private Node Identity enabled */
		uint8_t data[5];     /* Private Beacon data */
	} priv_beacon_ctx;
#endif

	struct bt_mesh_subnet_keys {
		bool valid;
		struct bt_mesh_key net;         /* NetKey */
		struct bt_mesh_net_cred msg;
		uint8_t net_id[8];              /* Network ID */
	#if defined(CONFIG_BT_MESH_GATT_PROXY)
		struct bt_mesh_key identity;    /* IdentityKey */
	#endif
		struct bt_mesh_key beacon;      /* BeaconKey */
		struct bt_mesh_key priv_beacon; /* PrivateBeaconKey */
	} keys[2];
#if defined(CONFIG_BT_MESH_PROXY_SOLICITATION)
	bool sol_tx;
#endif
#if defined(CONFIG_BT_MESH_OD_PRIV_PROXY_SRV)
	uint32_t priv_net_id_sent; /* Timestamp for Private Network ID advertising
				    * started via Proxy Solicitation
				    */
	bool solicited;      /* Subnet received valid Solicitation PDU */
#endif
};

/** Subnet callback structure. Instantiate with @ref BT_MESH_SUBNET_CB */
struct bt_mesh_subnet_cb {
	void (*evt_handler)(struct bt_mesh_subnet *subnet,
			    enum bt_mesh_key_evt evt);
};

/**
 *  @brief Register a subnet event callback.
 *
 *  @param _name Handler name.
 */
#define BT_MESH_SUBNET_CB_DEFINE(_name)                                    \
	static const STRUCT_SECTION_ITERABLE(                               \
		bt_mesh_subnet_cb, _CONCAT(bt_mesh_subnet_cb_, _name))

/** @brief Reset all Network keys. */
void bt_mesh_net_keys_reset(void);

/** @brief Call cb on every valid Subnet until it returns a non-zero value.
 *
 *  @param cb Callback to call, or NULL to return first valid subnet. If the callback returns true,
 *            iteration stops, and the passed subnet is returned.
 *  @param cb_data Callback data to pass to callback.
 *
 *  @return Subnet that returned non-zero value.
 */
struct bt_mesh_subnet *bt_mesh_subnet_find(bool (*cb)(struct bt_mesh_subnet *sub, void *cb_data),
					   void *cb_data);

/** @brief Iterate through all valid Subnets.
 *
 *  @param cb Callback to call on every Subnet.
 *
 *  @returns The number of valid subnets.
 */
size_t bt_mesh_subnet_foreach(void (*cb)(struct bt_mesh_subnet *sub));

/** @brief Get the next valid Subnet.
 *
 *  If there's only one valid Subnet, this will be returned on every call.
 *
 *  @param sub Previous Subnet, or NULL to get the first valid.
 *
 *  @returns Gets the next valid Subnet after @c sub, or NULL if there are no
 *           valid Subnets.
 */
struct bt_mesh_subnet *bt_mesh_subnet_next(struct bt_mesh_subnet *sub);

/** @brief Get a pointer to the Subnet with the given index.
 *
 *  @param net_idx Network index to look for.
 *
 *  @returns Subnet with index @c net_idx, or NULL if no such Subnet is known.
 */
struct bt_mesh_subnet *bt_mesh_subnet_get(uint16_t net_idx);

/** @brief Initialize a new Subnet.
 *
 *  @param net_idx Network index of the Subnet.
 *  @param kr_phase Key refresh phase the Subnet should be in.
 *  @param key The current network key for the Subnet.
 *  @param new_key New network key, if available.
 *
 *  @returns 0 on success, or (negative) error code on failure.
 */
int bt_mesh_subnet_set(uint16_t net_idx, uint8_t kr_phase,
		       const struct bt_mesh_key *key, const struct bt_mesh_key *new_key);

/** @brief Create Friendship credentials.
 *
 *  @param cred Credential object to create.
 *  @param lpn_addr Address of the LPN node in the friendship.
 *  @param frnd_addr Address of the Friend node in the friendship.
 *  @param lpn_counter The LPN's counter parameter.
 *  @param frnd_counter The Friend node's counter parameter.
 *  @param key Network key to create the Friendship credentials for.
 *
 *  @returns 0 on success, or (negative) error code on failure.
 */
int bt_mesh_friend_cred_create(struct bt_mesh_net_cred *cred,
			       uint16_t lpn_addr, uint16_t frnd_addr,
			       uint16_t lpn_counter, uint16_t frnd_counter,
			       const struct bt_mesh_key *key);

/** @brief Destroy Friendship credentials.
 *
 *  @param cred Credential object to destroy.
 */
void bt_mesh_friend_cred_destroy(struct bt_mesh_net_cred *cred);

/** @brief Iterate through all valid network credentials to decrypt a message.
 *
 *  @param rx Network RX parameters, passed to the callback.
 *  @param in Input message buffer, passed to the callback.
 *  @param out Output message buffer, passed to the callback.
 *  @param cb Callback to call for each known network credential. Iteration
 *            stops when this callback returns @c true.
 *
 *  @returns Whether any of the credentials got a @c true return from the
 *           callback.
 */
bool bt_mesh_net_cred_find(struct bt_mesh_net_rx *rx, struct net_buf_simple *in,
			   struct net_buf_simple *out,
			   bool (*cb)(struct bt_mesh_net_rx *rx,
				      struct net_buf_simple *in,
				      struct net_buf_simple *out,
				      const struct bt_mesh_net_cred *cred));

/** @brief Get the network flags of the given Subnet.
 *
 *  @param sub Subnet to get the network flags of.
 *
 *  @returns A bitmap of @ref BT_MESH_NET_FLAG_KR and @ref BT_MESH_NET_FLAG_IVU.
 */
uint8_t bt_mesh_net_flags(struct bt_mesh_subnet *sub);

/** @brief Process a Key Refresh event from a beacon.
 *
 *  @param sub Subnet the Key Refresh was received on.
 *  @param kr_flag Key Refresh flag.
 *  @param new_key Whether the Key Refresh event was received on the new key
 *                 set.
 *
 *  @returns Whether the Key Refresh event caused a change.
 */
void bt_mesh_kr_update(struct bt_mesh_subnet *sub, bool kr_flag, bool new_key);

/** @brief Check whether the Subnet has the refreshed keys.
 *
 *  @param sub Subnet.
 *
 *  @returns Whether the Subnet's second key is valid.
 */
static inline bool
bt_mesh_subnet_has_new_key(const struct bt_mesh_subnet *sub)
{
	return sub->kr_phase != BT_MESH_KR_NORMAL;
}

/** Kind of currently enabled Node Identity state on one or more subnets. */
enum bt_mesh_subnets_node_id_state {
	/* None node identity states are enabled on any subnets. */
	BT_MESH_SUBNETS_NODE_ID_STATE_NONE,
	/* Node Identity state is enabled on one or more subnets. */
	BT_MESH_SUBNETS_NODE_ID_STATE_ENABLED,
	/* Private Node Identity state is enabled on one or more subnets. */
	BT_MESH_SUBNETS_NODE_ID_STATE_ENABLED_PRIVATE,
};

/** @brief Returns what kind of node identity state is currently enabled on one or more subnets.
 *
 * Only one kind (either non-private or private) can be enabled at the same time on all subnets.
 *
 * @returns Kind of node identity state that is currently enabled.
 */
enum bt_mesh_subnets_node_id_state bt_mesh_subnets_node_id_state_get(void);

/** @brief Store the Subnet information in persistent storage.
 *
 * @param net_idx Network index to store.
 */
void bt_mesh_subnet_store(uint16_t net_idx);

/** @brief Store the pending Subnets in persistent storage. */
void bt_mesh_subnet_pending_store(void);

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_MESH_SUBNET_H_ */
