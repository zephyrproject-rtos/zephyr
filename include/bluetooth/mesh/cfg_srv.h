/** @file
 *  @brief Bluetooth Mesh Configuration Server Model APIs.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_CFG_SRV_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_CFG_SRV_H_

/**
 * @brief Bluetooth Mesh
 * @defgroup bt_mesh_cfg_srv Bluetooth Mesh Configuration Server Model
 * @ingroup bt_mesh
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Mesh Configuration Server Model Context */
struct bt_mesh_cfg_srv {
	/** Composition data model entry pointer. */
	struct bt_mesh_model *model;

	uint8_t net_transmit;         /**< Network Transmit state */
	uint8_t relay;                /**< Relay Mode state */
	uint8_t relay_retransmit;     /**< Relay Retransmit state */
	uint8_t beacon;               /**< Secure Network Beacon state */
	uint8_t gatt_proxy;           /**< GATT Proxy state */
	uint8_t frnd;                 /**< Friend state */
	uint8_t default_ttl;          /**< Default TTL */

	/** Heartbeat Publication parameters */
	struct bt_mesh_hb_pub {
		struct k_delayed_work timer;

		/** Destination address. */
		uint16_t dst;
		/** Remaining publish count. */
		uint16_t count;
		/** Logarithmic publish interval in seconds. */
		uint8_t  period;
		/** Time To Live value. */
		uint8_t  ttl;
		/**
		 * Bitmap of features that trigger a Heartbeat publication if
		 * they change. Legal values are
		 * @ref BT_MESH_FEAT_RELAY, @ref BT_MESH_FEAT_PROXY,
		 * @ref BT_MESH_FEAT_FRIEND and @ref BT_MESH_FEAT_LOW_POWER.
		 */
		uint16_t feat;
		/** Network index used for publishing. */
		uint16_t net_idx;
	} hb_pub;

	/** Heartbeat Subscription parameters. */
	struct bt_mesh_hb_sub {
		/** Subscription period exipration timestamp. */
		int64_t  expiry;
		/** Source address to receive Heartbeats from. */
		uint16_t src;
		/** Destination address to received Heartbeats on. */
		uint16_t dst;
		/** The number of received Heartbeat messages so far. */
		uint16_t count;
		/**
		 * Minimum hops in received messages, ie the shortest registered
		 * path from the publishing node to the subscribing node. A
		 * Heartbeat received from an immediate neighbor has hop
		 * count = 1.
		 */
		uint8_t  min_hops;
		/**
		 * Maximum hops in received messages, ie the longest registered
		 * path from the publishing node to the subscribing node. A
		 * Heartbeat received from an immediate neighbor has hop
		 * count = 1.
		 */
		uint8_t  max_hops;

		/** @brief Optional Heartbeat subscription tracking callback.
		 *
		 *  Gets called on every received Heartbeat.
		 *
		 *  @param hops The number of hops the Heartbeat was received
		 *              with.
		 *  @param feat The feature set of the publishing node. The
		 *              value is a bitmap of @ref BT_MESH_FEAT_RELAY,
		 *              @ref BT_MESH_FEAT_PROXY,
		 *              @ref BT_MESH_FEAT_FRIEND and
		 *              @ref BT_MESH_FEAT_LOW_POWER.
		 */
		void (*func)(uint8_t hops, uint16_t feat);
	} hb_sub;
};

/** @def BT_MESH_MODEL_CFG_SRV
 *
 *  @brief Generic Configuration Server model composition data entry.
 *
 *  @param srv_data Pointer to a @ref bt_mesh_cfg_srv instance.
 */
#define BT_MESH_MODEL_CFG_SRV(srv_data)                                        \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_CFG_SRV, bt_mesh_cfg_srv_op, NULL,   \
			 srv_data, &bt_mesh_cfg_srv_cb)

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op bt_mesh_cfg_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_cfg_srv_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_CFG_SRV_H_ */
