/** @file
 *  @brief Bluetooth Personal Area Networking Profile (PAN)
 */

/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_PAN_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_PAN_H_

#include <zephyr/bluetooth/conn.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Personal Area Networking Profile (PAN)
 * @defgroup bt_pan Personal Area Networking Profile (PAN)
 * @ingroup bluetooth
 * @{
 */

/** PAN profile version */
#define BT_PAN_VERSION 0x0100

/** @brief PAN role */
enum bt_pan_role {
	/** Personal Area Network User */
	BT_PAN_ROLE_PANU = 0,
	/** Network Access Point */
	BT_PAN_ROLE_NAP = 1,
};

/** @brief PAN connection state */
enum bt_pan_state {
	/** BNEP connection is not established */
	BT_PAN_STATE_DISCONNECTED = 0,
	/** BNEP connection is being established */
	BT_PAN_STATE_CONNECTING = 1,
	/** BNEP connection is established */
	BT_PAN_STATE_CONNECTED = 2,
};

struct bt_pan;

/** @brief PAN callbacks */
struct bt_pan_cb {
	/** Accept an incoming BNEP connection (NAP role).
	 *  Return 0 to accept, negative errno to reject.
	 */
	int (*accept)(struct bt_conn *conn, struct bt_pan **pan);

	/** BNEP connection established */
	void (*connected)(struct bt_pan *pan);

	/** BNEP connection terminated */
	void (*disconnected)(struct bt_pan *pan);

	/** Ethernet frame received (without BNEP header) */
	void (*recv)(struct bt_pan *pan, struct net_buf *buf);
};

/** @brief Register PAN role and callbacks
 *
 *  @param role PAN role (PANU or NAP).
 *  @param cb Callback structure.
 *
 *  @return 0 on success, negative errno on failure.
 */
int bt_pan_register(enum bt_pan_role role, const struct bt_pan_cb *cb);

/** @brief Get active PAN session for ACL connection
 *
 *  @param conn ACL connection.
 *
 *  @return PAN session or NULL if BNEP is not active.
 */
struct bt_pan *bt_pan_get(struct bt_conn *conn);

/** @brief Look up PAN session slot for ACL connection
 *
 *  @param conn ACL connection.
 *
 *  @return PAN session slot or NULL if connection index is invalid.
 */
struct bt_pan *bt_pan_lookup(struct bt_conn *conn);

/** @brief Initiate BNEP connection (PANU role)
 *
 *  @param conn ACL connection to NAP.
 *  @param pan PAN session.
 *
 *  @return 0 on success, negative errno on failure.
 */
int bt_pan_connect(struct bt_conn *conn, struct bt_pan *pan);

/** @brief Disconnect BNEP session
 *
 *  @param pan PAN session.
 *
 *  @return 0 on success, negative errno on failure.
 */
int bt_pan_disconnect(struct bt_pan *pan);

/** @brief Send Ethernet frame over BNEP
 *
 *  @param pan PAN session.
 *  @param buf Buffer containing Ethernet frame.
 *
 *  @return 0 on success, negative errno on failure.
 */
int bt_pan_send(struct bt_pan *pan, struct net_buf *buf);

/** @brief Allocate buffer for Ethernet frame transmission
 *
 *  @param len Ethernet frame length.
 *
 *  @return Allocated buffer or NULL.
 */
struct net_buf *bt_pan_alloc_buf(size_t len);

/** @brief Get PAN connection state */
enum bt_pan_state bt_pan_get_state(const struct bt_pan *pan);

/** @brief Get PAN role */
enum bt_pan_role bt_pan_get_role(const struct bt_pan *pan);

/** @brief Get ACL connection for PAN session */
struct bt_conn *bt_pan_get_conn(const struct bt_pan *pan);

#if defined(CONFIG_BT_PAN_NET)

/** @brief PAN network device context */
struct bt_pan_net_context {
	/** MAC address */
	uint8_t mac_addr[6];
	/** Associated PAN session */
	struct bt_pan *pan;
};

/** @brief Initialize PAN network device */
int bt_pan_net_init(const struct device *dev);

extern const struct ethernet_api bt_pan_net_api;

/** @brief Attach PAN session to network interface
 *
 *  @param pan PAN session.
 *  @param iface Network interface.
 */
void bt_pan_net_attach(struct bt_pan *pan, struct net_if *iface);

/** @brief Detach PAN session from network interface */
void bt_pan_net_detach(struct bt_pan *pan);

/** @brief Get network interface for a PAN network device
 *
 *  @param dev Network device created with BT_PAN_NET_DEVICE_DEFINE.
 *
 *  @return Network interface or NULL if not found.
 */
struct net_if *bt_pan_net_get_iface(const struct device *dev);

/** @brief Define a Bluetooth PAN network device
 *
 *  @param name Device instance name.
 *  @param dev_name Network device name string.
 *  @param ... Six-byte MAC address as individual octet values.
 */
#define BT_PAN_NET_DEVICE_DEFINE(name, dev_name, ...)			\
	static struct bt_pan_net_context name##_ctx = {			\
		.mac_addr = { __VA_ARGS__ },				\
	};								\
	NET_DEVICE_INIT(name, dev_name, bt_pan_net_init, NULL,		\
			&name##_ctx, NULL, CONFIG_ETH_INIT_PRIORITY,	\
			&bt_pan_net_api, ETHERNET_L2,			\
			NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU)

#endif /* CONFIG_BT_PAN_NET */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_PAN_H_ */
