/** @file
 *  @brief Bluetooth Mesh Proxy APIs.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_PROXY_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_PROXY_H_

/**
 * @brief Bluetooth Mesh Proxy
 * @defgroup bt_mesh_proxy Bluetooth Mesh Proxy
 * @ingroup bt_mesh
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Enable advertising with Node Identity.
 *
 *  This API requires that GATT Proxy support has been enabled. Once called
 *  each subnet will start advertising using Node Identity for the next
 *  60 seconds.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_proxy_identity_enable(void);

/** Proxy Client Callbacks. */
struct bt_mesh_proxy {
	/** @brief Proxy Network Identity Beacon has been received.
	 *
	 *  This callback notifies the application that Proxy Network
	 *  Identity Beacon has been received.
	 *
	 *  @param addr Remote Bluetooth address.
	 *  @param net_idx NetKeyIndex.
	 */
	void (*network_id)(const bt_addr_le_t *addr, uint16_t net_idx);

	/** @brief Proxy Node Identity Beacon has been received.
	 *
	 *  This callback notifies the application that Node Network
	 *  Identity Beacon has been received.
	 *
	 *  @param addr Remote Bluetooth address.
	 *  @param net_idx NetKeyIndex.
	 *  @param node_addr Node Address.
	 */
	void (*node_id)(const bt_addr_le_t *addr, uint16_t net_idx,
			uint16_t node_addr);

	/** @brief A new proxy connection has been established.
	 *
	 *  This callback notifies the application of a new connection.
	 *  In case the err parameter is non-zero it means that the
	 *  connection establishment failed.
	 *
	 *  @param conn New connection object.
	 *  @param err  Zero for success, non-zero otherwise.
	 */
	void (*connected)(struct bt_conn *conn, uint8_t reason);

	/** @brief A proxy connection has been disconnected.
	 *
	 *  This callback notifies the application that a proxy connection
	 *  has been disconnected.
	 *
	 *  @param conn Connection object.
	 *  @param reason reason for the disconnection.
	 */
	void (*disconnected)(struct bt_conn *conn, uint8_t reason);
};

/** @brief Register a structure for Proxy Client notification to application.
 *
 *  Registers a callback that will be called whenever Proxy Client received
 *  vaild nerwork identity beacon or node identity beacon.
 *
 *  @param cb Callback struct.
 */
void bt_mesh_proxy_client_set_cb(struct bt_mesh_proxy *cb);

/** @brief Initiate an proxy connection to a remote device.
 *
 *  Initiate an proxy connection to a remote device, discovery services and
 *  open the notification.
 *
 *  @param addr Remote address.
 *  @param net_idx NetKeyIndex.
 *
 *  @return Zero on success or (negative) error code on failure.
 */
int bt_mesh_proxy_connect(const bt_addr_le_t *addr, uint16_t net_idx);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_PROXY_H_ */
