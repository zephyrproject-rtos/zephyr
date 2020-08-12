/** @file
 *  @brief Bluetooth Mesh Heartbeat API.
 */

/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_HEARTBEAT_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_HEARTBEAT_H_

#include <sys/slist.h>

/**
 * @brief Bluetooth Mesh
 * @defgroup bt_mesh_heartbeat Bluetooth Mesh Heartbeat
 * @ingroup bt_mesh
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Heartbeat Publication parameters */
struct bt_mesh_hb_pub {
	/** Destination address. */
	uint16_t dst;
	/** Remaining publish count. */
	uint16_t count;
	/** Time To Live value. */
	uint8_t ttl;
	/**
	 * Bitmap of features that trigger a Heartbeat publication if
	 * they change. Legal values are @ref BT_MESH_FEAT_RELAY,
	 * @ref BT_MESH_FEAT_PROXY, @ref BT_MESH_FEAT_FRIEND and
	 * @ref BT_MESH_FEAT_LOW_POWER.
	 */
	uint16_t feat;
	/** Network index used for publishing. */
	uint16_t net_idx;
	/** Publication period in seconds. */
	uint32_t period;
};

/** Heartbeat Subscription parameters. */
struct bt_mesh_hb_sub {
	/** Subscription period in seconds. */
	uint32_t period;
	/** Remaining subscription time in seconds. */
	uint32_t remaining;
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
	uint8_t min_hops;
	/**
	 * Maximum hops in received messages, ie the longest registered
	 * path from the publishing node to the subscribing node. A
	 * Heartbeat received from an immediate neighbor has hop
	 * count = 1.
	 */
	uint8_t max_hops;
};

/** Heartbeat callback structure */
struct bt_mesh_hb_cb {
	/** @brief Receive callback for heartbeats.
	 *
	 *  Gets called on every received Heartbeat that matches the current
	 *  Heartbeat subscription parameters.
	 *
	 *  @param sub  Current Heartbeat subscription parameters.
	 *  @param hops The number of hops the Heartbeat was received
	 *              with.
	 *  @param feat The feature set of the publishing node. The
	 *              value is a bitmap of @ref BT_MESH_FEAT_RELAY,
	 *              @ref BT_MESH_FEAT_PROXY,
	 *              @ref BT_MESH_FEAT_FRIEND and
	 *              @ref BT_MESH_FEAT_LOW_POWER.
	 */
	void (*recv)(const struct bt_mesh_hb_sub *sub, uint8_t hops,
		     uint16_t feat);

	/** @brief Subscription end callback for heartbeats.
	 *
	 *  Gets called when the subscription period ends, providing a summary
	 *  of the received heartbeat messages.
	 *
	 *  @param sub Current Heartbeat subscription parameters.
	 */
	void (*sub_end)(const struct bt_mesh_hb_sub *sub);
};

/** @def BT_MESH_HB_CB_DEFINE
 *
 *  @brief Register a callback structure for Heartbeat events.
 *
 *  Registers a callback structure that will be called whenever Heartbeat
 *  events occur
 *
 *  @param _name Name of callback structure.
 */
#define BT_MESH_HB_CB_DEFINE(_name)                                            \
	static const Z_STRUCT_SECTION_ITERABLE(bt_mesh_hb_cb, _name)

/** @brief Get the current Heartbeat publication parameters.
 *
 *  @param get Heartbeat publication parameters return buffer.
 */
void bt_mesh_hb_pub_get(struct bt_mesh_hb_pub *get);

/** @brief Get the current Heartbeat subscription parameters.
 *
 *  @param get Heartbeat subscription parameters return buffer.
 */
void bt_mesh_hb_sub_get(struct bt_mesh_hb_sub *get);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_HEARTBEAT_H_ */
